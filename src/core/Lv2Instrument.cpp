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

#include "Engine.h"
//#include "JalvQt.h"
#include "Lv2Instrument.h"
#include "Lv2Manager.h"
#include "Mixer.h"

extern "C"
{
#include "worker.h"
}

#include "embed.h"

static PixmapLoader dummyLoader;

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
	&dummyLoader, // logo
	NULL
} ;
}

Lv2Instrument::Lv2Instrument(const LilvPlugin* plgn,
		InstrumentTrack * _it) :
	Instrument( _it, &lv2_plugin_descriptor ),
	m_plugin(plgn)
{
	Jalv* jalv = &Lv2Manager::getInstance().jalv;
	bool use_generic_ui = false;
	LilvState* state = NULL;
	jalv->plugin = plgn;

	/* Check that any required features are supported */
	LilvNodes* req_feats = lilv_plugin_get_required_features(plgn);
	LILV_FOREACH(nodes, f, req_feats) {
		const char* uri = lilv_node_as_uri(lilv_nodes_get(req_feats, f));
		if (!Lv2Manager::getInstance().feature_is_supported(uri)) {
			fprintf(stderr, "Feature %s is not supported\n", uri);
		}
	}
	lilv_nodes_free(req_feats);

	/* Check for thread-safe state restore() method. */
	LilvNode* state_threadSafeRestore = lilv_new_uri(
		jalv->world, LV2_STATE__threadSafeRestore);
	if (lilv_plugin_has_feature(plgn, state_threadSafeRestore)) {
		jalv->safe_restore = true;
	}
	lilv_node_free(state_threadSafeRestore);

	/* Get a plugin UI */
	const char* native_ui_type_uri = jalv_native_ui_type(jalv);
	jalv->uis = lilv_plugin_get_uis(plgn);
	if (!use_generic_ui && native_ui_type_uri) {
		const LilvNode* native_ui_type = lilv_new_uri(jalv->world, native_ui_type_uri);
		LILV_FOREACH(uis, u, jalv->uis) {
			const LilvUI* this_ui = lilv_uis_get(jalv->uis, u);
			if (lilv_ui_is_supported(this_ui,
			                         suil_ui_supported,
			                         native_ui_type,
			                         &jalv->ui_type)) {
				/* TODO: Multiple UI support */
				jalv->ui = this_ui;
				break;
			}
		}
	} else if (!use_generic_ui) {
		jalv->ui = lilv_uis_get(jalv->uis, lilv_uis_begin(jalv->uis));
	}

	/* Create ringbuffers for UI if necessary */
	if (jalv->ui) {
		fprintf(stderr, "UI:           %s\n",
		        lilv_node_as_uri(lilv_ui_get_uri(jalv->ui)));
	} else {
		fprintf(stderr, "UI:           None\n");
	}

	/* Create port and control structures */
	jalv_create_ports(jalv);
	jalv_create_controls(jalv, true);
	jalv_create_controls(jalv, false);

	if (!(jalv->backend = jalv_backend_init(jalv))) {
		fprintf(stderr, "Failed to connect to audio system");
	}

	printf("Sample rate:  %u Hz\n", jalv->sample_rate);
	printf("Block length: %u frames\n", jalv->block_length);
	printf("MIDI buffers: %zu bytes\n", jalv->midi_buf_size);

	if (jalv->opts.buffer_size == 0) {
		/* The UI ring is fed by plugin output ports (usually one), and the UI
		   updates roughly once per cycle.  The ring size is a few times the
		   size of the MIDI output to give the UI a chance to keep up.  The UI
		   should be able to keep up with 4 cycles, and tests show this works
		   for me, but this value might need increasing to avoid overflows.
		*/
		jalv->opts.buffer_size = jalv->midi_buf_size * N_BUFFER_CYCLES;
	}

	if (jalv->opts.update_rate == 0.0) {
		/* Calculate a reasonable UI update frequency. */
		jalv->ui_update_hz = (float)jalv->sample_rate / jalv->midi_buf_size * 2.0f;
		jalv->ui_update_hz = MAX(25.0f, jalv->ui_update_hz);
	} else {
		/* Use user-specified UI update rate. */
		jalv->ui_update_hz = jalv->opts.update_rate;
		jalv->ui_update_hz = MAX(1.0f, jalv->ui_update_hz);
	}

	/* The UI can only go so fast, clamp to reasonable limits */
	jalv->ui_update_hz     = MIN(60, jalv->ui_update_hz);
	jalv->opts.buffer_size = MAX(4096, jalv->opts.buffer_size);
	fprintf(stderr, "Comm buffers: %d bytes\n", jalv->opts.buffer_size);
	fprintf(stderr, "Update rate:  %.01f Hz\n", jalv->ui_update_hz);

	/* Build options array to pass to plugin */
	const LV2_Options_Option options[] = {
		{ LV2_OPTIONS_INSTANCE, 0, jalv->urids.param_sampleRate,
		  sizeof(float), jalv->urids.atom_Float, &jalv->sample_rate },
		{ LV2_OPTIONS_INSTANCE, 0, jalv->urids.bufsz_minBlockLength,
		  sizeof(int32_t), jalv->urids.atom_Int, &jalv->block_length },
		{ LV2_OPTIONS_INSTANCE, 0, jalv->urids.bufsz_maxBlockLength,
		  sizeof(int32_t), jalv->urids.atom_Int, &jalv->block_length },
		{ LV2_OPTIONS_INSTANCE, 0, jalv->urids.bufsz_sequenceSize,
		  sizeof(int32_t), jalv->urids.atom_Int, &jalv->midi_buf_size },
		{ LV2_OPTIONS_INSTANCE, 0, jalv->urids.ui_updateRate,
		  sizeof(float), jalv->urids.atom_Float, &jalv->ui_update_hz },
		{ LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL }
	};

	extern LV2_Feature options_feature;
	options_feature.data = (void*)&options;

	/* Create Plugin <=> UI communication buffers */
	jalv->ui_events     = zix_ring_new(jalv->opts.buffer_size);
	jalv->plugin_events = zix_ring_new(jalv->opts.buffer_size);
	zix_ring_mlock(jalv->ui_events);
	zix_ring_mlock(jalv->plugin_events);

	/* Instantiate the plugin */
	jalv->instance = lilv_plugin_instantiate(
		plgn, jalv->sample_rate, Lv2Manager::features);
	if (!jalv->instance) {
		fprintf(stderr, "Failed to instantiate plugin.\n");
	}

	Lv2Manager::ext_data.data_access = lilv_instance_get_descriptor(jalv->instance)->extension_data;

	fprintf(stderr, "\n");
	if (!jalv->buf_size_set) {
		jalv_allocate_port_buffers(jalv);
	}

	/* Create workers if necessary */
	if (lilv_plugin_has_extension_data(plgn, jalv->nodes.work_interface)) {
		const LV2_Worker_Interface* iface = (const LV2_Worker_Interface*)
			lilv_instance_get_extension_data(jalv->instance, LV2_WORKER__interface);

		jalv_worker_init(jalv, &jalv->worker, iface, true);
		if (jalv->safe_restore) {
			jalv_worker_init(jalv, &jalv->state_worker, iface, false);
		}
	}

	/* Apply loaded state to plugin instance if necessary */
	if (state) {
		jalv_apply_state(jalv, state);
	}

	/* Set Jack callbacks */
	jalv_backend_init(jalv);

	/* Create Jack ports and connect plugin ports to buffers */
	for (uint32_t i = 0; i < jalv->num_ports; ++i) {
		jalv_backend_activate_port(jalv, i);
	}

	/* Print initial control values */
	for (size_t i = 0; i < jalv->controls.n_controls; ++i) {
		ControlID* control = jalv->controls.controls[i];
		if (control->type == PORT) {// && control->value_type == jalv->forge.Float) {
			struct Port* port = &jalv->ports[control->index];
			Lv2Manager::print_control_value(jalv, port, port->control);
		}
	}

	/* Activate plugin */
	lilv_instance_activate(jalv->instance);

	/* Discover UI */
	jalv->has_ui = jalv_discover_ui(jalv);

	/* Activate Jack */
	jalv_backend_activate(jalv);
	jalv->play_state = JALV_RUNNING;

	/* Run UI (or prompt at console) */
	jalv_open_ui(jalv);
}

Lv2Instrument::~Lv2Instrument()
{

}

void Lv2Instrument::playNote( NotePlayHandle * _note_to_play/* _note_to_play */,
				sampleFrame * _working_buf/* _working_buf */ )
{
	// TODO
		memset( _working_buf, 0, sizeof( sampleFrame ) *
			Engine::mixer()->framesPerPeriod() );
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
