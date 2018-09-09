/*
 * Lv2Instrument.cpp - implementation of Lv2Instrument class
 *
 * Copyright (c) 2018 Alexandros Theodotou
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

#include <cmath>

#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include <lilv/lilv.h>

#include <suil/suil.h>

#include "control.h"
#include "lv2_evbuf.h"
#include "worker.h"
#include "log.h"

#include "Lv2Plugin.h"
#include "Lv2Manager.h"
#include "Plugin.h"
#include "embed.h"


/** Return true iff Jalv supports the given feature. */
bool
Lv2Plugin::feature_is_supported(const char* uri)
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

static LV2_URID
map_uri(LV2_URID_Map_Handle handle,
        const char*         uri)
{
	Lv2Plugin* p = (Lv2Plugin*)handle;
	zix_sem_wait(&p->symap_lock);
	const LV2_URID id = symap_map(p->symap, uri);
	zix_sem_post(&p->symap_lock);
	return id;
}


static const char*
unmap_uri(LV2_URID_Unmap_Handle handle,
          LV2_URID              urid)
{
	Lv2Plugin* p = (Lv2Plugin *)handle;
	zix_sem_wait(&p->symap_lock);
	const char* uri = symap_unmap(p->symap, urid);
	zix_sem_post(&p->symap_lock);
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
	Lv2Plugin* jalv = (Lv2Plugin*)callback_data;
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
create_port(Lv2Plugin*    jalv,
            uint32_t port_index,
            float    default_value)
{
	Port* const port = &jalv->ports[port_index];

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
Lv2Plugin::create_ports()
{
	num_ports = lilv_plugin_get_num_ports(plugin);
	ports     = (Port*)calloc(num_ports, sizeof(Port));
	float* default_values = (float*)calloc(
		lilv_plugin_get_num_ports(plugin), sizeof(float));
	lilv_plugin_get_port_ranges_float(plugin, NULL, NULL, default_values);

	for (uint32_t i = 0; i < num_ports; ++i) {
		create_port(this, i, default_values[i]);
	}

	const LilvPort* control_input = lilv_plugin_get_port_by_designation(
		plugin, nodes.lv2_InputPort, nodes.lv2_control);
	if (control_input) {
		control_in = lilv_port_get_index(plugin, control_input);
	}

	free(default_values);
}

/**
   Allocate port buffers (only necessary for MIDI).
*/
void
Lv2Plugin::allocate_port_buffers ()
{
	for (uint32_t i = 0; i < num_ports; ++i) {
		Port* const port     = &ports[i];
		switch (port->type) {
		case TYPE_EVENT: {
			lv2_evbuf_free(port->evbuf);
			const size_t buf_size = (port->buf_size > 0)
				? port->buf_size
				: midi_buf_size;
			port->evbuf = lv2_evbuf_new(
				buf_size,
				port->old_api ? LV2_EVBUF_EVENT : LV2_EVBUF_ATOM,
				map.map(map.handle,
				              lilv_node_as_string(nodes.atom_Chunk)),
				map.map(map.handle,
				              lilv_node_as_string(nodes.atom_Sequence)));
			lilv_instance_connect_port(
				instance, i, lv2_evbuf_get_buffer(port->evbuf));
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
Port*
Lv2Plugin::port_by_symbol(const char* sym)
{
	for (uint32_t i = 0; i < num_ports; ++i) {
		Port* const port     = &ports[i];
		const LilvNode*    port_sym = lilv_port_get_symbol(plugin,
		                                                   port->lilv_port);

		if (!strcmp(lilv_node_as_string(port_sym), sym)) {
			return port;
		}
	}

	return NULL;
}

ControlID*
Lv2Plugin::control_by_symbol (const char* sym)
{
	for (size_t i = 0; i < controls.n_controls; ++i) {
		if (!strcmp(lilv_node_as_string(controls.controls[i]->symbol),
		            sym)) {
			return controls.controls[i];
		}
	}
	return NULL;
}

void
Lv2Plugin::create_controls(bool writable)
{
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
			for (size_t i = 0; i < controls.n_controls; ++i) {
				if (lilv_node_equals(controls.controls[i]->node, property)) {
					record              = controls.controls[i];
					record->is_readable = true;
					break;
				}
			}

			if (record) {
				continue;
			}
		}

		record = new_property_control(this, property);
		if (writable) {
			record->is_writable = true;
		} else {
			record->is_readable = true;
		}

		if (record->value_type) {
			add_control(&controls, record);
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
Lv2Plugin::set_control(const ControlID* control,
                 uint32_t         size,
                 LV2_URID         type,
                 const void*      body)
{
	if (control->type == PORT && type == forge.Float) {
		Port* port = &ports[control->index];
		port->control = *(float*)body;
	} else if (control->type == PROPERTY) {
		// Copy forge since it is used by process thread
		LV2_Atom_Forge       forge = forge;
		LV2_Atom_Forge_Frame frame;
		uint8_t              buf[1024];
		lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));

		lv2_atom_forge_object(&forge, &frame, 0,
                          urids.patch_Set);
		lv2_atom_forge_key(&forge,
                       urids.patch_property);
		lv2_atom_forge_urid(&forge, control->property);
		lv2_atom_forge_key(&forge,
                       urids.patch_value);
		lv2_atom_forge_atom(&forge, size, type);
		lv2_atom_forge_write(&forge, body, size);

		const LV2_Atom* atom = lv2_atom_forge_deref(&forge, frame.ref);
		lv2_ui_write(this,
		              control_in,
		              lv2_atom_total_size(atom),
		              urids.atom_eventTransfer,
		              atom);
	}
}

void
Lv2Plugin::ui_instantiate(const char* native_ui_type, void* parent)
{
	ui_host = suil_host_new(lv2_ui_write, lv2_ui_port_index, NULL, NULL);

	const LV2_Feature parent_feature = {
		LV2_UI__parent, parent
	};
	const LV2_Feature instance_feature = {
		NS_EXT "instance-access", lilv_instance_get_handle(instance)
	};
	const LV2_Feature data_feature = {
		LV2_DATA_ACCESS_URI, &ext_data
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

	const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(ui));
	const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(ui));
	char*       bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
	char*       binary_path = lilv_file_uri_parse(binary_uri, NULL);

	ui_instance = suil_instance_new(
		ui_host,
		this,
		native_ui_type,
		lilv_node_as_uri(lilv_plugin_get_uri(plugin)),
		lilv_node_as_uri(lilv_ui_get_uri(ui)),
		lilv_node_as_uri(ui_type),
		bundle_path,
		binary_path,
		ui_features);

	lilv_free(binary_path);
	lilv_free(bundle_path);
}

bool
Lv2Plugin::ui_is_resizable()
{
	if (!ui) {
		return false;
	}

	LilvWorld* world = Lv2Manager::getInstance().world;

	const LilvNode* s   = lilv_ui_get_uri(ui);
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
lv2_ui_write(SuilController controller,
              uint32_t       port_index,
              uint32_t       buffer_size,
              uint32_t       protocol,
              const void*    buffer)
{
	Lv2Plugin* const plugin = (Lv2Plugin*)controller;
	if (protocol != 0 && protocol != plugin->urids.atom_eventTransfer) {
		fprintf(stderr, "UI write with unsupported protocol %d (%s)\n",
		        protocol, unmap_uri(plugin, protocol));
		return;
	}

	if (port_index >= plugin->num_ports) {
		fprintf(stderr, "UI write to out of range port index %d\n",
		        port_index);
		return;
	}

	if (protocol == plugin->urids.atom_eventTransfer) {
		const LV2_Atom* atom = (const LV2_Atom*)buffer;
		char*           str  = sratom_to_turtle(
			plugin->sratom, &plugin->unmap, "jalv:", NULL, NULL,
			atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));
		lv2_ansi_start(stdout, 36);
		printf("\n## UI => Plugin (%u bytes) ##\n%s\n", atom->size, str);
		lv2_ansi_reset(stdout);
		free(str);
	}

	char buf[sizeof(ControlChange) + buffer_size];
	ControlChange* ev = (ControlChange*)buf;
	ev->index    = port_index;
	ev->protocol = protocol;
	ev->size     = buffer_size;
	memcpy(ev->body, buffer, buffer_size);
	zix_ring_write(plugin->ui_events, buf, sizeof(buf));
}

void
Lv2Plugin::apply_ui_events(uint32_t nframes)
{
	if (!has_ui) {
		return;
	}
	ControlChange ev;
	const size_t  space = zix_ring_read_space(ui_events);
	for (size_t i = 0; i < space; i += sizeof(ev) + ev.size) {
		zix_ring_read(ui_events, (char*)&ev, sizeof(ev));
		char body[ev.size];
		if (zix_ring_read(ui_events, body, ev.size) != ev.size) {
			fprintf(stderr, "error: Error reading from UI ring buffer\n");
			break;
		}
		assert(ev.index < num_ports);
		Port* const port = &ports[ev.index];
		if (ev.protocol == 0) {
			assert(ev.size == sizeof(float));
			port->control = *(float*)body;
		} else if (ev.protocol == urids.atom_eventTransfer) {
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
lv2_ui_port_index(SuilController controller, const char* symbol)
{
	Lv2Plugin* const  jalv = (Lv2Plugin*)controller;
	Port* port = jalv->port_by_symbol(symbol);

	return port ? port->index : LV2UI_INVALID_PORT_INDEX;
}

void
Lv2Plugin::init_ui()
{
	// Set initial control port values
	for (uint32_t i = 0; i < num_ports; ++i) {
		if (ports[i].type == TYPE_CONTROL) {
			Lv2Plugin::ui_port_event(i,
			                   sizeof(float), 0,
			                   &ports[i].control);
		}
	}

	if (control_in != (uint32_t)-1) {
		// Send patch:Get message for initial parameters/etc
		LV2_Atom_Forge       forge = forge;
		LV2_Atom_Forge_Frame frame;
		uint8_t              buf[1024];
		lv2_atom_forge_set_buffer(&forge, buf, sizeof(buf));
		lv2_atom_forge_object(&forge, &frame, 0, urids.patch_Get);

		const LV2_Atom* atom = lv2_atom_forge_deref(&forge, frame.ref);
		lv2_ui_write(this,
		              control_in,
		              lv2_atom_total_size(atom),
		              urids.atom_eventTransfer,
		              atom);
		lv2_atom_forge_pop(&forge, &frame);
	}
}

bool
Lv2Plugin::send_to_ui(uint32_t    port_index,
                uint32_t    type,
                uint32_t    size,
                const void* body)
{
	/* TODO: Be more disciminate about what to send */
	char evbuf[sizeof(ControlChange) + sizeof(LV2_Atom)];
	ControlChange* ev = (ControlChange*)evbuf;
	ev->index    = port_index;
	ev->protocol = urids.atom_eventTransfer;
	ev->size     = sizeof(LV2_Atom) + size;

	LV2_Atom* atom = (LV2_Atom*)ev->body;
	atom->type = type;
	atom->size = size;

	if (zix_ring_write_space(plugin_events) >= sizeof(evbuf) + size) {
		zix_ring_write(plugin_events, evbuf, sizeof(evbuf));
		zix_ring_write(plugin_events, (const char*)body, size);
		return true;
	} else {
		fprintf(stderr, "Plugin => UI buffer overflow!\n");
		return false;
	}
}

bool
Lv2Plugin::run(uint32_t nframes)
{
	/* Read and apply control change events from UI */
	Lv2Plugin::apply_ui_events(nframes);

	/* Run plugin for this cycle */
	lilv_instance_run(instance, nframes);

	/* Process any worker replies. */
	lv2_worker_emit_responses(&state_worker, instance);
	lv2_worker_emit_responses(&worker, instance);

	/* Notify the plugin the run() cycle is finished */
	if (worker.iface && worker.iface->end_run) {
		worker.iface->end_run(instance->lv2_handle);
	}

	/* Check if it's time to send updates to the UI */
	event_delta_t += nframes;
	bool     send_ui_updates = false;
	uint32_t update_frames   = sample_rate / ui_update_hz;
	if (has_ui && (event_delta_t > update_frames)) {
		send_ui_updates = true;
		event_delta_t = 0;
	}

	return send_ui_updates;
}

bool
Lv2Plugin::update()
{
	/* Check quit flag and close if set. */
	if (zix_sem_try_wait(&exit_sem)) {
		Lv2Plugin::close_ui();
		return false;
	}
	/* Emit UI events. */
	ControlChange ev;
	const size_t  space = zix_ring_read_space(plugin_events);
	for (size_t i = 0;
	     i + sizeof(ev) < space;
	     i += sizeof(ev) + ev.size) {
		/* Read event header to get the size */
		zix_ring_read(plugin_events, (char*)&ev, sizeof(ev));

		/* Resize read buffer if necessary */
		ui_event_buf = realloc(ui_event_buf, ev.size);
		void* const buf = ui_event_buf;

		/* Read event body */
		zix_ring_read(plugin_events, (char*)buf, ev.size);

		if (ev.protocol == urids.atom_eventTransfer) {
			/* Dump event in Turtle to the console */
			LV2_Atom* atom = (LV2_Atom*)buf;
			char*     str  = sratom_to_turtle(
				ui_sratom, &unmap, "jalv:", NULL, NULL,
				atom->type, atom->size, LV2_ATOM_BODY(atom));
		  lv2_ansi_start(stdout, 35);
			printf("\n## Plugin => UI (%u bytes) ##\n%s\n", atom->size, str);
			lv2_ansi_reset(stdout);
			free(str);
		}

		if (ui_instance) {
			suil_instance_port_event(ui_instance, ev.index,
			                         ev.size, ev.protocol, buf);
		} else {
			Lv2Plugin::ui_port_event(ev.index, ev.size, ev.protocol, buf);
		}

		if (ev.protocol == 0) {
			print_control_value(&ports[ev.index], *(float*)buf);
		}
	}

	return true;
}

void
Lv2Plugin::print_control_value(const Port* port, float value)
{
	const LilvNode* sym = lilv_port_get_symbol(plugin, port->lilv_port);
	printf("%-*s = %f\n", longest_sym, lilv_node_as_string(sym), value);
}

char*
lv2_make_path(LV2_State_Make_Path_Handle handle,
               const char*                path)
{
	Lv2Plugin* jalv = (Lv2Plugin*)handle;

	// Create in save directory if saving, otherwise use temp directory
	return lv2_strjoin(jalv->save_dir ? jalv->save_dir : jalv->temp_dir, path);
}

/**
 * Instantiate the plugin.
 */
void
Lv2Plugin::instantiate()
{
	Lv2Manager* lv2Manager = &Lv2Manager::getInstance();
	LilvWorld* world = lv2Manager->world;

	/* TODO Set audio engine properties */
	//jalvPlugin.sample_rate   = jack_get_sample_rate(client);
	//jalvPlugin.block_length  = jack_get_buffer_size(client);
	midi_buf_size = 4096;

	/* Cache URIs for concepts we'll use */
	nodes.atom_AtomPort          = lilv_new_uri(world, LV2_ATOM__AtomPort);
	nodes.atom_Chunk             = lilv_new_uri(world, LV2_ATOM__Chunk);
	nodes.atom_Float             = lilv_new_uri(world, LV2_ATOM__Float);
	nodes.atom_Path              = lilv_new_uri(world, LV2_ATOM__Path);
	nodes.atom_Sequence          = lilv_new_uri(world, LV2_ATOM__Sequence);
	nodes.ev_EventPort           = lilv_new_uri(world, LV2_EVENT__EventPort);
	nodes.lv2_AudioPort          = lilv_new_uri(world, LV2_CORE__AudioPort);
	nodes.lv2_CVPort             = lilv_new_uri(world, LV2_CORE__CVPort);
	nodes.lv2_ControlPort        = lilv_new_uri(world, LV2_CORE__ControlPort);
	nodes.lv2_InputPort          = lilv_new_uri(world, LV2_CORE__InputPort);
	nodes.lv2_OutputPort         = lilv_new_uri(world, LV2_CORE__OutputPort);
	nodes.lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
	nodes.lv2_control            = lilv_new_uri(world, LV2_CORE__control);
	nodes.lv2_default            = lilv_new_uri(world, LV2_CORE__default);
	nodes.lv2_enumeration        = lilv_new_uri(world, LV2_CORE__enumeration);
	nodes.lv2_integer            = lilv_new_uri(world, LV2_CORE__integer);
	nodes.lv2_maximum            = lilv_new_uri(world, LV2_CORE__maximum);
	nodes.lv2_minimum            = lilv_new_uri(world, LV2_CORE__minimum);
	nodes.lv2_name               = lilv_new_uri(world, LV2_CORE__name);
	nodes.lv2_reportsLatency     = lilv_new_uri(world, LV2_CORE__reportsLatency);
	nodes.lv2_sampleRate         = lilv_new_uri(world, LV2_CORE__sampleRate);
	nodes.lv2_symbol             = lilv_new_uri(world, LV2_CORE__symbol);
	nodes.lv2_toggled            = lilv_new_uri(world, LV2_CORE__toggled);
	nodes.midi_MidiEvent         = lilv_new_uri(world, LV2_MIDI__MidiEvent);
	nodes.pg_group               = lilv_new_uri(world, LV2_PORT_GROUPS__group);
	nodes.pprops_logarithmic     = lilv_new_uri(world, LV2_PORT_PROPS__logarithmic);
	nodes.pprops_notOnGUI        = lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI);
	nodes.pprops_rangeSteps      = lilv_new_uri(world, LV2_PORT_PROPS__rangeSteps);
	nodes.pset_Preset            = lilv_new_uri(world, LV2_PRESETS__Preset);
	nodes.pset_bank              = lilv_new_uri(world, LV2_PRESETS__bank);
	nodes.rdfs_comment           = lilv_new_uri(world, LILV_NS_RDFS "comment");
	nodes.rdfs_range             = lilv_new_uri(world, LILV_NS_RDFS "range");
	nodes.rsz_minimumSize        = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
	nodes.work_interface         = lilv_new_uri(world, LV2_WORKER__interface);
	nodes.work_schedule          = lilv_new_uri(world, LV2_WORKER__schedule);
	nodes.end                    = NULL;

	bool use_generic_ui = false;
	LilvState* state = NULL;

	symap = symap_new();
	zix_sem_init(&symap_lock, 1);
	zix_sem_init(&work_lock, 1);
	uri_map_feature.data  = &uri_map;
	uri_map.callback_data = this;

	map.handle  = this;
	map.map     = map_uri;
	map_feature.data = &map;

	worker.plugin       = this;
	state_worker.plugin = this;

  unmap.handle  = this;
	unmap.unmap   = unmap_uri;
	unmap_feature.data = &unmap;

	lv2_atom_forge_init(&forge, &map);

	env = serd_env_new(NULL);
	serd_env_set_prefix_from_strings(
		env, (const uint8_t*)"patch", (const uint8_t*)LV2_PATCH_PREFIX);
	serd_env_set_prefix_from_strings(
		env, (const uint8_t*)"time", (const uint8_t*)LV2_TIME_PREFIX);
	serd_env_set_prefix_from_strings(
		env, (const uint8_t*)"xsd", (const uint8_t*)NS_XSD);

	sratom    = sratom_new(&map);
	ui_sratom = sratom_new(&map);
	sratom_set_env(sratom, env);
	sratom_set_env(ui_sratom, env);

	midi_event_id = uri_to_id(
		this, "http://lv2plug.in/ns/ext/event",
    LV2_MIDI__MidiEvent);

	urids.atom_Float           = symap_map(symap, LV2_ATOM__Float);
	urids.atom_Int             = symap_map(symap, LV2_ATOM__Int);
	urids.atom_Object          = symap_map(symap, LV2_ATOM__Object);
	urids.atom_Path            = symap_map(symap, LV2_ATOM__Path);
	urids.atom_String          = symap_map(symap, LV2_ATOM__String);
	urids.atom_eventTransfer   = symap_map(symap, LV2_ATOM__eventTransfer);
	urids.bufsz_maxBlockLength = symap_map(symap, LV2_BUF_SIZE__maxBlockLength);
	urids.bufsz_minBlockLength = symap_map(symap, LV2_BUF_SIZE__minBlockLength);
	urids.bufsz_sequenceSize   = symap_map(symap, LV2_BUF_SIZE__sequenceSize);
	urids.log_Error            = symap_map(symap, LV2_LOG__Error);
	urids.log_Trace            = symap_map(symap, LV2_LOG__Trace);
	urids.log_Warning          = symap_map(symap, LV2_LOG__Warning);
	urids.midi_MidiEvent       = symap_map(symap, LV2_MIDI__MidiEvent);
	urids.param_sampleRate     = symap_map(symap, LV2_PARAMETERS__sampleRate);
	urids.patch_Get            = symap_map(symap, LV2_PATCH__Get);
	urids.patch_Put            = symap_map(symap, LV2_PATCH__Put);
	urids.patch_Set            = symap_map(symap, LV2_PATCH__Set);
	urids.patch_body           = symap_map(symap, LV2_PATCH__body);
	urids.patch_property       = symap_map(symap, LV2_PATCH__property);
	urids.patch_value          = symap_map(symap, LV2_PATCH__value);
	urids.time_Position        = symap_map(symap, LV2_TIME__Position);
	urids.time_bar             = symap_map(symap, LV2_TIME__bar);
	urids.time_barBeat         = symap_map(symap, LV2_TIME__barBeat);
	urids.time_beatUnit        = symap_map(symap, LV2_TIME__beatUnit);
	urids.time_beatsPerBar     = symap_map(symap, LV2_TIME__beatsPerBar);
	urids.time_beatsPerMinute  = symap_map(symap, LV2_TIME__beatsPerMinute);
	urids.time_frame           = symap_map(symap, LV2_TIME__frame);
	urids.time_speed           = symap_map(symap, LV2_TIME__speed);
	urids.ui_updateRate        = symap_map(symap, LV2_UI__updateRate);

#ifdef _WIN32
	temp_dir = jalv_strdup("jalvXXXXXX");
	_mktemp(temp_dir);
#else
	char* templ = lv2_strdup("/tmp/jalv-XXXXXX");
	temp_dir = lv2_strjoin(mkdtemp(templ), "/");
	free(templ);
#endif

	LV2_State_Make_Path make_path = { this, lv2_make_path };
	make_path_feature.data = &make_path;

	LV2_Worker_Schedule sched = { &worker, lv2_worker_schedule };
	sched_feature.data = &sched;

	LV2_Worker_Schedule ssched = { &state_worker, lv2_worker_schedule };
	state_sched_feature.data = &ssched;

	LV2_Log_Log llog = { this, lv2_printf, lv2_vprintf };
	log_feature.data = &llog;

	zix_sem_init(&exit_sem, 0);
	done = &exit_sem;

	zix_sem_init(&paused, 0);
	zix_sem_init(&worker.sem, 0);

	midi_buf_size = 1024;  /* Should be set by backend */
	control_in    = (uint32_t)-1;

	/* Check that any required features are supported */
	LilvNodes* req_feats = lilv_plugin_get_required_features(plugin);
	LILV_FOREACH(nodes, f, req_feats) {
		const char* uri = lilv_node_as_uri(lilv_nodes_get(req_feats, f));
		if (!feature_is_supported(uri)) {
			fprintf(stderr, "Feature %s is not supported\n", uri);
			return;
		}
	}
	lilv_nodes_free(req_feats);

	/* Check for thread-safe state restore() method. */
	LilvNode* state_threadSafeRestore = lilv_new_uri(
		lv2Manager->world, LV2_STATE__threadSafeRestore);
	if (lilv_plugin_has_feature(plugin, state_threadSafeRestore)) {
		safe_restore = true;
	}
	lilv_node_free(state_threadSafeRestore);

	/* Get a plugin UI */
	const char* native_ui_type_uri = native_ui_type();
	uis = lilv_plugin_get_uis(plugin);
	if (!use_generic_ui && native_ui_type_uri) {
		const LilvNode* native_ui_type = lilv_new_uri(world, native_ui_type_uri);
		LILV_FOREACH(uis, u, uis) {
			const LilvUI* this_ui = lilv_uis_get(uis, u);
			if (lilv_ui_is_supported(this_ui,
			                         suil_ui_supported,
			                         native_ui_type,
			                         &ui_type)) {
				/* TODO: Multiple UI support */
				ui = this_ui;
				break;
			}
		}
	} else if (!use_generic_ui) {
		ui = lilv_uis_get(uis, lilv_uis_begin(uis));
	}

	/* Create ringbuffers for UI if necessary */
	if (ui) {
		fprintf(stderr, "UI:           %s\n",
		        lilv_node_as_uri(lilv_ui_get_uri(ui)));
	} else {
		fprintf(stderr, "UI:           None\n");
	}

	/* Create port and control structures */
	create_ports();
	create_controls(true);
	create_controls(false);

	//if (!(jalvPlugin->backend = jalv_backend_init(jalvPlugin))) {
		//fprintf(stderr, "Failed to connect to audio system");
	//}

	printf("Sample rate:  %u Hz\n", sample_rate);
	printf("Block length: %u frames\n", block_length);
	printf("MIDI buffers: %zu bytes\n", midi_buf_size);

  Lv2Manager & manager = Lv2Manager::getInstance ();
	if (manager.buffer_size == 0) {
		/* The UI ring is fed by plugin output ports (usually one), and the UI
		   updates roughly once per cycle.  The ring size is a few times the
		   size of the MIDI output to give the UI a chance to keep up.  The UI
		   should be able to keep up with 4 cycles, and tests show this works
		   for me, but this value might need increasing to avoid overflows.
		*/
		manager.buffer_size = midi_buf_size * N_BUFFER_CYCLES;
	}

	if (manager.update_rate == 0.0) {
		/* Calculate a reasonable UI update frequency. */
		ui_update_hz = (float)sample_rate / midi_buf_size * 2.0f;
		ui_update_hz = MAX(25.0f, ui_update_hz);
	} else {
		/* Use user-specified UI update rate. */
		ui_update_hz = manager.update_rate;
		ui_update_hz = MAX(1.0f, ui_update_hz);
	}

	/* The UI can only go so fast, clamp to reasonable limits */
	ui_update_hz     = MIN(60, ui_update_hz);
	manager.buffer_size = MAX(4096, manager.buffer_size);
	fprintf(stderr, "Comm buffers: %d bytes\n", manager.buffer_size);
	fprintf(stderr, "Update rate:  %.01f Hz\n", ui_update_hz);

	/* Build options array to pass to plugin */
	const LV2_Options_Option options[] = {
		{ LV2_OPTIONS_INSTANCE, 0, urids.param_sampleRate,
		  sizeof(float), urids.atom_Float, &sample_rate },
		{ LV2_OPTIONS_INSTANCE, 0, urids.bufsz_minBlockLength,
		  sizeof(int32_t), urids.atom_Int, &block_length },
		{ LV2_OPTIONS_INSTANCE, 0, urids.bufsz_maxBlockLength,
		  sizeof(int32_t), urids.atom_Int, &block_length },
		{ LV2_OPTIONS_INSTANCE, 0, urids.bufsz_sequenceSize,
		  sizeof(int32_t), urids.atom_Int, &midi_buf_size },
		{ LV2_OPTIONS_INSTANCE, 0, urids.ui_updateRate,
		  sizeof(float), urids.atom_Float, &ui_update_hz },
		{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
	};

	options_feature.data = (void*)&options;

	/* Create Plugin <=> UI communication buffers */
	ui_events     = zix_ring_new(manager.buffer_size);
	plugin_events = zix_ring_new(manager.buffer_size);
	zix_ring_mlock(ui_events);
	zix_ring_mlock(plugin_events);

	/* Instantiate the plugin */
	instance = lilv_plugin_instantiate(
		plugin, sample_rate, features);
	if (!instance) {
		fprintf(stderr, "Failed to instantiate plugin.\n");
	}

	ext_data.data_access = lilv_instance_get_descriptor(instance)->extension_data;

	fprintf(stderr, "\n");
	if (!buf_size_set) {
		allocate_port_buffers();
	}

	/* Create workers if necessary */
	if (lilv_plugin_has_extension_data(plugin, nodes.work_interface)) {
		const LV2_Worker_Interface* iface = (const LV2_Worker_Interface*)
			lilv_instance_get_extension_data(instance, LV2_WORKER__interface);

		lv2_worker_init(this, &worker, iface, true);
		if (safe_restore) {
			lv2_worker_init(this, &state_worker, iface, false);
		}
	}

	/* Apply loaded state to plugin instance if necessary */
	if (state) {
		apply_state(state);
	}

	/* Set Jack callbacks */
	//jalv_backend_init(&jalvPlugin);

	/* Create Jack ports and connect plugin ports to buffers */
	for (uint32_t i = 0; i < num_ports; ++i) {
		//jalv_backend_activate_port(&jalvPlugin, i);
	}

	/* Print initial control values */
	for (size_t i = 0; i < controls.n_controls; ++i) {
		ControlID* control = controls.controls[i];
		if (control->type == PORT) {// && control->value_type == forge.Float) {
			Port* port = &ports[control->index];
			print_control_value(port, port->control);
		}
	}

	/* Activate plugin */
	lilv_instance_activate(instance);

	/* Discover UI */
	has_ui = discover_ui();

	/* Activate Jack */
	//jalv_backend_activate(jalvPlugin);
	//play_state = JALV_RUNNING;

	/* Run UI (or prompt at console) */
	open_ui();
}

/**
 * Creates the Lv2 plugin from uri
 */
Lv2Plugin::Lv2Plugin (const QString & uri)
{
  Lv2Manager & manager = Lv2Manager::getInstance();
  LilvNode * lv2_uri = lilv_new_uri (manager.world,
                  uri.toUtf8().data());
  plugin = lilv_plugins_get_by_uri (
                                    manager.plugins,
                                    lv2_uri);

  if (!plugin)
    {
      qDebug ("Failed to get LV2 Plugin from URI %s",
              uri.toUtf8().data());
    }

}

