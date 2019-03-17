/*
 * Lv2Instrument.cpp - implementation of LV2 instrument
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "Lv2Instrument.h"

#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QTemporaryFile>
#include <lv2.h>

#include "AutomatableModel.h"
#include "ControllerConnection.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "LedCheckbox.h"
#include "Lv2Proc.h"
#include "Lv2SubPluginFeatures.h"
#include "StringPairDrag.h" // DnD
#include "gui_templates.h"
#include "embed.h"
#include "plugin_export.h"




Plugin::Descriptor PLUGIN_EXPORT lv2instrument_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"LV2",
	QT_TRANSLATE_NOOP("Lv2Instrument",
		"plugin for using arbitrary LV2 instruments inside LMMS."),
	"Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	new Lv2SubPluginFeatures(Plugin::Instrument)
};




/*
	Lv2Instrument
*/


Lv2Instrument::Lv2Instrument(InstrumentTrack *instrumentTrackArg,
	Descriptor::SubPluginFeatures::Key *key) :
	Instrument(instrumentTrackArg, &lv2instrument_plugin_descriptor, key),
	Lv2ControlBase(this, key->attributes["uri"])
{
	if(Lv2ControlBase::isValid())
	{
#ifdef LV2_INSTRUMENT_USE_MIDI
		for (int i = 0; i < NumKeys; ++i) {
			m_runningNotes[i] = 0;
		}
#endif
		connect(instrumentTrack()->pitchRangeModel(), SIGNAL(dataChanged()),
			this, SLOT(updatePitchRange()));
		connect(Engine::mixer(), SIGNAL(sampleRateChanged()),
			this, SLOT(reloadPlugin()));
		if(multiChannelLinkModel()) {
			connect(multiChannelLinkModel(), SIGNAL(dataChanged()),
				this, SLOT(updateLinkStatesFromGlobal()));
			connect(getGroup(0), SIGNAL(linkStateChanged(int, bool)),
					this, SLOT(linkPort(int, bool)));
		}

		// now we need a play-handle which cares for calling play()
		InstrumentPlayHandle *iph =
			new InstrumentPlayHandle(this, instrumentTrackArg);
		Engine::mixer()->addPlayHandle(iph);
	}
}




Lv2Instrument::~Lv2Instrument()
{
	Engine::mixer()->removePlayHandlesOfTypes(instrumentTrack(),
		PlayHandle::TypeNotePlayHandle |
						  PlayHandle::TypeInstrumentPlayHandle);
}




bool Lv2Instrument::isValid() const { return Lv2ControlBase::isValid(); }




void Lv2Instrument::saveSettings(QDomDocument &doc, QDomElement &that)
{
	Lv2ControlBase::saveSettings(doc, that);
}




void Lv2Instrument::loadSettings(const QDomElement &that)
{
	Lv2ControlBase::loadSettings(that);
}




void Lv2Instrument::loadFile(const QString &file)
{
	Lv2ControlBase::loadFile(file);
}




#ifdef LV2_INSTRUMENT_USE_MIDI
bool Lv2Instrument::handleMidiEvent(
	const MidiEvent &event, const MidiTime &time, f_cnt_t offset)
{
	(void)time;
	(void)offset;
#ifdef TODO
	switch (event.type())
	{
	// the old zynaddsubfx plugin always uses channel 0
	case MidiNoteOn:
		if (event.velocity() > 0)
		{
			if (event.key() <= 0 || event.key() >= 128)
			{
				break;
			}
			if (m_runningNotes[event.key()] > 0)
			{
				m_pluginMutex.lock();
				writeOsc("/noteOff", "ii", 0, event.key());
				m_pluginMutex.unlock();
			}
			++m_runningNotes[event.key()];
			m_pluginMutex.lock();
			writeOsc("/noteOn", "iii", 0, event.key(),
				event.velocity());
			m_pluginMutex.unlock();
		}
		break;
	case MidiNoteOff:
		if (event.key() > 0 && event.key() < 128) {
			if (--m_runningNotes[event.key()] <= 0)
			{
				m_pluginMutex.lock();
				writeOsc("/noteOff", "ii", 0, event.key());
				m_pluginMutex.unlock();
			}
		}
		break;
		/*              case MidiPitchBend:
				m_master->SetController( event.channel(),
		   C_pitchwheel, event.pitchBend()-8192 ); break; case
		   MidiControlChange: m_master->SetController( event.channel(),
					midiIn.getcontroller(
		   event.controllerNumber() ), event.controllerValue() );
				break;*/
	default:
		break;
	}
#else
	(void)event;
#endif
	// those can be called from GUI threads while the plugin is running,
	// so this requires caching, e.g. in ringbuffers
	return true;
}
#endif




// not yet working
#ifndef LV2_INSTRUMENT_USE_MIDI
void Lv2Instrument::playNote(NotePlayHandle *nph, sampleFrame *)
{
	// no idea what that means
	if (nph->isMasterNote() || (nph->hasParent() && nph->isReleased()))
	{
		return;
	}

	const f_cnt_t tfp = nph->totalFramesPlayed();

	const float LOG440 = 2.643452676f;

	int midiNote = (int)floor(
		12.0 * (log2(nph->unpitchedFrequency()) - LOG440) - 4.0);

	qDebug() << "midiNote: " << midiNote << ", r? " << nph->isReleased();
	// out of range?
	if (midiNote <= 0 || midiNote >= 128)
	{
		return;
	}

	if (tfp == 0)
	{
		const int baseVelocity =
			instrumentTrack()->midiPort()->baseVelocity();
		m_plugin->send_osc("/noteOn", "iii", 0, midiNote, baseVelocity);
	}
	else if (nph->isReleased() &&
		!nph->instrumentTrack()
			 ->isSustainPedalPressed()) // note is released during
						    // this period
	{
		m_plugin->send_osc("/noteOff", "ii", 0, midiNote);
	}
	else if (nph->framesLeft() <= 0)
	{
		m_plugin->send_osc("/noteOff", "ii", 0, midiNote);
	}
	// those can be called from GUI threads while the plugin is running,
	// so this requires caching, e.g. in ringbuffers
}
#endif




void Lv2Instrument::play(sampleFrame *buf)
{
	//if (m_plugin)
	{
		copyModelsFromLmms();

		fpp_t fpp = Engine::mixer()->framesPerPeriod();

		run(static_cast<unsigned>(fpp));

		copyBuffersToLmms(buf, fpp);
	}
	instrumentTrack()->processAudioBuffer(
		buf, Engine::mixer()->framesPerPeriod(), nullptr);
}




PluginView *Lv2Instrument::instantiateView(QWidget *parent)
{
	return new Lv2InsView(this, parent);
}




void Lv2Instrument::updatePitchRange()
{
	qDebug() << "Lmms: Cannot update pitch range for lv2 plugin:"
				"not implemented yet";
}




void Lv2Instrument::reloadPlugin() { Lv2ControlBase::reloadPlugin(); }




void Lv2Instrument::updateLinkStatesFromGlobal()
{
	Lv2ControlBase::updateLinkStatesFromGlobal();
}




QString Lv2Instrument::nodeName() const
{
	return Lv2ControlBase::nodeName();
}




DataFile::Types Lv2Instrument::settingsType()
{
	return DataFile::InstrumentTrackSettings;
}




void Lv2Instrument::setNameFromFile(const QString &name)
{
	instrumentTrack()->setName(name);
}




/*
	Lv2InsView
*/


Lv2InsView::Lv2InsView(Lv2Instrument *_instrument, QWidget *_parent) :
	InstrumentView(_instrument, _parent),
	Lv2ViewBase(this, _instrument)
{
	setAutoFillBackground(true);
	if(m_reloadPluginButton) {
		connect(m_reloadPluginButton, SIGNAL(toggled(bool)),
			this, SLOT(reloadPlugin()));
	}
	if(m_toggleUIButton) {
		connect(m_toggleUIButton, SIGNAL(toggled(bool)),
			this, SLOT(toggleUI()));
	}
	if(m_helpButton) {
		connect(m_helpButton, SIGNAL(toggled(bool)),
			this, SLOT(toggleHelp(bool)));
	}
}




Lv2InsView::~Lv2InsView()
{
	Lv2Instrument *model = castModel<Lv2Instrument>();
	if (model && /* DISABLES CODE */ (false)
		/* TODO: check if plugin has UI extension */ && model->hasGui())
	{
		qDebug() << "shutting down UI...";
		// TODO: tell plugin to hide the UI
	}
}




void Lv2InsView::dragEnterEvent(QDragEnterEvent *_dee)
{
	void (QDragEnterEvent::*reaction)(void) = &QDragEnterEvent::ignore;

	if (_dee->mimeData()->hasFormat(StringPairDrag::mimeType()))
	{
		const QString txt =
			_dee->mimeData()->data(StringPairDrag::mimeType());
		if (txt.section(':', 0, 0) == "pluginpresetfile") {
			reaction = &QDragEnterEvent::acceptProposedAction;
		}
	}

	(_dee->*reaction)();
}




void Lv2InsView::dropEvent(QDropEvent *_de)
{
	const QString type = StringPairDrag::decodeKey(_de);
	const QString value = StringPairDrag::decodeValue(_de);
	if (type == "pluginpresetfile")
	{
		castModel<Lv2Instrument>()->loadFile(value);
		_de->accept();
		return;
	}
	_de->ignore();
}




void Lv2InsView::reloadPlugin()
{
	Lv2Instrument *model = castModel<Lv2Instrument>();
	model->reloadPlugin();
}




void Lv2InsView::toggleUI()
{
	Lv2Instrument *model = castModel<Lv2Instrument>();
	if (/* DISABLES CODE */ (false)
		/* TODO: check if plugin has the UI extension */ &&
		model->hasGui() != m_toggleUIButton->isChecked())
	{
		model->setHasGui(m_toggleUIButton->isChecked());
		// TODO: show the UI
		ControllerConnection::finalizeConnections();
	}
}




void Lv2InsView::toggleHelp(bool visible)
{
	Lv2ViewBase::toggleHelp(visible);
}




void Lv2InsView::modelChanged()
{
	Lv2ViewBase::modelChanged(castModel<Lv2Instrument>());
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *_parent, void *_data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	Lv2Instrument* ins = new Lv2Instrument(
							static_cast<InstrumentTrack*>(_parent),
							static_cast<KeyType*>(_data ));
	if(!ins->isValid())
		ins = nullptr;
	return ins;
}

}
