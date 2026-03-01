/*
 * ClapInstance.h - Implementation of ClapInstance class
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

#ifndef LMMS_CLAP_INSTANCE_H
#define LMMS_CLAP_INSTANCE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <atomic>
#include <memory>
#include <clap/ext/latency.h>
#include <clap/helpers/event-list.hh>
#include <clap/host.h>

#include "ClapAudioPorts.h"
#include "ClapGui.h"
#include "ClapLog.h"
#include "ClapNotePorts.h"
#include "ClapParams.h"
#include "ClapPluginInfo.h"
#include "ClapPresetLoader.h"
#include "ClapState.h"
#include "ClapThreadCheck.h"
#include "ClapTimerSupport.h"
#include "MidiEvent.h"
#include "Plugin.h"
#include "SerializingObject.h"
#include "TimePos.h"
#include "../src/3rdparty/ringbuffer/include/ringbuffer/ringbuffer.h"
#include "lmms_export.h"

namespace lmms
{

/**
 * ClapInstance is a CLAP instrument/effect processor which provides
 * basic CLAP functionality plus support for multiple CLAP extensions.
 */
class LMMS_EXPORT ClapInstance final
	: public QObject
	, public SerializingObject
{
	Q_OBJECT;

public:

	//! Passkey idiom
	class Access
	{
	public:
		friend class ClapInstance;
		Access(Access&&) noexcept = default;
	private:
		Access() {}
		Access(const Access&) = default;
	};

	//! Creates and starts a plugin, returning nullptr if an error occurred
	static auto create(const std::string& pluginId, Model* parent) -> std::unique_ptr<ClapInstance>;

	ClapInstance() = delete;
	ClapInstance(Access, const ClapPluginInfo& pluginInfo, Model* parent);
	~ClapInstance() override;

	ClapInstance(const ClapInstance&) = delete;
	ClapInstance(ClapInstance&&) noexcept = delete;
	auto operator=(const ClapInstance&) -> ClapInstance& = delete;
	auto operator=(ClapInstance&&) noexcept -> ClapInstance& = delete;

	auto parent() const -> Model* { return dynamic_cast<Model*>(QObject::parent()); }

	enum class PluginState
	{
		None, // Plugin hasn't been created yet or failed to load (NoneWithError)
		Loaded, // Plugin has been created but not initialized
		LoadedWithError, // Initialization failed - LMMS might not support this plugin yet
		Inactive, // Plugin is initialized and inactive, only the main thread uses it
		InactiveWithError, // Activation failed
		ActiveAndSleeping, // The audio engine can call set_processing()
		ActiveAndProcessing,
		ActiveWithError, // The plugin did process but an irrecoverable error occurred
		ActiveAndReadyToDeactivate // Plugin is unused by audio engine; can deactivate on main thread
	};

	/**
	 * LMMS audio thread
	 */

	//! Copy values from the LMMS core (connected models, MIDI events, ...) into the respective ports
	void copyModelsFromCore();

	//! Bring values from all ports to the LMMS core
	void copyModelsToCore();

	//! Run the CLAP plugin instance for @param frames frames
	void run(fpp_t frames);

	void handleMidiInputEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset);

	auto controlCount() const -> std::size_t;
	auto hasNoteInput() const -> bool;


	/**
	 * SerializingObject implementation
	 */
	static constexpr std::string_view ClapNodeName = "clapcontrols";
	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;
	auto nodeName() const -> QString override { return ClapNodeName.data(); }

	/**
	 * Info
	 */

	auto isValid() const -> bool;
	auto info() const -> const ClapPluginInfo& { return m_pluginInfo; }

	/**
	 * Control
	 */

	auto start() -> bool; //!< Loads, inits, and activates in that order
	auto restart() -> bool;
	void destroy();

	auto load() -> bool;
	auto unload() -> bool;
	auto init() -> bool;
	auto activate() -> bool;
	auto deactivate() -> bool;

	void idle();

	auto processBegin(std::uint32_t frames) -> bool;
	void processNote(f_cnt_t offset, std::int8_t channel, std::int16_t key, std::uint8_t velocity, bool isOn);
	void processKeyPressure(f_cnt_t offset, std::int8_t channel, std::int16_t key, std::uint8_t pressure);
	auto process(std::uint32_t frames) -> bool;
	auto processEnd(std::uint32_t frames) -> bool;

	auto isActive() const -> bool;
	auto isProcessing() const -> bool;
	auto isSleeping() const -> bool;
	auto isErrorState() const -> bool;

	/**
	 * Extensions
	 */

	auto host() const -> const clap_host* { return &m_host; };
	auto plugin() const -> const clap_plugin* { return m_plugin; }

	auto audioPorts() -> ClapAudioPorts& { return m_audioPorts; }
	auto gui() -> ClapGui& { return m_gui; }
	auto logger() -> ClapLog& { return m_log; }
	auto notePorts() -> ClapNotePorts& { return m_notePorts; }
	auto params() -> ClapParams& { return m_params; }
	auto presetLoader() -> ClapPresetLoader& { return m_presetLoader; }
	auto state() -> ClapState& { return m_state; }
	auto timerSupport() -> ClapTimerSupport& { return m_timerSupport; }

private:
	/**
	 * State
	 */

	void setPluginState(PluginState state);
	auto isNextStateValid(PluginState next) const -> bool;

	template<typename... Args>
	auto isCurrentStateValid(Args... validStates) const -> bool
	{
		return ((m_pluginState == validStates) || ...);
	}

	/**
	 * Host API implementation
	 */

	static auto clapGetExtension(const clap_host* host, const char* extensionId) -> const void*;
	static void clapRequestCallback(const clap_host* host);
	static void clapRequestProcess(const clap_host* host);
	static void clapRequestRestart(const clap_host* host);

	/**
	 * Latency extension
	 * TODO: Fully implement and move to separate class
	 */

	static void clapLatencyChanged(const clap_host* host);
	static constexpr const clap_host_latency s_clapLatency {
		&clapLatencyChanged
	};

	/**
	 * Important data members
	 */

	ClapPluginInfo m_pluginInfo;
	PluginState m_pluginState = PluginState::None;

	clap_host m_host;
	const clap_plugin* m_plugin = nullptr;

	/**
	 * Process-related
	 */

	clap_process m_process{};
	clap::helpers::EventList m_evIn;  // TODO: Find better way to handle param and note events
	clap::helpers::EventList m_evOut; // TODO: Find better way to handle param and note events

	/**
	 * MIDI
	 */

	// TODO: Many things here may be moved into the `Instrument` class
	static constexpr std::size_t s_maxMidiInputEvents = 1024;

	//! Spinlock for the MIDI ringbuffer (for MIDI events going to the plugin)
	std::atomic_flag m_ringLock = ATOMIC_FLAG_INIT;

	//! MIDI ringbuffer (for MIDI events going to the plugin)
	ringbuffer_t<struct MidiInputEvent> m_midiInputBuf;

	//! MIDI ringbuffer reader
	ringbuffer_reader_t<struct MidiInputEvent> m_midiInputReader;

	/**
	 * Scheduling
	 */

	bool m_scheduleRestart = false;
	bool m_scheduleDeactivate = false;
	bool m_scheduleProcess = true;
	bool m_scheduleMainThreadCallback = false;

	/**
	 * Extensions
	 */

	ClapAudioPorts m_audioPorts{ this };
	ClapGui m_gui{ this };
	ClapLog m_log{ this };
	ClapNotePorts m_notePorts{ this };
	ClapParams m_params;
	ClapPresetLoader m_presetLoader;
	ClapState m_state{ this };
	ClapThreadCheck m_threadCheck{ this };
	ClapTimerSupport m_timerSupport{ this };
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_INSTANCE_H
