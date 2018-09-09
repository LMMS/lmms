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
#include "worker.h"
#include "symap.h"

#include <cmath>

#include "Engine.h"
//#include "JalvQt.h"
#include "Lv2Instrument.h"
#include "Lv2Manager.h"
#include "Mixer.h"
#include "Song.h"
#include "log.h"


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
	"",
	0x0100,
	Plugin::Instrument,
  Plugin::Lv2,
	&pml, // logo
	NULL
} ;
}


/**
 * Creates an LV2 instrument from the LV2 Plugin object.
 *
 * Since it is called from Plugin::instantiate, also instantiate
 */
Lv2Instrument::Lv2Instrument (const QString & uri, ///< plugin
    InstrumentTrack * _it) :
  Instrument( _it, &lv2_plugin_descriptor )
{
  plugin = new Lv2Plugin (uri);

  plugin->instantiate ();
}

Lv2Instrument::~Lv2Instrument()
{
  /* TODO free resources */

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
	//const bool xport_changed = (rolling != jalvPlugin.rolling ||
								//frame != jalvPlugin.position ||
								//beats_per_minute != jalvPlugin.bpm);
                // FIXME
  const int xport_changed = 1;

	int beat = song->getBeat();
	int tick = song->getTicks();

	uint8_t   pos_buf[256];
	LV2_Atom* lv2_pos = (LV2_Atom*)pos_buf;
	if (xport_changed) {
		/* Build an LV2 position object to report change to plugin */
		lv2_atom_forge_set_buffer(&plugin->forge, pos_buf, sizeof(pos_buf));
		LV2_Atom_Forge*      forge = &plugin->forge;
		LV2_Atom_Forge_Frame frame;
		lv2_atom_forge_object(forge, &frame, 0, plugin->urids.time_Position);
		lv2_atom_forge_key(forge, plugin->urids.time_frame);
		//lv2_atom_forge_long(forge, frame);
		lv2_atom_forge_key(forge, plugin->urids.time_speed);
		lv2_atom_forge_float(forge, rolling ? 1.0 : 0.0);
		//if (pos.valid & JackPositionBBT) {
			lv2_atom_forge_key(forge, plugin->urids.time_barBeat);
			MeterModel* timeSig = &song->getTimeSigModel();
			lv2_atom_forge_float(
				forge, beat - 1 + (tick /
					(song->ticksPerTact() / timeSig->numeratorModel().value())));
			// FIXME 4 = beats per bar ^
			lv2_atom_forge_key(forge, plugin->urids.time_bar);
			lv2_atom_forge_long(forge, song->getTacts() - 1);
			lv2_atom_forge_key(forge, plugin->urids.time_beatUnit);
			//lv2_atom_forge_int(forge, pos.beat_type);
			lv2_atom_forge_key(forge, plugin->urids.time_beatsPerBar);
			//lv2_atom_forge_float(forge, song->beatspos.beats_per_bar);
			lv2_atom_forge_key(forge, plugin->urids.time_beatsPerMinute);
			lv2_atom_forge_float(forge, beats_per_minute);
		//}

		char* str = sratom_to_turtle(
			plugin->sratom, &plugin->unmap, "time:", NULL, NULL,
			lv2_pos->type, lv2_pos->size, LV2_ATOM_BODY(lv2_pos));
		lv2_ansi_start(stdout, 36);
		printf("\n## Position ##\n%s\n", str);
		lv2_ansi_reset(stdout);
		free(str);
	}

	/* Update transport state to expected values for next cycle */
	plugin->position = rolling ? frame + frames : frame;
	plugin->bpm      = beats_per_minute;
	plugin->rolling  = rolling;

	//switch (plugin->play_state) {
	//case JALV_PAUSE_REQUESTED:
		//plugin->play_state = JALV_PAUSED;
		//zix_sem_post(&plugin->paused);
		//break;
	//case JALV_PAUSED:
		//for (uint32_t p = 0; p < plugin->num_ports; ++p) {
			////jack_port_t* jport = plugin->ports[p].sys_port;
			////if (jport && plugin->ports[p].flow == FLOW_OUTPUT) {
				////void* buf = jack_port_get_buffer(jport, nframes);
				////if (plugin->ports[p].type == TYPE_EVENT) {
					////jack_midi_clear_buffer(buf);
				////} else {
					////memset(buf, '\0', frames * sizeof(float));
				////}
			////}
		//}
		////return 0;
	//default:
		//break;
	//}

	/* Prepare port buffers */
	for (uint32_t p = 0; p < plugin->num_ports; ++p) {
		Port* port = &plugin->ports[p];
		if (port->type == TYPE_AUDIO && port->sys_port) {
			/* Connect plugin port directly to Jack port buffer */
			//lilv_instance_connect_port(
				//plugin->instance, p,
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

			if (plugin->request_update) {
				/* Plugin state has changed, request an update */
				const LV2_Atom_Object get = {
					{ sizeof(LV2_Atom_Object_Body), plugin->urids.atom_Object },
					{ 0, plugin->urids.patch_Get } };
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
									//plugin->midi_event_id,
									//ev.size, ev.buffer);
				//}
			}
		} else if (port->type == TYPE_EVENT) {
			/* Clear event output for plugin to write to */
			lv2_evbuf_reset(port->evbuf, false);
		}
	}
	plugin->request_update = false;

	/* Run plugin for this cycle */
	const bool send_ui_updates = plugin->run(frames);

	/* Deliver MIDI output and UI events */
	for (uint32_t p = 0; p < plugin->num_ports; ++p) {
		Port* const port = &plugin->ports[p];
		if (port->flow == FLOW_OUTPUT && port->type == TYPE_CONTROL &&
		    lilv_port_has_property(plugin->plugin, port->lilv_port,
		                           plugin->nodes.lv2_reportsLatency)) {
			if (plugin->plugin_latency != port->control) {
				plugin->plugin_latency = port->control;
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

				if (buf && type == plugin->midi_event_id) {
					// Write MIDI event to Jack output
					//jack_midi_event_write(buf, frames, body, size);
				}

				if (plugin->has_ui && !port->old_api) {
					// Forward event to UI
					plugin->send_to_ui(p, type, size, body);
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
			if (zix_ring_write(plugin->plugin_events, buf, sizeof(buf))
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
	return lilv_node_as_string(lilv_plugin_get_name(plugin->plugin));
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
