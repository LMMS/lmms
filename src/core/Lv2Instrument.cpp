/*
 * Lv2Instrument.cpp - implementation of Lv2Instrument class
 *
 * Copyright (c) 2018 Alexandros Theodotou @faiyadesu
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

#include "lv2/lv2plug.in/ns/ext/options/options.h"
extern "C"
{
#include "worker.h"
#include "symap.h"
}

#include <cmath>

#include "Engine.h"
//#include "JalvQt.h"
#include "Lv2Instrument.h"
#include "Lv2Manager.h"
#include "Mixer.h"
#include "Song.h"


#include "embed.h"

static PixmapLoader pml = new PixmapLoader( "lv2_plugin" );

extern "C"
{

Plugin::Descriptor lv2_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Lv2 Ins",
	QT_TRANSLATE_NOOP( "Lv2 Instrument",
				"An lv2 instrument plugin" ),
	"Alexandros Theodotou @faiyadesu",
	0x0100,
	Plugin::Instrument,
	&pml, // logo
	NULL
} ;
}

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"

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

static LV2_URID
map_uri(LV2_URID_Map_Handle handle,
        const char*         uri)
{
	JalvPlugin* jalv = (JalvPlugin*)handle;
	zix_sem_wait(&jalv->symap_lock);
	const LV2_URID id = symap_map(jalv->symap, uri);
	zix_sem_post(&jalv->symap_lock);
	return id;
}


static const char*
unmap_uri(LV2_URID_Unmap_Handle handle,
          LV2_URID              urid)
{
	JalvPlugin* jalv = (JalvPlugin*)handle;
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
	JalvPlugin* jalv = (JalvPlugin*)callback_data;
	zix_sem_wait(&jalv->symap_lock);
	const LV2_URID id = symap_map(jalv->symap, uri);
	zix_sem_post(&jalv->symap_lock);
	return id;
}

static LV2_URI_Map_Feature uri_map = { NULL, &uri_to_id };


/**
   Create a port structure from data description.  This is called before plugin
   and Jack instantiation.  The remaining instance-specific setup
   (e.g. buffers) is done later in activate_port().
*/
static void
create_port(JalvPlugin*    jalv,
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
		//("Mandatory port has unknown type (neither input nor output)");
	}

	/* Set control values */
	if (lilv_port_is_a(jalv->plugin, port->lilv_port, jalv->nodes.lv2_ControlPort)) {
		port->type    = TYPE_CONTROL;
		port->control = std::isnan(default_value) ? 0.0f : default_value;
		if (!lilv_port_has_property(jalv->plugin, port->lilv_port, jalv->nodes.pprops_notOnGUI)) {
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
		// TODO
		//die("Mandatory port has unknown data type");
	}

	LilvNode* min_size = lilv_port_get(
		jalv->plugin, port->lilv_port, jalv->nodes.rsz_minimumSize);
	if (min_size && lilv_node_is_int(min_size)) {
		port->buf_size = lilv_node_as_int(min_size);
		//jalv->opts.buffer_size = MAX(
			//jalv->opts.buffer_size, port->buf_size * N_BUFFER_CYCLES);
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
jalv_create_ports(JalvPlugin* jalv)
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
jalv_allocate_port_buffers(JalvPlugin* jalv)
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
jalv_port_by_symbol(JalvPlugin* jalv, const char* sym)
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
jalv_control_by_symbol(JalvPlugin* jalv, const char* sym)
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
Lv2Manager::print_control_value(JalvPlugin* jalv, const struct Port* port, float value)
{
	const LilvNode* sym = lilv_port_get_symbol(jalv->plugin, port->lilv_port);
	printf("%-*s = %f\n", jalv->longest_sym, lilv_node_as_string(sym), value);
}

void
jalv_create_controls(JalvPlugin* jalv, bool writable)
{
	const LilvPlugin* plugin         = jalv->plugin;
	LilvWorld*        world          = Lv2Manager::getInstance().world;
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
	JalvPlugin* jalv = control->jalv;
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
jalv_ui_instantiate(JalvPlugin* jalv, const char* native_ui_type, void* parent)
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
jalv_ui_is_resizable(JalvPlugin* jalv)
{
	if (!jalv->ui) {
		return false;
	}

	LilvWorld* world = Lv2Manager::getInstance().world;

	const LilvNode* s   = lilv_ui_get_uri(jalv->ui);
	LilvNode*       p   = lilv_new_uri(world, LV2_CORE__optionalFeature);
	LilvNode*       fs  = lilv_new_uri(world, LV2_UI__fixedSize);
	LilvNode*       nrs = lilv_new_uri(world, LV2_UI__noUserResize);

	LilvNodes* fs_matches = lilv_world_find_nodes(world, s, p, fs);
	LilvNodes* nrs_matches = lilv_world_find_nodes(world, s, p, nrs);

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
	JalvPlugin* const jalv = (JalvPlugin*)controller;

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

	if (protocol == jalv->urids.atom_eventTransfer) {
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
jalv_apply_ui_events(JalvPlugin* jalv, uint32_t nframes)
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
	JalvPlugin* const  jalv = (JalvPlugin*)controller;
	struct Port* port = jalv_port_by_symbol(jalv, symbol);

	return port ? port->index : LV2UI_INVALID_PORT_INDEX;
}

void
jalv_init_ui(JalvPlugin* jalv)
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
jalv_send_to_ui(JalvPlugin*       jalv,
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
jalv_run(JalvPlugin* jalv, uint32_t nframes)
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
jalv_update(JalvPlugin* jalv)
{
	/* Check quit flag and close if set. */
	if (zix_sem_try_wait(&jalv->exit_sem)) {
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

		if (ev.protocol == jalv->urids.atom_eventTransfer) {
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

		if (ev.protocol == 0) {
			Lv2Manager::print_control_value(jalv, &jalv->ports[ev.index], *(float*)buf);
		}
	}

	return true;
}
Lv2Instrument::Lv2Instrument(const LilvPlugin* plgn,
		InstrumentTrack * _it) :
	Instrument( _it, &lv2_plugin_descriptor ),
	m_plugin(plgn)
{
	Lv2Manager* lv2Manager = &Lv2Manager::getInstance();
	LilvWorld* world = lv2Manager->world;
	memset(&jalvPlugin, '\0', sizeof(JalvPlugin));
	jalvPlugin.world = world;

	/* TODO Set audio engine properties */
	//jalvPlugin.sample_rate   = jack_get_sample_rate(client);
	//jalvPlugin.block_length  = jack_get_buffer_size(client);
	jalvPlugin.midi_buf_size = 4096;

	/* Cache URIs for concepts we'll use */
	jalvPlugin.nodes.atom_AtomPort          = lilv_new_uri(world, LV2_ATOM__AtomPort);
	jalvPlugin.nodes.atom_Chunk             = lilv_new_uri(world, LV2_ATOM__Chunk);
	jalvPlugin.nodes.atom_Float             = lilv_new_uri(world, LV2_ATOM__Float);
	jalvPlugin.nodes.atom_Path              = lilv_new_uri(world, LV2_ATOM__Path);
	jalvPlugin.nodes.atom_Sequence          = lilv_new_uri(world, LV2_ATOM__Sequence);
	jalvPlugin.nodes.ev_EventPort           = lilv_new_uri(world, LV2_EVENT__EventPort);
	jalvPlugin.nodes.lv2_AudioPort          = lilv_new_uri(world, LV2_CORE__AudioPort);
	jalvPlugin.nodes.lv2_CVPort             = lilv_new_uri(world, LV2_CORE__CVPort);
	jalvPlugin.nodes.lv2_ControlPort        = lilv_new_uri(world, LV2_CORE__ControlPort);
	jalvPlugin.nodes.lv2_InputPort          = lilv_new_uri(world, LV2_CORE__InputPort);
	jalvPlugin.nodes.lv2_OutputPort         = lilv_new_uri(world, LV2_CORE__OutputPort);
	jalvPlugin.nodes.lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
	jalvPlugin.nodes.lv2_control            = lilv_new_uri(world, LV2_CORE__control);
	jalvPlugin.nodes.lv2_default            = lilv_new_uri(world, LV2_CORE__default);
	jalvPlugin.nodes.lv2_enumeration        = lilv_new_uri(world, LV2_CORE__enumeration);
	jalvPlugin.nodes.lv2_integer            = lilv_new_uri(world, LV2_CORE__integer);
	jalvPlugin.nodes.lv2_maximum            = lilv_new_uri(world, LV2_CORE__maximum);
	jalvPlugin.nodes.lv2_minimum            = lilv_new_uri(world, LV2_CORE__minimum);
	jalvPlugin.nodes.lv2_name               = lilv_new_uri(world, LV2_CORE__name);
	jalvPlugin.nodes.lv2_reportsLatency     = lilv_new_uri(world, LV2_CORE__reportsLatency);
	jalvPlugin.nodes.lv2_sampleRate         = lilv_new_uri(world, LV2_CORE__sampleRate);
	jalvPlugin.nodes.lv2_symbol             = lilv_new_uri(world, LV2_CORE__symbol);
	jalvPlugin.nodes.lv2_toggled            = lilv_new_uri(world, LV2_CORE__toggled);
	jalvPlugin.nodes.midi_MidiEvent         = lilv_new_uri(world, LV2_MIDI__MidiEvent);
	jalvPlugin.nodes.pg_group               = lilv_new_uri(world, LV2_PORT_GROUPS__group);
	jalvPlugin.nodes.pprops_logarithmic     = lilv_new_uri(world, LV2_PORT_PROPS__logarithmic);
	jalvPlugin.nodes.pprops_notOnGUI        = lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI);
	jalvPlugin.nodes.pprops_rangeSteps      = lilv_new_uri(world, LV2_PORT_PROPS__rangeSteps);
	jalvPlugin.nodes.pset_Preset            = lilv_new_uri(world, LV2_PRESETS__Preset);
	jalvPlugin.nodes.pset_bank              = lilv_new_uri(world, LV2_PRESETS__bank);
	jalvPlugin.nodes.rdfs_comment           = lilv_new_uri(world, LILV_NS_RDFS "comment");
	jalvPlugin.nodes.rdfs_range             = lilv_new_uri(world, LILV_NS_RDFS "range");
	jalvPlugin.nodes.rsz_minimumSize        = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
	jalvPlugin.nodes.work_interface         = lilv_new_uri(world, LV2_WORKER__interface);
	jalvPlugin.nodes.work_schedule          = lilv_new_uri(world, LV2_WORKER__schedule);
	jalvPlugin.nodes.end                    = NULL;

	bool use_generic_ui = false;
	LilvState* state = NULL;
	jalvPlugin.plugin = plgn;

	jalvPlugin.symap = symap_new();
	zix_sem_init(&jalvPlugin.symap_lock, 1);
	zix_sem_init(&jalvPlugin.work_lock, 1);
	uri_map_feature.data  = &uri_map;
	uri_map.callback_data = &jalvPlugin;

	jalvPlugin.map.handle  = &jalvPlugin;
	jalvPlugin.map.map     = map_uri;
	map_feature.data = &jalvPlugin.map;

	jalvPlugin.worker.jalv       = &jalvPlugin;
	jalvPlugin.state_worker.jalv = &jalvPlugin;

	jalvPlugin.unmap.handle  = &jalvPlugin;
	jalvPlugin.unmap.unmap   = unmap_uri;
	unmap_feature.data = &jalvPlugin.unmap;

	lv2_atom_forge_init(&jalvPlugin.forge, &jalvPlugin.map);

	jalvPlugin.env = serd_env_new(NULL);
	serd_env_set_prefix_from_strings(
		jalvPlugin.env, (const uint8_t*)"patch", (const uint8_t*)LV2_PATCH_PREFIX);
	serd_env_set_prefix_from_strings(
		jalvPlugin.env, (const uint8_t*)"time", (const uint8_t*)LV2_TIME_PREFIX);
	serd_env_set_prefix_from_strings(
		jalvPlugin.env, (const uint8_t*)"xsd", (const uint8_t*)NS_XSD);

	jalvPlugin.sratom    = sratom_new(&jalvPlugin.map);
	jalvPlugin.ui_sratom = sratom_new(&jalvPlugin.map);
	sratom_set_env(jalvPlugin.sratom, jalvPlugin.env);
	sratom_set_env(jalvPlugin.ui_sratom, jalvPlugin.env);

	jalvPlugin.midi_event_id = uri_to_id(
		&jalvPlugin, "http://lv2plug.in/ns/ext/event", LV2_MIDI__MidiEvent);

	jalvPlugin.urids.atom_Float           = symap_map(jalvPlugin.symap, LV2_ATOM__Float);
	jalvPlugin.urids.atom_Int             = symap_map(jalvPlugin.symap, LV2_ATOM__Int);
	jalvPlugin.urids.atom_Object          = symap_map(jalvPlugin.symap, LV2_ATOM__Object);
	jalvPlugin.urids.atom_Path            = symap_map(jalvPlugin.symap, LV2_ATOM__Path);
	jalvPlugin.urids.atom_String          = symap_map(jalvPlugin.symap, LV2_ATOM__String);
	jalvPlugin.urids.atom_eventTransfer   = symap_map(jalvPlugin.symap, LV2_ATOM__eventTransfer);
	jalvPlugin.urids.bufsz_maxBlockLength = symap_map(jalvPlugin.symap, LV2_BUF_SIZE__maxBlockLength);
	jalvPlugin.urids.bufsz_minBlockLength = symap_map(jalvPlugin.symap, LV2_BUF_SIZE__minBlockLength);
	jalvPlugin.urids.bufsz_sequenceSize   = symap_map(jalvPlugin.symap, LV2_BUF_SIZE__sequenceSize);
	jalvPlugin.urids.log_Error            = symap_map(jalvPlugin.symap, LV2_LOG__Error);
	jalvPlugin.urids.log_Trace            = symap_map(jalvPlugin.symap, LV2_LOG__Trace);
	jalvPlugin.urids.log_Warning          = symap_map(jalvPlugin.symap, LV2_LOG__Warning);
	jalvPlugin.urids.midi_MidiEvent       = symap_map(jalvPlugin.symap, LV2_MIDI__MidiEvent);
	jalvPlugin.urids.param_sampleRate     = symap_map(jalvPlugin.symap, LV2_PARAMETERS__sampleRate);
	jalvPlugin.urids.patch_Get            = symap_map(jalvPlugin.symap, LV2_PATCH__Get);
	jalvPlugin.urids.patch_Put            = symap_map(jalvPlugin.symap, LV2_PATCH__Put);
	jalvPlugin.urids.patch_Set            = symap_map(jalvPlugin.symap, LV2_PATCH__Set);
	jalvPlugin.urids.patch_body           = symap_map(jalvPlugin.symap, LV2_PATCH__body);
	jalvPlugin.urids.patch_property       = symap_map(jalvPlugin.symap, LV2_PATCH__property);
	jalvPlugin.urids.patch_value          = symap_map(jalvPlugin.symap, LV2_PATCH__value);
	jalvPlugin.urids.time_Position        = symap_map(jalvPlugin.symap, LV2_TIME__Position);
	jalvPlugin.urids.time_bar             = symap_map(jalvPlugin.symap, LV2_TIME__bar);
	jalvPlugin.urids.time_barBeat         = symap_map(jalvPlugin.symap, LV2_TIME__barBeat);
	jalvPlugin.urids.time_beatUnit        = symap_map(jalvPlugin.symap, LV2_TIME__beatUnit);
	jalvPlugin.urids.time_beatsPerBar     = symap_map(jalvPlugin.symap, LV2_TIME__beatsPerBar);
	jalvPlugin.urids.time_beatsPerMinute  = symap_map(jalvPlugin.symap, LV2_TIME__beatsPerMinute);
	jalvPlugin.urids.time_frame           = symap_map(jalvPlugin.symap, LV2_TIME__frame);
	jalvPlugin.urids.time_speed           = symap_map(jalvPlugin.symap, LV2_TIME__speed);
	jalvPlugin.urids.ui_updateRate        = symap_map(jalvPlugin.symap, LV2_UI__updateRate);

#ifdef _WIN32
	jalvPlugin.temp_dir = jalv_strdup("jalvXXXXXX");
	_mktemp(jalvPlugin.temp_dir);
#else
	char* templ = jalv_strdup("/tmp/jalv-XXXXXX");
	jalvPlugin.temp_dir = jalv_strjoin(mkdtemp(templ), "/");
	free(templ);
#endif

	LV2_State_Make_Path make_path = { &jalvPlugin, jalv_make_path };
	make_path_feature.data = &make_path;

	LV2_Worker_Schedule sched = { &jalvPlugin.worker, jalv_worker_schedule };
	sched_feature.data = &sched;

	LV2_Worker_Schedule ssched = { &jalvPlugin.state_worker, jalv_worker_schedule };
	state_sched_feature.data = &ssched;

	LV2_Log_Log llog = { &jalvPlugin, jalv_printf, jalv_vprintf };
	log_feature.data = &llog;

	zix_sem_init(&jalvPlugin.exit_sem, 0);
	jalvPlugin.done = &jalvPlugin.exit_sem;

	zix_sem_init(&jalvPlugin.paused, 0);
	zix_sem_init(&jalvPlugin.worker.sem, 0);

	jalvPlugin.midi_buf_size = 1024;  /* Should be set by backend */
	jalvPlugin.control_in    = (uint32_t)-1;

	/* Check that any required features are supported */
	LilvNodes* req_feats = lilv_plugin_get_required_features(plgn);
	LILV_FOREACH(nodes, f, req_feats) {
		const char* uri = lilv_node_as_uri(lilv_nodes_get(req_feats, f));
		if (!Lv2Manager::getInstance().feature_is_supported(uri)) {
			fprintf(stderr, "Feature %s is not supported\n", uri);
			return;
		}
	}
	lilv_nodes_free(req_feats);

	/* Check for thread-safe state restore() method. */
	LilvNode* state_threadSafeRestore = lilv_new_uri(
		lv2Manager->world, LV2_STATE__threadSafeRestore);
	if (lilv_plugin_has_feature(plgn, state_threadSafeRestore)) {
		jalvPlugin.safe_restore = true;
	}
	lilv_node_free(state_threadSafeRestore);

	/* Get a plugin UI */
	const char* native_ui_type_uri = jalv_native_ui_type(&jalvPlugin);
	jalvPlugin.uis = lilv_plugin_get_uis(plgn);
	if (!use_generic_ui && native_ui_type_uri) {
		const LilvNode* native_ui_type = lilv_new_uri(jalvPlugin.world, native_ui_type_uri);
		LILV_FOREACH(uis, u, jalvPlugin.uis) {
			const LilvUI* this_ui = lilv_uis_get(jalvPlugin.uis, u);
			if (lilv_ui_is_supported(this_ui,
			                         suil_ui_supported,
			                         native_ui_type,
			                         &jalvPlugin.ui_type)) {
				/* TODO: Multiple UI support */
				jalvPlugin.ui = this_ui;
				break;
			}
		}
	} else if (!use_generic_ui) {
		jalvPlugin.ui = lilv_uis_get(jalvPlugin.uis, lilv_uis_begin(jalvPlugin.uis));
	}

	/* Create ringbuffers for UI if necessary */
	if (jalvPlugin.ui) {
		fprintf(stderr, "UI:           %s\n",
		        lilv_node_as_uri(lilv_ui_get_uri(jalvPlugin.ui)));
	} else {
		fprintf(stderr, "UI:           None\n");
	}

	/* Create port and control structures */
	jalv_create_ports(&jalvPlugin);
	jalv_create_controls(&jalvPlugin, true);
	jalv_create_controls(&jalvPlugin, false);

	//if (!(jalvPlugin->backend = jalv_backend_init(jalvPlugin))) {
		//fprintf(stderr, "Failed to connect to audio system");
	//}

	printf("Sample rate:  %u Hz\n", jalvPlugin.sample_rate);
	printf("Block length: %u frames\n", jalvPlugin.block_length);
	printf("MIDI buffers: %zu bytes\n", jalvPlugin.midi_buf_size);

	if (jalvPlugin.opts.buffer_size == 0) {
		/* The UI ring is fed by plugin output ports (usually one), and the UI
		   updates roughly once per cycle.  The ring size is a few times the
		   size of the MIDI output to give the UI a chance to keep up.  The UI
		   should be able to keep up with 4 cycles, and tests show this works
		   for me, but this value might need increasing to avoid overflows.
		*/
		jalvPlugin.opts.buffer_size = jalvPlugin.midi_buf_size * N_BUFFER_CYCLES;
	}

	if (jalvPlugin.opts.update_rate == 0.0) {
		/* Calculate a reasonable UI update frequency. */
		jalvPlugin.ui_update_hz = (float)jalvPlugin.sample_rate / jalvPlugin.midi_buf_size * 2.0f;
		jalvPlugin.ui_update_hz = MAX(25.0f, jalvPlugin.ui_update_hz);
	} else {
		/* Use user-specified UI update rate. */
		jalvPlugin.ui_update_hz = jalvPlugin.opts.update_rate;
		jalvPlugin.ui_update_hz = MAX(1.0f, jalvPlugin.ui_update_hz);
	}

	/* The UI can only go so fast, clamp to reasonable limits */
	jalvPlugin.ui_update_hz     = MIN(60, jalvPlugin.ui_update_hz);
	jalvPlugin.opts.buffer_size = MAX(4096, jalvPlugin.opts.buffer_size);
	fprintf(stderr, "Comm buffers: %d bytes\n", jalvPlugin.opts.buffer_size);
	fprintf(stderr, "Update rate:  %.01f Hz\n", jalvPlugin.ui_update_hz);

	/* Build options array to pass to plugin */
	const LV2_Options_Option options[] = {
		{ LV2_OPTIONS_INSTANCE, 0, jalvPlugin.urids.param_sampleRate,
		  sizeof(float), jalvPlugin.urids.atom_Float, &jalvPlugin.sample_rate },
		{ LV2_OPTIONS_INSTANCE, 0, jalvPlugin.urids.bufsz_minBlockLength,
		  sizeof(int32_t), jalvPlugin.urids.atom_Int, &jalvPlugin.block_length },
		{ LV2_OPTIONS_INSTANCE, 0, jalvPlugin.urids.bufsz_maxBlockLength,
		  sizeof(int32_t), jalvPlugin.urids.atom_Int, &jalvPlugin.block_length },
		{ LV2_OPTIONS_INSTANCE, 0, jalvPlugin.urids.bufsz_sequenceSize,
		  sizeof(int32_t), jalvPlugin.urids.atom_Int, &jalvPlugin.midi_buf_size },
		{ LV2_OPTIONS_INSTANCE, 0, jalvPlugin.urids.ui_updateRate,
		  sizeof(float), jalvPlugin.urids.atom_Float, &jalvPlugin.ui_update_hz },
		{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
	};

	extern LV2_Feature options_feature;
	options_feature.data = (void*)&options;

	/* Create Plugin <=> UI communication buffers */
	jalvPlugin.ui_events     = zix_ring_new(jalvPlugin.opts.buffer_size);
	jalvPlugin.plugin_events = zix_ring_new(jalvPlugin.opts.buffer_size);
	zix_ring_mlock(jalvPlugin.ui_events);
	zix_ring_mlock(jalvPlugin.plugin_events);

	/* Instantiate the plugin */
	jalvPlugin.instance = lilv_plugin_instantiate(
		plgn, jalvPlugin.sample_rate, Lv2Manager::features);
	if (!jalvPlugin.instance) {
		fprintf(stderr, "Failed to instantiate plugin.\n");
	}

	Lv2Manager::ext_data.data_access = lilv_instance_get_descriptor(jalvPlugin.instance)->extension_data;

	fprintf(stderr, "\n");
	if (!jalvPlugin.buf_size_set) {
		jalv_allocate_port_buffers(&jalvPlugin);
	}

	/* Create workers if necessary */
	if (lilv_plugin_has_extension_data(plgn, jalvPlugin.nodes.work_interface)) {
		const LV2_Worker_Interface* iface = (const LV2_Worker_Interface*)
			lilv_instance_get_extension_data(jalvPlugin.instance, LV2_WORKER__interface);

		jalv_worker_init(&jalvPlugin, &jalvPlugin.worker, iface, true);
		if (jalvPlugin.safe_restore) {
			jalv_worker_init(&jalvPlugin, &jalvPlugin.state_worker, iface, false);
		}
	}

	/* Apply loaded state to plugin instance if necessary */
	if (state) {
		jalv_apply_state(&jalvPlugin, state);
	}

	/* Set Jack callbacks */
	//jalv_backend_init(&jalvPlugin);

	/* Create Jack ports and connect plugin ports to buffers */
	for (uint32_t i = 0; i < jalvPlugin.num_ports; ++i) {
		//jalv_backend_activate_port(&jalvPlugin, i);
	}

	/* Print initial control values */
	for (size_t i = 0; i < jalvPlugin.controls.n_controls; ++i) {
		ControlID* control = jalvPlugin.controls.controls[i];
		if (control->type == PORT) {// && control->value_type == jalvPlugin.forge.Float) {
			struct Port* port = &jalvPlugin.ports[control->index];
			Lv2Manager::print_control_value(&jalvPlugin, port, port->control);
		}
	}

	/* Activate plugin */
	lilv_instance_activate(jalvPlugin.instance);

	/* Discover UI */
	jalvPlugin.has_ui = jalv_discover_ui(&jalvPlugin);

	/* Activate Jack */
	//jalv_backend_activate(jalvPlugin);
	jalvPlugin.play_state = JALV_RUNNING;

	/* Run UI (or prompt at console) */
	jalv_open_ui(&jalvPlugin);

	// update descriptor
	lv2_plugin_descriptor.displayName =
		lilv_node_as_string(lilv_plugin_get_name(plgn));
}

Lv2Instrument::~Lv2Instrument()
{

}

void Lv2Instrument::playNote( NotePlayHandle * _note_to_play/* _note_to_play */,
				sampleFrame * _working_buf/* _working_buf */ )
{
		memset( _working_buf, 0, sizeof( sampleFrame ) *
			Engine::mixer()->framesPerPeriod() );
	const fpp_t frames = _note_to_play->framesLeftForCurrentPeriod();
	const f_cnt_t offset = _note_to_play->noteOffset();

	Song* song = Engine::getSong();

	if( _note_to_play->totalFramesPlayed() == 0 || _note_to_play->m_pluginData == NULL )
	{

	/* Get Jack transport position */
	const bool rolling = song->isPlaying();

	/* If transport state is not as expected, then something has changed */
	f_cnt_t frame = song->getFrames();
	bpm_t beats_per_minute = song->getTempo();
	const bool xport_changed = (rolling != jalvPlugin.rolling ||
								frame != jalvPlugin.position ||
								beats_per_minute != jalvPlugin.bpm);

	int beat = song->getBeat();
	int tick = song->getTicks();

	uint8_t   pos_buf[256];
	LV2_Atom* lv2_pos = (LV2_Atom*)pos_buf;
	if (xport_changed) {
		/* Build an LV2 position object to report change to plugin */
		lv2_atom_forge_set_buffer(&jalvPlugin.forge, pos_buf, sizeof(pos_buf));
		LV2_Atom_Forge*      forge = &jalvPlugin.forge;
		LV2_Atom_Forge_Frame frame;
		lv2_atom_forge_object(forge, &frame, 0, jalvPlugin.urids.time_Position);
		lv2_atom_forge_key(forge, jalvPlugin.urids.time_frame);
		//lv2_atom_forge_long(forge, frame);
		lv2_atom_forge_key(forge, jalvPlugin.urids.time_speed);
		lv2_atom_forge_float(forge, rolling ? 1.0 : 0.0);
		//if (pos.valid & JackPositionBBT) {
			lv2_atom_forge_key(forge, jalvPlugin.urids.time_barBeat);
			MeterModel* timeSig = &song->getTimeSigModel();
			lv2_atom_forge_float(
				forge, beat - 1 + (tick /
					(song->ticksPerTact() / timeSig->numeratorModel().value())));
			// FIXME 4 = beats per bar ^
			lv2_atom_forge_key(forge, jalvPlugin.urids.time_bar);
			lv2_atom_forge_long(forge, song->getTacts() - 1);
			lv2_atom_forge_key(forge, jalvPlugin.urids.time_beatUnit);
			//lv2_atom_forge_int(forge, pos.beat_type);
			lv2_atom_forge_key(forge, jalvPlugin.urids.time_beatsPerBar);
			//lv2_atom_forge_float(forge, song->beatspos.beats_per_bar);
			lv2_atom_forge_key(forge, jalvPlugin.urids.time_beatsPerMinute);
			lv2_atom_forge_float(forge, beats_per_minute);
		//}

		char* str = sratom_to_turtle(
			jalvPlugin.sratom, &jalvPlugin.unmap, "time:", NULL, NULL,
			lv2_pos->type, lv2_pos->size, LV2_ATOM_BODY(lv2_pos));
		jalv_ansi_start(stdout, 36);
		printf("\n## Position ##\n%s\n", str);
		jalv_ansi_reset(stdout);
		free(str);
	}

	/* Update transport state to expected values for next cycle */
	jalvPlugin.position = rolling ? frame + frames : frame;
	jalvPlugin.bpm      = beats_per_minute;
	jalvPlugin.rolling  = rolling;

	switch (jalvPlugin.play_state) {
	case JALV_PAUSE_REQUESTED:
		jalvPlugin.play_state = JALV_PAUSED;
		zix_sem_post(&jalvPlugin.paused);
		break;
	case JALV_PAUSED:
		for (uint32_t p = 0; p < jalvPlugin.num_ports; ++p) {
			//jack_port_t* jport = jalvPlugin.ports[p].sys_port;
			//if (jport && jalvPlugin.ports[p].flow == FLOW_OUTPUT) {
				//void* buf = jack_port_get_buffer(jport, nframes);
				//if (jalvPlugin.ports[p].type == TYPE_EVENT) {
					//jack_midi_clear_buffer(buf);
				//} else {
					//memset(buf, '\0', frames * sizeof(float));
				//}
			//}
		}
		//return 0;
	default:
		break;
	}

	/* Prepare port buffers */
	for (uint32_t p = 0; p < jalvPlugin.num_ports; ++p) {
		struct Port* port = &jalvPlugin.ports[p];
		if (port->type == TYPE_AUDIO && port->sys_port) {
			/* Connect plugin port directly to Jack port buffer */
			//lilv_instance_connect_port(
				//jalvPlugin.instance, p,
				//jack_port_get_buffer(port->sys_port, frames));
		} else if (port->type == TYPE_EVENT && port->flow == FLOW_INPUT) {
			lv2_evbuf_reset(port->evbuf, true);

			/* Write transport change event if applicable */
			LV2_Evbuf_Iterator iter = lv2_evbuf_begin(port->evbuf);
			if (xport_changed) {
				lv2_evbuf_write(&iter, 0, 0,
				                lv2_pos->type, lv2_pos->size,
				                (const uint8_t*)LV2_ATOM_BODY(lv2_pos));
			}

			if (jalvPlugin.request_update) {
				/* Plugin state has changed, request an update */
				const LV2_Atom_Object get = {
					{ sizeof(LV2_Atom_Object_Body), jalvPlugin.urids.atom_Object },
					{ 0, jalvPlugin.urids.patch_Get } };
				lv2_evbuf_write(&iter, 0, 0,
				                get.atom.type, get.atom.size,
				                (const uint8_t*)LV2_ATOM_BODY(&get));
			}

			if (port->sys_port) {
				/* Write Jack MIDI input */
				//void* buf = jack_port_get_buffer(port->sys_port, frames);
				//for (uint32_t i = 0; i < jack_midi_get_event_count(buf); ++i) {
					//jack_midi_event_t ev;
					//jack_midi_event_get(&ev, buf, i);
					//lv2_evbuf_write(&iter,
									//ev.time, 0,
									//jalvPlugin.midi_event_id,
									//ev.size, ev.buffer);
				//}
			}
		} else if (port->type == TYPE_EVENT) {
			/* Clear event output for plugin to write to */
			lv2_evbuf_reset(port->evbuf, false);
		}
	}
	jalvPlugin.request_update = false;

	/* Run plugin for this cycle */
	const bool send_ui_updates = jalv_run(&jalvPlugin, frames);

	/* Deliver MIDI output and UI events */
	for (uint32_t p = 0; p < jalvPlugin.num_ports; ++p) {
		struct Port* const port = &jalvPlugin.ports[p];
		if (port->flow == FLOW_OUTPUT && port->type == TYPE_CONTROL &&
		    lilv_port_has_property(jalvPlugin.plugin, port->lilv_port,
		                           jalvPlugin.nodes.lv2_reportsLatency)) {
			if (jalvPlugin.plugin_latency != port->control) {
				jalvPlugin.plugin_latency = port->control;
				//jack_recompute_total_latencies(client);
			}
		} else if (port->flow == FLOW_OUTPUT && port->type == TYPE_EVENT) {
			void* buf = NULL;
			if (port->sys_port) {
				//buf = jack_port_get_buffer(port->sys_port, frames);
				//jack_midi_clear_buffer(buf);
			}

			for (LV2_Evbuf_Iterator i = lv2_evbuf_begin(port->evbuf);
			     lv2_evbuf_is_valid(i);
			     i = lv2_evbuf_next(i)) {
				// Get event from LV2 buffer
				uint32_t frames, subframes, type, size;
				uint8_t* body;
				lv2_evbuf_get(i, &frames, &subframes, &type, &size, &body);

				if (buf && type == jalvPlugin.midi_event_id) {
					// Write MIDI event to Jack output
					//jack_midi_event_write(buf, frames, body, size);
				}

				if (jalvPlugin.has_ui && !port->old_api) {
					// Forward event to UI
					jalv_send_to_ui(&jalvPlugin, p, type, size, body);
				}
			}
		} else if (send_ui_updates &&
		           port->flow == FLOW_OUTPUT && port->type == TYPE_CONTROL) {
			char buf[sizeof(ControlChange) + sizeof(float)];
			ControlChange* ev = (ControlChange*)buf;
			ev->index    = p;
			ev->protocol = 0;
			ev->size     = sizeof(float);
			*(float*)ev->body = port->control;
			if (zix_ring_write(jalvPlugin.plugin_events, buf, sizeof(buf))
			    < sizeof(buf)) {
				fprintf(stderr, "Plugin => UI buffer overflow!\n");
			}
		}
	}

	}
	instrumentTrack()->processAudioBuffer( _working_buf, frames + offset, _note_to_play );

}

void Lv2Instrument::deleteNotePluginData( NotePlayHandle * _note_to_play )
{
	// TODO
}


void Lv2Instrument::loadSettings(const QDomElement& _this)
{

}

QString Lv2Instrument::nodeName() const
{
	return lilv_node_as_string(lilv_plugin_get_name(m_plugin));
}

void Lv2Instrument::saveSettings(QDomDocument& _doc, QDomElement& _parent)
{

}

PluginView* Lv2Instrument::instantiateView( QWidget * _parent)
{
	return new Lv2InstrumentView(this, _parent);
}

Lv2InstrumentView::Lv2InstrumentView(Instrument * _instrument,
		QWidget * _parent) :
	InstrumentView( _instrument, _parent )
{
	//TODO draw UI
}

Lv2InstrumentView::~Lv2InstrumentView()
{

}

/*
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "jalv_internal.h"
#include "worker.h"

