/*
 * ClapInstrument.cpp - Implementation of CLAP instrument
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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
#include "ClapLog.h"
#include "ClapSubPluginFeatures.h"
#include "Clipboard.h"
#include "embed.h"
#include "Engine.h"
#include "InstrumentPlayHandle.h"
#include "InstrumentTrack.h"
#include "PixmapButton.h"
#include "plugin_export.h"
#include "StringPairDrag.h"

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

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	using KeyType = Plugin::Descriptor::SubPluginFeatures::Key;
	auto instrument = std::make_unique<ClapInstrument>(static_cast<InstrumentTrack*>(parent), static_cast<KeyType*>(data));
	if (!instrument || !instrument->isValid()) { return nullptr; }
	return instrument.release();
}

} // extern "C"


/*
 * ClapInstrument
 */

ClapInstrument::ClapInstrument(InstrumentTrack* track, Descriptor::SubPluginFeatures::Key* key)
	: Instrument{track, &clapinstrument_plugin_descriptor, key, Flag::IsSingleStreamed | Flag::IsMidiBased}
	, m_instance{ClapInstance::create(key->attributes["uri"].toStdString(), this)}
	, m_idleTimer{this}
{
	if (isValid())
	{
		clearRunningNotes();

		connect(instrumentTrack()->pitchRangeModel(), &IntModel::dataChanged,
			this, &ClapInstrument::updatePitchRange, Qt::DirectConnection);
		connect(Engine::audioEngine(), &AudioEngine::sampleRateChanged, this, [this](){ onSampleRateChanged(); });

		m_idleTimer.moveToThread(QApplication::instance()->thread());
		connect(&m_idleTimer, &QTimer::timeout, this, [this] { if (m_instance) { m_instance->idle(); } });
		m_idleTimer.start(1000 / 30);

		// Now we need a play handle which cares for calling play()
		auto iph = new InstrumentPlayHandle{this, track};
		Engine::audioEngine()->addPlayHandle(iph); // TODO: Is this only for non-midi-based instruments?
	}
}

ClapInstrument::~ClapInstrument()
{
	Engine::audioEngine()->removePlayHandlesOfTypes(instrumentTrack(),
		PlayHandle::Type::NotePlayHandle | PlayHandle::Type::InstrumentPlayHandle);
}

auto ClapInstrument::isValid() const -> bool
{
	return m_instance != nullptr
		&& m_instance->isValid();
}

void ClapInstrument::reload()
{
	if (m_instance) { m_instance->restart(); }
	clearRunningNotes();

	emit modelChanged();
}

void ClapInstrument::clearRunningNotes()
{
	m_runningNotes.fill(0);
}

void ClapInstrument::onSampleRateChanged()
{
	reload();
}

void ClapInstrument::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	if (!m_instance) { return; }
	m_instance->saveSettings(doc, elem);
}

void ClapInstrument::loadSettings(const QDomElement& elem)
{
	if (!m_instance) { return; }
	m_instance->loadSettings(elem);
}

void ClapInstrument::loadFile(const QString& file)
{
	if (auto database = m_instance->presetLoader().presetDatabase())
	{
		auto presets = database->findOrLoadPresets(file.toStdString());
		if (!presets.empty())
		{
			m_instance->presetLoader().activatePreset(presets[0]->loadData());
		}
	}
}

auto ClapInstrument::hasNoteInput() const -> bool
{
	return m_instance ? m_instance->hasNoteInput() : false;
}

auto ClapInstrument::handleMidiEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset) -> bool
{
	// This function can be called from GUI threads while the plugin is running
	// handleMidiInputEvent will use a thread-safe ringbuffer

	if (!m_instance) { return false; }
	m_instance->handleMidiInputEvent(event, time, offset);
	return true;
}

void ClapInstrument::play(SampleFrame* buffer)
{
	if (!m_instance) { return; }

	m_instance->copyModelsFromCore();

	const fpp_t fpp = Engine::audioEngine()->framesPerPeriod();
	m_instance->run(fpp);

	m_instance->copyModelsToCore();
	m_instance->audioPorts().copyBuffersToCore(buffer, fpp);
	// TODO: Do bypassed channels need to be zeroed out or does AudioEngine handle it?
}

auto ClapInstrument::instantiateView(QWidget* parent) -> gui::PluginView*
{
	return new gui::ClapInsView{this, parent};
}

void ClapInstrument::updatePitchRange()
{
	if (!m_instance) { return; }
	m_instance->logger().log(CLAP_LOG_ERROR, "ClapInstrument::updatePitchRange() [NOT IMPLEMENTED YET]");
	// TODO
}


namespace gui
{

/*
 * ClapInsView
 */

ClapInsView::ClapInsView(ClapInstrument* instrument, QWidget* parent)
	: InstrumentView{instrument, parent}
	, ClapViewBase{this, instrument->m_instance.get()}
{
	setAutoFillBackground(true);
	if (m_reloadPluginButton)
	{
		connect(m_reloadPluginButton, &QPushButton::clicked,
			this, [this] { this->castModel<ClapInstrument>()->reload();} );
	}

	if (m_toggleUIButton)
	{
		connect(m_toggleUIButton, &QPushButton::toggled,
			this, [this] { toggleUI(); });
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
	ClapViewBase::modelChanged(castModel<ClapInstrument>()->m_instance.get());
	connect(castModel<ClapInstrument>(), &ClapInstrument::modelChanged, this, [this] { this->modelChanged(); } );
}

} // namespace gui

} // namespace lmms
