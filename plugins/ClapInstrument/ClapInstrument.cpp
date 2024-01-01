/*
 * ClapInstrument.cpp - Implementation of CLAP instrument
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapInstrument.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QPushButton>

#include "AudioEngine.h"
#include "Engine.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "ClapSubPluginFeatures.h"
#include "StringPairDrag.h"
#include "Clipboard.h"

#include "embed.h"
#include "plugin_export.h"


namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT clapinstrument_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"CLAP",
	QT_TRANSLATE_NOOP("PluginBrowser", "plugin for using arbitrary CLAP instruments inside LMMS."),
	"Dalton Messmer <messmer.dalton/at/gmail.com>",
	0x0100,
	Plugin::Type::Instrument,
	new PluginPixmapLoader("logo"),
	nullptr,
	new ClapSubPluginFeatures(Plugin::Type::Instrument)
};

}

/*
 * ClapInstrument
 */

ClapInstrument::ClapInstrument(InstrumentTrack* track, Descriptor::SubPluginFeatures::Key* key)
	: Instrument{track, &clapinstrument_plugin_descriptor, key}
	, ClapControlBase{this, key->attributes["uri"]}
	, m_idleTimer{this}
{
	if (ClapControlBase::isValid())
	{
		clearRunningNotes();

		connect(instrumentTrack()->pitchRangeModel(), &IntModel::dataChanged,
			this, &ClapInstrument::updatePitchRange, Qt::DirectConnection);
		connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, this, [this](){ onSampleRateChanged(); });

		m_idleTimer.moveToThread(QApplication::instance()->thread());
		connect(&m_idleTimer, &QTimer::timeout, this, [this](){ idle(); });
		m_idleTimer.start(1000 / 30);

		// Now we need a play handle which cares for calling play()
		auto iph = new InstrumentPlayHandle{this, track};
		Engine::audioEngine()->addPlayHandle(iph);
	}
}

ClapInstrument::~ClapInstrument()
{
	Engine::audioEngine()->removePlayHandlesOfTypes(instrumentTrack(),
		PlayHandle::Type::NotePlayHandle | PlayHandle::Type::InstrumentPlayHandle);
}

void ClapInstrument::reload()
{
	ClapControlBase::reload();
	clearRunningNotes();
	emit modelChanged();
}

void ClapInstrument::clearRunningNotes()
{
#ifdef CLAP_INSTRUMENT_USE_MIDI
	for (int i = 0; i < NumKeys; ++i) { m_runningNotes[i] = 0; }
#endif
}

void ClapInstrument::onSampleRateChanged()
{
	reload();
}

auto ClapInstrument::isValid() const -> bool
{
	return ClapControlBase::isValid();
}

void ClapInstrument::saveSettings(QDomDocument& doc, QDomElement& that)
{
	ClapControlBase::saveSettings(doc, that);
}

void ClapInstrument::loadSettings(const QDomElement& that)
{
	ClapControlBase::loadSettings(that);
}

void ClapInstrument::loadFile(const QString& file)
{
	ClapControlBase::loadFile(file);
}

#ifdef CLAP_INSTRUMENT_USE_MIDI
auto ClapInstrument::handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset) -> bool
{
	// this function can be called from GUI threads while the plugin is running
	// handleMidiInputEvent will use a thread-safe ringbuffer
	handleMidiInputEvent(event, time, offset);
	return true;
}
#else
void ClapInstrument::playNote(NotePlayHandle* nph, sampleFrame*)
{
}
#endif

void ClapInstrument::play(sampleFrame* buf)
{
	copyModelsFromLmms();

	fpp_t fpp = Engine::audioEngine()->framesPerPeriod();

	run(fpp);

	copyModelsToLmms();
	copyBuffersToLmms(buf, fpp);
}

auto ClapInstrument::instantiateView(QWidget* parent) -> gui::PluginView*
{
	return new gui::ClapInsView{this, parent};
}

void ClapInstrument::updatePitchRange()
{
	ClapLog::globalLog(CLAP_LOG_ERROR, "Cannot update pitch range for CLAP plugin: not implemented yet");
	// TODO
}

auto ClapInstrument::nodeName() const -> QString
{
	return ClapControlBase::nodeName();
}

namespace gui
{

/*
 * ClapInsView
 */

ClapInsView::ClapInsView(ClapInstrument* instrument, QWidget* parent)
	: InstrumentView{instrument, parent}
	, ClapViewBase{this, instrument}
{
	setAutoFillBackground(true);
	if (m_reloadPluginButton)
	{
		connect(m_reloadPluginButton, &QPushButton::clicked,
			this, [this](){ this->castModel<ClapInstrument>()->reload();} );
	}

	if (m_toggleUIButton)
	{
		connect(m_toggleUIButton, &QPushButton::toggled,
			this, [this](){ toggleUI(); });
	}

	if (m_helpButton)
	{
		connect(m_helpButton, &QPushButton::toggled,
			this, [this](bool visible){ toggleHelp(visible); });
	}
}

void ClapInsView::dragEnterEvent(QDragEnterEvent* dee)
{
	// For mimeType() and MimeType enum class
	using namespace Clipboard;

	void (QDragEnterEvent::*reaction)() = &QDragEnterEvent::ignore;

	if (dee->mimeData()->hasFormat(mimeType(MimeType::StringPair)))
	{
		const QString txt = dee->mimeData()->data(mimeType(MimeType::StringPair));
		if (txt.section(':', 0, 0) == "pluginpresetfile")
		{
			reaction = &QDragEnterEvent::acceptProposedAction;
		}
	}

	(dee->*reaction)();
}

void ClapInsView::dropEvent(QDropEvent* de)
{
	const QString type = StringPairDrag::decodeKey(de);
	const QString value = StringPairDrag::decodeValue(de);
	if (type == "pluginpresetfile")
	{
		castModel<ClapInstrument>()->loadFile(value);
		de->accept();
		return;
	}
	de->ignore();
}

void ClapInsView::modelChanged()
{
	ClapViewBase::modelChanged(castModel<ClapInstrument>());
	connect(castModel<ClapInstrument>(), &ClapInstrument::modelChanged, this, [this](){ this->modelChanged(); } );
}

} // namespace gui

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	auto instrument = std::make_unique<ClapInstrument>(static_cast<InstrumentTrack*>(parent), static_cast<KeyType*>(data));
	if (!instrument || !instrument->isValid()) { return nullptr; }
	return instrument.release();
}

}

} // namespace lmms
