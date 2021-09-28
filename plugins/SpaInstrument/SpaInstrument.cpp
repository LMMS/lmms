/*
 * SpaInstrument.cpp - implementation of SPA instrument
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

#include "SpaInstrument.h"

#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QTemporaryFile>
#include <spa/audio.h>

#include "AutomatableModel.h"
#include "ControllerConnection.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "AudioEngine.h"
#include "SpaProc.h" // TODO: remove from here -> use proper class cascading
#include "SpaSubPluginFeatures.h"
#include "StringPairDrag.h" // DnD TODO: move to SpaViewBase?
#include "embed.h"
#include "TimePos.h"

#include "plugin_export.h"

Plugin::Descriptor PLUGIN_EXPORT spainstrument_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"SPA",
	QT_TRANSLATE_NOOP("SpaInstrument",
		"plugin for using arbitrary SPA instruments inside LMMS."),
	"Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	new SpaSubPluginFeatures(Plugin::Instrument)
};

/*DataFile::Types SpaInstrument::settingsType()
{
	return DataFile::InstrumentTrackSettings;
}*/

void SpaInstrument::setNameFromFile(const QString &name)
{
	instrumentTrack()->setName(name);
}

SpaInstrument::SpaInstrument(InstrumentTrack *instrumentTrackArg,
	Descriptor::SubPluginFeatures::Key *key) :
	Instrument(instrumentTrackArg, &spainstrument_plugin_descriptor, key),
	SpaControlBase(this, key->attributes["plugin"],
		DataFile::Type::InstrumentTrackSettings)
{
	if (isValid())
	{
		connect(instrumentTrack()->pitchRangeModel(), SIGNAL(dataChanged()),
			this, SLOT(updatePitchRange()));
		connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()),
			this, SLOT(reloadPlugin())); // TODO: refactor to SpaControlBase?

		// now we need a play-handle which cares for calling play()
		InstrumentPlayHandle *iph =
			new InstrumentPlayHandle(this, instrumentTrackArg);
		Engine::audioEngine()->addPlayHandle(iph);
	}
}

SpaInstrument::~SpaInstrument()
{
	Engine::audioEngine()->removePlayHandlesOfTypes(instrumentTrack(),
		PlayHandle::TypeNotePlayHandle |
						  PlayHandle::TypeInstrumentPlayHandle);
}

void SpaInstrument::saveSettings(QDomDocument &doc, QDomElement &that)
{
	SpaControlBase::saveSettings(doc, that);
}

void SpaInstrument::loadSettings(const QDomElement &that)
{
	SpaControlBase::loadSettings(that);
}

// not yet working
#ifndef SPA_INSTRUMENT_USE_MIDI
void SpaInstrument::playNote(NotePlayHandle *nph, sampleFrame *)
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
}
#endif

void SpaInstrument::play(sampleFrame *buf)
{	
	copyModelsFromLmms();

	fpp_t fpp = Engine::audioEngine()->framesPerPeriod();

	run(static_cast<unsigned>(fpp));

	copyBuffersToLmms(buf, fpp);

	instrumentTrack()->processAudioBuffer(buf, fpp, nullptr);
}

void SpaInstrument::updatePitchRange()
{
	qDebug() << "Lmms: Cannot update pitch range for spa plugin:"
		    "not implemented yet";
}

QString SpaInstrument::nodeName() const
{
	return SpaControlBase::nodeName();
}

#ifdef SPA_INSTRUMENT_USE_MIDI
bool SpaInstrument::handleMidiEvent(
	const MidiEvent &event, const TimePos &time, f_cnt_t offset)
{
	// this function can be called from GUI threads while the plugin is running
	// handleMidiInputEvent will use a thread-safe ringbuffer
	handleMidiInputEvent(event, time, offset);
	return true;
}
#endif

PluginView *SpaInstrument::instantiateView(QWidget *parent)
{
	return new SpaInsView(this, parent);
}

unsigned SpaInstrument::netPort(std::size_t chan) const
{
	return chan < m_procs.size() ? m_procs[chan]->netPort() : 0;
}

AutomatableModel *SpaInstrument::modelAtPort(const QString &dest)
{
	return SpaControlBase::modelAtPort(dest);
}

SpaInsView::SpaInsView(SpaInstrument *_instrument, QWidget *_parent) :
	InstrumentView(_instrument, _parent),
	SpaViewBase(this, _instrument)
{
	setAutoFillBackground(true);
	connect(m_reloadPluginButton, SIGNAL(toggled(bool)),
		this, SLOT(reloadPlugin()));
	if(m_toggleUIButton)
		connect(m_toggleUIButton, SIGNAL(toggled(bool)),
			this, SLOT(toggleUI()));
}

SpaInsView::~SpaInsView()
{
	SpaInstrument *model = castModel<SpaInstrument>();
	if (model && model->m_spaDescriptor->ui_ext() && model->m_hasGUI)
	{
		qDebug() << "shutting down UI...";
		model->uiExtShow(false);
	}
}

void SpaInsView::modelChanged()
{
	SpaViewBase::modelChanged(castModel<SpaInstrument>());
}

void SpaInsView::toggleUI()
{
	SpaInstrument *model = castModel<SpaInstrument>();
	if (model->m_spaDescriptor->ui_ext() &&
		model->m_hasGUI != m_toggleUIButton->isChecked())
	{
		model->m_hasGUI = m_toggleUIButton->isChecked();
		model->uiExtShow(model->m_hasGUI);
		ControllerConnection::finalizeConnections();
	}
}

void SpaInsView::reloadPlugin()
{
	castModel<SpaInstrument>()->reloadPlugin();
}




extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *_parent, void *_data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	return new SpaInstrument(static_cast<InstrumentTrack*>(_parent),
		static_cast<KeyType*>(_data ));
}

}
