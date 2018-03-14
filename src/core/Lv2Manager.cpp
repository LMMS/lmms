/*
 * Lv2Manager.cpp - a class to manage loading and instantiation
 *						of LV2 plugins
 *
 * Copyright (c) Alexandros Theodotou @faiyadesu
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QtCore>

#include "Lv2Manager.h"

#define _POSIX_C_SOURCE 200809L /* for mkdtemp */
#define _DARWIN_C_SOURCE        /* for mkdtemp on OSX */

extern "C" {
#include <assert.h>
#include <malloc.h>
#include <cmath>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef __cplusplus
#    include <stdbool.h>
#endif

#ifdef _WIN32
#    include <io.h>  /* for _mktemp */
#    define snprintf _snprintf
#else
#    include <unistd.h>
#endif



#include "lilv/lilv.h"

#include "suil/suil.h"

#include "lv2_evbuf.h"
#include "worker.h"
}

#include "Engine.h"
#include "ConfigManager.h"
#include "Song.h"

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"


ZixSem exit_sem;  /**< Exit semaphore */

static LV2_URID
map_uri(LV2_URID_Map_Handle handle,
        const char*         uri)
{
	Jalv* jalv = (Jalv*)handle;
	zix_sem_wait(&jalv->symap_lock);
	const LV2_URID id = symap_map(jalv->symap, uri);
	zix_sem_post(&jalv->symap_lock);
	return id;
}


static const char*
unmap_uri(LV2_URID_Unmap_Handle handle,
          LV2_URID              urid)
{
	Jalv* jalv = (Jalv*)handle;
	zix_sem_wait(&jalv->symap_lock);
	const char* uri = symap_unmap(jalv->symap, urid);
	zix_sem_post(&jalv->symap_lock);
	return uri;
}

/**
   Map function for URI map extension.
*/
static uint32_t
uri_to_id(LV2_URI_Map_Callback_Data callback_data,
          const char*               map,
          const char*               uri)
{
	Jalv* jalv = (Jalv*)callback_data;
	zix_sem_wait(&jalv->symap_lock);
	const LV2_URID id = symap_map(jalv->symap, uri);
	zix_sem_post(&jalv->symap_lock);
	return id;
}

static LV2_URI_Map_Feature uri_map = { NULL, &uri_to_id };

LV2_Extension_Data_Feature Lv2Manager::ext_data = { NULL };


/** These features have no data */
static LV2_Feature buf_size_features[3] = {
	{ LV2_BUF_SIZE__powerOf2BlockLength, NULL },
	{ LV2_BUF_SIZE__fixedBlockLength, NULL },
	{ LV2_BUF_SIZE__boundedBlockLength, NULL } };


extern LV2_Feature uri_map_feature;
extern LV2_Feature map_feature;
extern LV2_Feature unmap_feature;
extern LV2_Feature make_path_feature;
extern LV2_Feature sched_feature;
extern LV2_Feature state_sched_feature;
extern LV2_Feature safe_restore_feature;
extern LV2_Feature log_feature;
extern LV2_Feature options_feature;
extern LV2_Feature def_state_feature;

const LV2_Feature* Lv2Manager::features[12] = {
	&uri_map_feature,
	&map_feature,
	&unmap_feature,
	&sched_feature,
	&log_feature,
	&options_feature,
	&def_state_feature,
	&safe_restore_feature,
	&buf_size_features[0],
	&buf_size_features[1],
	&buf_size_features[2],
	NULL
};

/** Return true iff Jalv supports the given feature. */
bool
Lv2Manager::feature_is_supported(const char* uri)
{
	if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#isLive")) {
		return true;
	}
	for (const LV2_Feature*const* f = features; *f; ++f) {
		if (!strcmp(uri, (*f)->URI)) {
			return true;
		}
	}
	return false;
}

/** Abort and exit on error */
static void
die(const char* msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

/**
   Create a port structure from data description.  This is called before plugin
   and Jack instantiation.  The remaining instance-specific setup
   (e.g. buffers) is done later in activate_port().
*/
static void
create_port(Jalv*    jalv,
            uint32_t port_index,
            float    default_value)
{
	struct Port* const port = &jalv->ports[port_index];

	port->lilv_port = lilv_plugin_get_port_by_index(jalv->plugin, port_index);
	port->sys_port  = NULL;
	port->evbuf     = NULL;
	port->buf_size  = 0;
	port->index     = port_index;
	port->control   = 0.0f;
	port->flow      = FLOW_UNKNOWN;

	const bool optional = lilv_port_has_property(
		jalv->plugin, port->lilv_port, jalv->nodes.lv2_connectionOptional);

	/* Set the port flow (input or output) */
	if (lilv_port_is_a(jalv->plugin, port->lilv_port, jalv->nodes.lv2_InputPort)) {
		port->flow = FLOW_INPUT;
	} else if (lilv_port_is_a(jalv->plugin, port->lilv_port,
	                          jalv->nodes.lv2_OutputPort)) {
		port->flow = FLOW_OUTPUT;
	} else if (!optional) {
		die("Mandatory port has unknown type (neither input nor output)");
	}

	/* Set control values */
	if (lilv_port_is_a(jalv->plugin, port->lilv_port, jalv->nodes.lv2_ControlPort)) {
		port->type    = TYPE_CONTROL;
		port->control = std::isnan(default_value) ? 0.0f : default_value;
		if (jalv->opts.show_hidden ||
		    !lilv_port_has_property(jalv->plugin, port->lilv_port, jalv->nodes.pprops_notOnGUI)) {
			add_control(&jalv->controls, new_port_control(jalv, port->index));
		}
	} else if (lilv_port_is_a(jalv->plugin, port->lilv_port,
	                          jalv->nodes.lv2_AudioPort)) {
		port->type = TYPE_AUDIO;
#ifdef HAVE_JACK_METADATA
	} else if (lilv_port_is_a(jalv->plugin, port->lilv_port,
	                          jalv->nodes.lv2_CVPort)) {
		port->type = TYPE_CV;
#endif
	} else if (lilv_port_is_a(jalv->plugin, port->lilv_port,
	                          jalv->nodes.ev_EventPort)) {
		port->type = TYPE_EVENT;
		port->old_api = true;
	} else if (lilv_port_is_a(jalv->plugin, port->lilv_port,
	                          jalv->nodes.atom_AtomPort)) {
		port->type = TYPE_EVENT;
		port->old_api = false;
	} else if (!optional) {
		die("Mandatory port has unknown data type");
	}

	LilvNode* min_size = lilv_port_get(
		jalv->plugin, port->lilv_port, jalv->nodes.rsz_minimumSize);
	if (min_size && lilv_node_is_int(min_size)) {
		port->buf_size = lilv_node_as_int(min_size);
		jalv->opts.buffer_size = MAX(
			jalv->opts.buffer_size, port->buf_size * N_BUFFER_CYCLES);
	}
	lilv_node_free(min_size);

	/* Update longest symbol for aligned console printing */
	const LilvNode* sym = lilv_port_get_symbol(jalv->plugin, port->lilv_port);
	const size_t    len = strlen(lilv_node_as_string(sym));
	if (len > jalv->longest_sym) {
		jalv->longest_sym = len;
	}
}

/**
   Create port structures from data (via create_port()) for all ports.
*/
void
jalv_create_ports(Jalv* jalv)
{
	jalv->num_ports = lilv_plugin_get_num_ports(jalv->plugin);
	jalv->ports     = (struct Port*)calloc(jalv->num_ports, sizeof(struct Port));
	float* default_values = (float*)calloc(
		lilv_plugin_get_num_ports(jalv->plugin), sizeof(float));
	lilv_plugin_get_port_ranges_float(jalv->plugin, NULL, NULL, default_values);

	for (uint32_t i = 0; i < jalv->num_ports; ++i) {
		create_port(jalv, i, default_values[i]);
	}

	const LilvPort* control_input = lilv_plugin_get_port_by_designation(
		jalv->plugin, jalv->nodes.lv2_InputPort, jalv->nodes.lv2_control);
	if (control_input) {
		jalv->control_in = lilv_port_get_index(jalv->plugin, control_input);
	}

	free(default_values);
}

/**
   Allocate port buffers (only necessary for MIDI).
*/
void
jalv_allocate_port_buffers(Jalv* jalv)
{
	for (uint32_t i = 0; i < jalv->num_ports; ++i) {
		struct Port* const port     = &jalv->ports[i];
		switch (port->type) {
		case TYPE_EVENT: {
			lv2_evbuf_free(port->evbuf);
			const size_t buf_size = (port->buf_size > 0)
				? port->buf_size
				: jalv->midi_buf_size;
			port->evbuf = lv2_evbuf_new(
				buf_size,
				port->old_api ? LV2_EVBUF_EVENT : LV2_EVBUF_ATOM,
				jalv->map.map(jalv->map.handle,
				              lilv_node_as_string(jalv->nodes.atom_Chunk)),
				jalv->map.map(jalv->map.handle,
				              lilv_node_as_string(jalv->nodes.atom_Sequence)));
			lilv_instance_connect_port(
				jalv->instance, i, lv2_evbuf_get_buffer(port->evbuf));
		}
		default: break;
		}
	}
}

/**
   Get a port structure by symbol.

   TODO: Build an index to make this faster, currently O(n) which may be
   a problem when restoring the state of plugins with many ports.
*/
struct Port*
jalv_port_by_symbol(Jalv* jalv, const char* sym)
{
	for (uint32_t i = 0; i < jalv->num_ports; ++i) {
		struct Port* const port     = &jalv->ports[i];
		const LilvNode*    port_sym = lilv_port_get_symbol(jalv->plugin,
		                                                   port->lilv_port);

		if (!strcmp(lilv_node_as_string(port_sym), sym)) {
			return port;
		}
	}

	return NULL;
}

ControlID*
jalv_control_by_symbol(Jalv* jalv, const char* sym)
{
	for (size_t i = 0; i < jalv->controls.n_controls; ++i) {
		if (!strcmp(lilv_node_as_string(jalv->controls.controls[i]->symbol),
		            sym)) {
			return jalv->controls.controls[i];
		}
	}
	return NULL;
}

void
Lv2Manager::print_control_value(Jalv* jalv, const struct Port* port, float value)
{
	const LilvNode* sym = lilv_port_get_symbol(jalv->plugin, port->lilv_port);
	printf("%-*s = %f\n", jalv->longest_sym, lilv_node_as_string(sym), value);
}

void
jalv_create_controls(Jalv* jalv, bool writable)
{
	const LilvPlugin* plugin         = jalv->plugin;
	LilvWorld*        world          = jalv->world;
	LilvNode*         patch_writable = lilv_new_uri(world, LV2_PATCH__writable);
	LilvNode*         patch_readable = lilv_new_uri(world, LV2_PATCH__readable);

	LilvNodes* properties = lilv_world_find_nodes(
		world,
		lilv_plugin_get_uri(plugin),
		writable ? patch_writable : patch_readable,
		NULL);
	LILV_FOREACH(nodes, p, properties) {
		const LilvNode* property = lilv_nodes_get(properties, p);
		ControlID*      record   = NULL;

		if (!writable && lilv_world_ask(world,
		                                lilv_plugin_get_uri(plugin),
		                                patch_writable,
		                                property)) {
			// Find existing writable control
			for (size_t i = 0; i < jalv->controls.n_controls; ++i) {
				if (lilv_node_equals(jalv->controls.controls[i]->node, property)) {
					record              = jalv->controls.controls[i];
					record->is_readable = true;
					break;
				}
			}

			if (record) {
				continue;
			}
		}

		record = new_property_control(jalv, property);
		if (writable) {
			record->is_writable = true;
		} else {
			record->is_readable = true;
		}

		if (record->value_type) {
			add_control(&jalv->controls, record);
		} else {
			fprintf(stderr, "Parameter <%s> has unknown value type, ignored\n",
			        lilv_node_as_string(record->node));
			free(record);
		}
	}
	lilv_nodes_free(properties);

	lilv_node_free(patch_readable);
	lilv_node_free(patch_writable);
}

void
jalv_set_control(const ControlID* control,
                 uint32_t         size,
                 LV2_URID         type,
                 const void*      body)
{
	Jalv* jalv = control->jalv;
	if (control->type == PORT && type == jalv->forge.Float) {
		struct Port* port = &control->jalv->ports[control->index];
		port->control = *(float*)body;
	} else if (control->type == PROPERTY) {
		// Copy forge since it is used by process thread
		LV2_Atom_Forge       forge = jalv->forge;
		LV2_Atom_Forge_Frame frame;
		uint8_t              buf[1024];
		lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));

		lv2_atom_forge_object(&forge, &frame, 0, jalv->urids.patch_Set);
		lv2_atom_forge_key(&forge, jalv->urids.patch_property);
		lv2_atom_forge_urid(&forge, control->property);
		lv2_atom_forge_key(&forge, jalv->urids.patch_value);
		lv2_atom_forge_atom(&forge, size, type);
		lv2_atom_forge_write(&forge, body, size);

		const LV2_Atom* atom = lv2_atom_forge_deref(&forge, frame.ref);
		jalv_ui_write(jalv,
		              jalv->control_in,
		              lv2_atom_total_size(atom),
		              jalv->urids.atom_eventTransfer,
		              atom);
	}
}

void
jalv_ui_instantiate(Jalv* jalv, const char* native_ui_type, void* parent)
{
	jalv->ui_host = suil_host_new(jalv_ui_write, jalv_ui_port_index, NULL, NULL);

	const LV2_Feature parent_feature = {
		LV2_UI__parent, parent
	};
	const LV2_Feature instance_feature = {
		NS_EXT "instance-access", lilv_instance_get_handle(jalv->instance)
	};
	const LV2_Feature data_feature = {
		LV2_DATA_ACCESS_URI, &Lv2Manager::ext_data
	};
	const LV2_Feature idle_feature = {
		LV2_UI__idleInterface, NULL
	};
	const LV2_Feature* ui_features[] = {
		&uri_map_feature, &map_feature, &unmap_feature,
		&instance_feature,
		&data_feature,
		&log_feature,
		&parent_feature,
		&options_feature,
		&idle_feature,
		NULL
	};

	const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(jalv->ui));
	const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(jalv->ui));
	char*       bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
	char*       binary_path = lilv_file_uri_parse(binary_uri, NULL);

	jalv->ui_instance = suil_instance_new(
		jalv->ui_host,
		jalv,
		native_ui_type,
		lilv_node_as_uri(lilv_plugin_get_uri(jalv->plugin)),
		lilv_node_as_uri(lilv_ui_get_uri(jalv->ui)),
		lilv_node_as_uri(jalv->ui_type),
		bundle_path,
		binary_path,
		ui_features);

	lilv_free(binary_path);
	lilv_free(bundle_path);
}

bool
jalv_ui_is_resizable(Jalv* jalv)
{
	if (!jalv->ui) {
		return false;
	}

	const LilvNode* s   = lilv_ui_get_uri(jalv->ui);
	LilvNode*       p   = lilv_new_uri(jalv->world, LV2_CORE__optionalFeature);
	LilvNode*       fs  = lilv_new_uri(jalv->world, LV2_UI__fixedSize);
	LilvNode*       nrs = lilv_new_uri(jalv->world, LV2_UI__noUserResize);

	LilvNodes* fs_matches = lilv_world_find_nodes(jalv->world, s, p, fs);
	LilvNodes* nrs_matches = lilv_world_find_nodes(jalv->world, s, p, nrs);

	lilv_nodes_free(nrs_matches);
	lilv_nodes_free(fs_matches);
	lilv_node_free(nrs);
	lilv_node_free(fs);
	lilv_node_free(p);

	return !fs_matches && !nrs_matches;
}

void
jalv_ui_write(SuilController controller,
              uint32_t       port_index,
              uint32_t       buffer_size,
              uint32_t       protocol,
              const void*    buffer)
{
	Jalv* const jalv = (Jalv*)controller;

	if (protocol != 0 && protocol != jalv->urids.atom_eventTransfer) {
		fprintf(stderr, "UI write with unsupported protocol %d (%s)\n",
		        protocol, unmap_uri(jalv, protocol));
		return;
	}

	if (port_index >= jalv->num_ports) {
		fprintf(stderr, "UI write to out of range port index %d\n",
		        port_index);
		return;
	}

	if (jalv->opts.dump && protocol == jalv->urids.atom_eventTransfer) {
		const LV2_Atom* atom = (const LV2_Atom*)buffer;
		char*           str  = sratom_to_turtle(
			jalv->sratom, &jalv->unmap, "jalv:", NULL, NULL,
			atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));
		jalv_ansi_start(stdout, 36);
		printf("\n## UI => Plugin (%u bytes) ##\n%s\n", atom->size, str);
		jalv_ansi_reset(stdout);
		free(str);
	}

	char buf[sizeof(ControlChange) + buffer_size];
	ControlChange* ev = (ControlChange*)buf;
	ev->index    = port_index;
	ev->protocol = protocol;
	ev->size     = buffer_size;
	memcpy(ev->body, buffer, buffer_size);
	zix_ring_write(jalv->ui_events, buf, sizeof(buf));
}

void
jalv_apply_ui_events(Jalv* jalv, uint32_t nframes)
{
	if (!jalv->has_ui) {
		return;
	}

	ControlChange ev;
	const size_t  space = zix_ring_read_space(jalv->ui_events);
	for (size_t i = 0; i < space; i += sizeof(ev) + ev.size) {
		zix_ring_read(jalv->ui_events, (char*)&ev, sizeof(ev));
		char body[ev.size];
		if (zix_ring_read(jalv->ui_events, body, ev.size) != ev.size) {
			fprintf(stderr, "error: Error reading from UI ring buffer\n");
			break;
		}
		assert(ev.index < jalv->num_ports);
		struct Port* const port = &jalv->ports[ev.index];
		if (ev.protocol == 0) {
			assert(ev.size == sizeof(float));
			port->control = *(float*)body;
		} else if (ev.protocol == jalv->urids.atom_eventTransfer) {
			LV2_Evbuf_Iterator    e    = lv2_evbuf_end(port->evbuf);
			const LV2_Atom* const atom = (const LV2_Atom*)body;
			lv2_evbuf_write(&e, nframes, 0, atom->type, atom->size,
			                (const uint8_t*)LV2_ATOM_BODY_CONST(atom));
		} else {
			fprintf(stderr, "error: Unknown control change protocol %d\n",
			        ev.protocol);
		}
	}
}

uint32_t
jalv_ui_port_index(SuilController controller, const char* symbol)
{
	Jalv* const  jalv = (Jalv*)controller;
	struct Port* port = jalv_port_by_symbol(jalv, symbol);

	return port ? port->index : LV2UI_INVALID_PORT_INDEX;
}

void
jalv_init_ui(Jalv* jalv)
{
	// Set initial control port values
	for (uint32_t i = 0; i < jalv->num_ports; ++i) {
		if (jalv->ports[i].type == TYPE_CONTROL) {
			jalv_ui_port_event(jalv, i,
			                   sizeof(float), 0,
			                   &jalv->ports[i].control);
		}
	}

	if (jalv->control_in != (uint32_t)-1) {
		// Send patch:Get message for initial parameters/etc
		LV2_Atom_Forge       forge = jalv->forge;
		LV2_Atom_Forge_Frame frame;
		uint8_t              buf[1024];
		lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));
		lv2_atom_forge_object(&forge, &frame, 0, jalv->urids.patch_Get);

		const LV2_Atom* atom = lv2_atom_forge_deref(&forge, frame.ref);
		jalv_ui_write(jalv,
		              jalv->control_in,
		              lv2_atom_total_size(atom),
		              jalv->urids.atom_eventTransfer,
		              atom);
		lv2_atom_forge_pop(&forge, &frame);
	}
}

bool
jalv_send_to_ui(Jalv*       jalv,
                uint32_t    port_index,
                uint32_t    type,
                uint32_t    size,
                const void* body)
{
	/* TODO: Be more disciminate about what to send */
	char evbuf[sizeof(ControlChange) + sizeof(LV2_Atom)];
	ControlChange* ev = (ControlChange*)evbuf;
	ev->index    = port_index;
	ev->protocol = jalv->urids.atom_eventTransfer;
	ev->size     = sizeof(LV2_Atom) + size;

	LV2_Atom* atom = (LV2_Atom*)ev->body;
	atom->type = type;
	atom->size = size;

	if (zix_ring_write_space(jalv->plugin_events) >= sizeof(evbuf) + size) {
		zix_ring_write(jalv->plugin_events, evbuf, sizeof(evbuf));
		zix_ring_write(jalv->plugin_events, (const char*)body, size);
		return true;
	} else {
		fprintf(stderr, "Plugin => UI buffer overflow!\n");
		return false;
	}
}

bool
jalv_run(Jalv* jalv, uint32_t nframes)
{
	/* Read and apply control change events from UI */
	jalv_apply_ui_events(jalv, nframes);

	/* Run plugin for this cycle */
	lilv_instance_run(jalv->instance, nframes);

	/* Process any worker replies. */
	jalv_worker_emit_responses(&jalv->state_worker, jalv->instance);
	jalv_worker_emit_responses(&jalv->worker, jalv->instance);

	/* Notify the plugin the run() cycle is finished */
	if (jalv->worker.iface && jalv->worker.iface->end_run) {
		jalv->worker.iface->end_run(jalv->instance->lv2_handle);
	}

	/* Check if it's time to send updates to the UI */
	jalv->event_delta_t += nframes;
	bool     send_ui_updates = false;
	uint32_t update_frames   = jalv->sample_rate / jalv->ui_update_hz;
	if (jalv->has_ui && (jalv->event_delta_t > update_frames)) {
		send_ui_updates = true;
		jalv->event_delta_t = 0;
	}

	return send_ui_updates;
}

bool
jalv_update(Jalv* jalv)
{
	/* Check quit flag and close if set. */
	if (zix_sem_try_wait(&exit_sem)) {
		jalv_close_ui(jalv);
		return false;
	}

	/* Emit UI events. */
	ControlChange ev;
	const size_t  space = zix_ring_read_space(jalv->plugin_events);
	for (size_t i = 0;
	     i + sizeof(ev) < space;
	     i += sizeof(ev) + ev.size) {
		/* Read event header to get the size */
		zix_ring_read(jalv->plugin_events, (char*)&ev, sizeof(ev));

		/* Resize read buffer if necessary */
		jalv->ui_event_buf = realloc(jalv->ui_event_buf, ev.size);
		void* const buf = jalv->ui_event_buf;

		/* Read event body */
		zix_ring_read(jalv->plugin_events, (char*)buf, ev.size);

		if (jalv->opts.dump && ev.protocol == jalv->urids.atom_eventTransfer) {
			/* Dump event in Turtle to the console */
			LV2_Atom* atom = (LV2_Atom*)buf;
			char*     str  = sratom_to_turtle(
				jalv->ui_sratom, &jalv->unmap, "jalv:", NULL, NULL,
				atom->type, atom->size, LV2_ATOM_BODY(atom));
			jalv_ansi_start(stdout, 35);
			printf("\n## Plugin => UI (%u bytes) ##\n%s\n", atom->size, str);
			jalv_ansi_reset(stdout);
			free(str);
		}

		if (jalv->ui_instance) {
			suil_instance_port_event(jalv->ui_instance, ev.index,
			                         ev.size, ev.protocol, buf);
		} else {
			jalv_ui_port_event(jalv, ev.index, ev.size, ev.protocol, buf);
		}

		if (ev.protocol == 0 && jalv->opts.print_controls) {
			Lv2Manager::print_control_value(jalv, &jalv->ports[ev.index], *(float*)buf);
		}
	}

	return true;
}

//static bool
//jalv_apply_control_arg(Jalv* jalv, const char* s)
//{
	//char  sym[256];
	//float val = 0.0f;
	//if (sscanf(s, "%[^=]=%f", sym, &val) != 2) {
		//fprintf(stderr, "warning: Ignoring invalid value `%s'\n", s);
		//return false;
	//}

	//ControlID* control = jalv_control_by_symbol(jalv, sym);
	//if (!control) {
		//fprintf(stderr, "warning: Ignoring value for unknown control `%s'\n", sym);
		//return false;
	//}

	//jalv_set_control(control, sizeof(float), jalv->urids.atom_Float, &val);
	//printf("%-*s = %f\n", jalv->longest_sym, sym, val);

	//return true;
//}


static void
signal_handler(int ignored)
{
	zix_sem_post(&exit_sem);
}

const LilvPlugin* Lv2Manager::find_by_uri(const char* uri)
{
	const LilvNode* node = lilv_new_uri(
			jalv.world, uri);
	const LilvPlugins* plugins = lilv_world_get_all_plugins(jalv.world);
	return lilv_plugins_get_by_uri(plugins, node);
}

Lv2Manager::Lv2Manager()
{
	memset(&jalv, '\0', sizeof(Jalv));
	jalv.prog_name     = "lmms";
	jalv.block_length  = ConfigManager::inst()
		->value("mixer", "framesperaudiobuffer").toInt();
	jalv.midi_buf_size = 1024;  /* Should be set by backend */
	jalv.play_state    = JALV_PAUSED;
	jalv.bpm           = Engine::getSong()->getTempo();
	jalv.control_in    = (uint32_t)-1;

	suil_init(0, NULL, SUIL_ARG_NONE);

	if (jalv.opts.uuid) {
		printf("UUID: %s\n", jalv.opts.uuid);
	}

	jalv.symap = symap_new();
	zix_sem_init(&jalv.symap_lock, 1);
	zix_sem_init(&jalv.work_lock, 1);
	uri_map_feature.data  = &uri_map;
	uri_map.callback_data = &jalv;

	jalv.map.handle  = &jalv;
	jalv.map.map     = map_uri;
	map_feature.data = &jalv.map;

	jalv.worker.jalv       = &jalv;
	jalv.state_worker.jalv = &jalv;

	jalv.unmap.handle  = &jalv;
	jalv.unmap.unmap   = unmap_uri;
	unmap_feature.data = &jalv.unmap;

	lv2_atom_forge_init(&jalv.forge, &jalv.map);

	jalv.env = serd_env_new(NULL);
	serd_env_set_prefix_from_strings(
		jalv.env, (const uint8_t*)"patch", (const uint8_t*)LV2_PATCH_PREFIX);
	serd_env_set_prefix_from_strings(
		jalv.env, (const uint8_t*)"time", (const uint8_t*)LV2_TIME_PREFIX);
	serd_env_set_prefix_from_strings(
		jalv.env, (const uint8_t*)"xsd", (const uint8_t*)NS_XSD);

	jalv.sratom    = sratom_new(&jalv.map);
	jalv.ui_sratom = sratom_new(&jalv.map);
	sratom_set_env(jalv.sratom, jalv.env);
	sratom_set_env(jalv.ui_sratom, jalv.env);

	jalv.midi_event_id = uri_to_id(
		&jalv, "http://lv2plug.in/ns/ext/event", LV2_MIDI__MidiEvent);

	jalv.urids.atom_Float           = symap_map(jalv.symap, LV2_ATOM__Float);
	jalv.urids.atom_Int             = symap_map(jalv.symap, LV2_ATOM__Int);
	jalv.urids.atom_Object          = symap_map(jalv.symap, LV2_ATOM__Object);
	jalv.urids.atom_Path            = symap_map(jalv.symap, LV2_ATOM__Path);
	jalv.urids.atom_String          = symap_map(jalv.symap, LV2_ATOM__String);
	jalv.urids.atom_eventTransfer   = symap_map(jalv.symap, LV2_ATOM__eventTransfer);
	jalv.urids.bufsz_maxBlockLength = symap_map(jalv.symap, LV2_BUF_SIZE__maxBlockLength);
	jalv.urids.bufsz_minBlockLength = symap_map(jalv.symap, LV2_BUF_SIZE__minBlockLength);
	jalv.urids.bufsz_sequenceSize   = symap_map(jalv.symap, LV2_BUF_SIZE__sequenceSize);
	jalv.urids.log_Error            = symap_map(jalv.symap, LV2_LOG__Error);
	jalv.urids.log_Trace            = symap_map(jalv.symap, LV2_LOG__Trace);
	jalv.urids.log_Warning          = symap_map(jalv.symap, LV2_LOG__Warning);
	jalv.urids.midi_MidiEvent       = symap_map(jalv.symap, LV2_MIDI__MidiEvent);
	jalv.urids.param_sampleRate     = symap_map(jalv.symap, LV2_PARAMETERS__sampleRate);
	jalv.urids.patch_Get            = symap_map(jalv.symap, LV2_PATCH__Get);
	jalv.urids.patch_Put            = symap_map(jalv.symap, LV2_PATCH__Put);
	jalv.urids.patch_Set            = symap_map(jalv.symap, LV2_PATCH__Set);
	jalv.urids.patch_body           = symap_map(jalv.symap, LV2_PATCH__body);
	jalv.urids.patch_property       = symap_map(jalv.symap, LV2_PATCH__property);
	jalv.urids.patch_value          = symap_map(jalv.symap, LV2_PATCH__value);
	jalv.urids.time_Position        = symap_map(jalv.symap, LV2_TIME__Position);
	jalv.urids.time_bar             = symap_map(jalv.symap, LV2_TIME__bar);
	jalv.urids.time_barBeat         = symap_map(jalv.symap, LV2_TIME__barBeat);
	jalv.urids.time_beatUnit        = symap_map(jalv.symap, LV2_TIME__beatUnit);
	jalv.urids.time_beatsPerBar     = symap_map(jalv.symap, LV2_TIME__beatsPerBar);
	jalv.urids.time_beatsPerMinute  = symap_map(jalv.symap, LV2_TIME__beatsPerMinute);
	jalv.urids.time_frame           = symap_map(jalv.symap, LV2_TIME__frame);
	jalv.urids.time_speed           = symap_map(jalv.symap, LV2_TIME__speed);
	jalv.urids.ui_updateRate        = symap_map(jalv.symap, LV2_UI__updateRate);

#ifdef _WIN32
	jalv.temp_dir = jalv_strdup("jalvXXXXXX");
	_mktemp(jalv.temp_dir);
#else
	char* templ = jalv_strdup("/tmp/jalv-XXXXXX");
	jalv.temp_dir = jalv_strjoin(mkdtemp(templ), "/");
	free(templ);
#endif

	LV2_State_Make_Path make_path = { &jalv, jalv_make_path };
	make_path_feature.data = &make_path;

	LV2_Worker_Schedule sched = { &jalv.worker, jalv_worker_schedule };
	sched_feature.data = &sched;

	LV2_Worker_Schedule ssched = { &jalv.state_worker, jalv_worker_schedule };
	state_sched_feature.data = &ssched;

	LV2_Log_Log llog = { &jalv, jalv_printf, jalv_vprintf };
	log_feature.data = &llog;

	zix_sem_init(&exit_sem, 0);
	jalv.done = &exit_sem;

	zix_sem_init(&jalv.paused, 0);
	zix_sem_init(&jalv.worker.sem, 0);

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* Find all installed plugins */
	LilvWorld* world = lilv_world_new();
	lilv_world_load_all(world);
	jalv.world = world;
	const LilvPlugins* plugins = lilv_world_get_all_plugins(world);

	/* Cache URIs for concepts we'll use */
	jalv.nodes.atom_AtomPort          = lilv_new_uri(world, LV2_ATOM__AtomPort);
	jalv.nodes.atom_Chunk             = lilv_new_uri(world, LV2_ATOM__Chunk);
	jalv.nodes.atom_Float             = lilv_new_uri(world, LV2_ATOM__Float);
	jalv.nodes.atom_Path              = lilv_new_uri(world, LV2_ATOM__Path);
	jalv.nodes.atom_Sequence          = lilv_new_uri(world, LV2_ATOM__Sequence);
	jalv.nodes.ev_EventPort           = lilv_new_uri(world, LV2_EVENT__EventPort);
	jalv.nodes.lv2_AudioPort          = lilv_new_uri(world, LV2_CORE__AudioPort);
	jalv.nodes.lv2_CVPort             = lilv_new_uri(world, LV2_CORE__CVPort);
	jalv.nodes.lv2_ControlPort        = lilv_new_uri(world, LV2_CORE__ControlPort);
	jalv.nodes.lv2_InputPort          = lilv_new_uri(world, LV2_CORE__InputPort);
	jalv.nodes.lv2_OutputPort         = lilv_new_uri(world, LV2_CORE__OutputPort);
	jalv.nodes.lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
	jalv.nodes.lv2_control            = lilv_new_uri(world, LV2_CORE__control);
	jalv.nodes.lv2_default            = lilv_new_uri(world, LV2_CORE__default);
	jalv.nodes.lv2_enumeration        = lilv_new_uri(world, LV2_CORE__enumeration);
	jalv.nodes.lv2_integer            = lilv_new_uri(world, LV2_CORE__integer);
	jalv.nodes.lv2_maximum            = lilv_new_uri(world, LV2_CORE__maximum);
	jalv.nodes.lv2_minimum            = lilv_new_uri(world, LV2_CORE__minimum);
	jalv.nodes.lv2_name               = lilv_new_uri(world, LV2_CORE__name);
	jalv.nodes.lv2_reportsLatency     = lilv_new_uri(world, LV2_CORE__reportsLatency);
	jalv.nodes.lv2_sampleRate         = lilv_new_uri(world, LV2_CORE__sampleRate);
	jalv.nodes.lv2_symbol             = lilv_new_uri(world, LV2_CORE__symbol);
	jalv.nodes.lv2_toggled            = lilv_new_uri(world, LV2_CORE__toggled);
	jalv.nodes.midi_MidiEvent         = lilv_new_uri(world, LV2_MIDI__MidiEvent);
	jalv.nodes.pg_group               = lilv_new_uri(world, LV2_PORT_GROUPS__group);
	jalv.nodes.pprops_logarithmic     = lilv_new_uri(world, LV2_PORT_PROPS__logarithmic);
	jalv.nodes.pprops_notOnGUI        = lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI);
	jalv.nodes.pprops_rangeSteps      = lilv_new_uri(world, LV2_PORT_PROPS__rangeSteps);
	jalv.nodes.pset_Preset            = lilv_new_uri(world, LV2_PRESETS__Preset);
	jalv.nodes.pset_bank              = lilv_new_uri(world, LV2_PRESETS__bank);
	jalv.nodes.rdfs_comment           = lilv_new_uri(world, LILV_NS_RDFS "comment");
	jalv.nodes.rdfs_label             = lilv_new_uri(world, LILV_NS_RDFS "label");
	jalv.nodes.rdfs_range             = lilv_new_uri(world, LILV_NS_RDFS "range");
	jalv.nodes.rsz_minimumSize        = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
	jalv.nodes.work_interface         = lilv_new_uri(world, LV2_WORKER__interface);
	jalv.nodes.work_schedule          = lilv_new_uri(world, LV2_WORKER__schedule);
	jalv.nodes.end                    = NULL;
}

