/*
 * Lv2Instrument.cpp - implementation of LV2 instrument
 *
 * Copyright (c) 2018-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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
#include <QDragEnterEvent>

#include "Engine.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "Lv2SubPluginFeatures.h"
#include "Mixer.h"
#include "StringPairDrag.h"
#include "Clipboard.h"

#include "embed.h"
#include "plugin_export.h"




extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT lv2instrument_plugin_descriptor =
{
	STRINGIFY(PLUGIN_NAME),
	"LV2",
	QT_TRANSLATE_NOOP("PluginBrowser",
		"plugin for using arbitrary LV2 instruments inside LMMS."),
	"Johannes Lorenz <jlsf2013$$$users.sourceforge.net, $$$=@>",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	new Lv2SubPluginFeatures(Plugin::Instrument)
};

}




/*
	Lv2Instrument
*/


Lv2Instrument::Lv2Instrument(InstrumentTrack *instrumentTrackArg,
	Descriptor::SubPluginFeatures::Key *key) :
	Instrument(instrumentTrackArg, &lv2instrument_plugin_descriptor, key),
	Lv2ControlBase(this, key->attributes["uri"])
{
	if (Lv2ControlBase::isValid())
	{
#ifdef LV2_INSTRUMENT_USE_MIDI
		for (int i = 0; i < NumKeys; ++i) { m_runningNotes[i] = 0; }
#endif
		connect(instrumentTrack()->pitchRangeModel(), SIGNAL(dataChanged()),
			this, SLOT(updatePitchRange()), Qt::DirectConnection);
		connect(Engine::mixer(), &Mixer::sampleRateChanged,
			this, [this](){Lv2ControlBase::reloadPlugin();});

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
	// this function can be called from GUI threads while the plugin is running
	// handleMidiInputEvent will use a thread-safe ringbuffer
	handleMidiInputEvent(event, time, offset);
	return true;
}
#endif




// not yet working
#ifndef LV2_INSTRUMENT_USE_MIDI
void Lv2Instrument::playNote(NotePlayHandle *nph, sampleFrame *)
{
}
#endif




void Lv2Instrument::play(sampleFrame *buf)
{
	copyModelsFromLmms();

	fpp_t fpp = Engine::mixer()->framesPerPeriod();

	run(fpp);

	copyModelsToLmms();
	copyBuffersToLmms(buf, fpp);

	instrumentTrack()->processAudioBuffer(buf, fpp, nullptr);
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
	if (m_reloadPluginButton) {
		connect(m_reloadPluginButton, &QPushButton::clicked,
			this, [this](){ this->castModel<Lv2Instrument>()->reloadPlugin();} );
	}
	if (m_toggleUIButton) {
		connect(m_toggleUIButton, &QPushButton::toggled,
			this, [this](){ toggleUI(); });
	}
	if (m_helpButton) {
		connect(m_helpButton, &QPushButton::toggled,
			this, [this](bool visible){ toggleHelp(visible); });
	}
}




void Lv2InsView::dragEnterEvent(QDragEnterEvent *_dee)
{
	// For mimeType() and MimeType enum class
	using namespace Clipboard;

	void (QDragEnterEvent::*reaction)(void) = &QDragEnterEvent::ignore;

	if (_dee->mimeData()->hasFormat( mimeType( MimeType::StringPair )))
	{
		const QString txt =
			_dee->mimeData()->data( mimeType( MimeType::StringPair ) );
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
	if (!ins->isValid()) { delete ins; ins = nullptr; }
	return ins;
}

}
