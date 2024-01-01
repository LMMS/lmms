/*
 * ClapInstance.h - Implementation of ClapInstance class
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

#ifndef LMMS_CLAP_INSTANCE_H
#define LMMS_CLAP_INSTANCE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapAudioPorts.h"
#include "ClapFile.h"
#include "ClapGui.h"
#include "ClapLog.h"
#include "ClapNotePorts.h"
#include "ClapParams.h"
#include "ClapState.h"
#include "ClapThreadCheck.h"
#include "ClapThreadPool.h"
#include "ClapTimerSupport.h"

#include "Plugin.h"
#include "MidiEvent.h"
#include "TimePos.h"
#include "../src/3rdparty/ringbuffer/include/ringbuffer/ringbuffer.h"

#include <memory>
#include <atomic>
#include <cassert>
#include <cstddef>

#include <clap/clap.h>
#include <clap/helpers/event-list.hh>

namespace lmms
{

/**
 * @brief ClapInstance stores a CLAP host/plugin instance pair.
 *
 * When a new CLAP plugin instance is created by the ClapManager,
 * a new clap_host instance needs to be passed to the plugin instance,
 * creating a CLAP host instance / CLAP plugin instance pair.
 * The plugin instance will pass the host pointer whenever it calls the
 * host's API (instead of passing the plugin pointer), and that is how
 * the host instance can know which plugin instance called the host API.
 * 
 * The ClapInstance class implements the CLAP host API and
 *     stores a pointer to a clap_host object. It also provides access
 *     to the plugin instance for making plugin API calls.
 *
 * Every ClapInstance is owned by a ClapControlBase object.
 */
class ClapInstance final : public QObject
{
	Q_OBJECT;

public:
	ClapInstance() = delete;
	ClapInstance(const ClapPluginInfo* pluginInfo, Model* parent);
	~ClapInstance() override;

	ClapInstance(const ClapInstance&) = delete;
	ClapInstance(ClapInstance&&) noexcept = delete;
	auto operator=(const ClapInstance&) -> ClapInstance& = delete;
	auto operator=(ClapInstance&&) noexcept -> ClapInstance& = delete;

	enum class PluginState
	{
		// The plugin hasn't been created yet or failed to load (NoneWithError)
		None,
		// The plugin has been created but not initialized
		Loaded,
		// Initialization failed - LMMS probably does not support this plugin yet
		LoadedWithError,
		// The plugin is initialized and inactive, only the main thread uses it
		Inactive,
		// Activation failed
		InactiveWithError,
		// The plugin is active and sleeping, the audio engine can call set_processing()
		ActiveAndSleeping,
		// The plugin is processing
		ActiveAndProcessing,
		// The plugin did process but is in error
		ActiveWithError,
		// The plugin is not used anymore by the audio engine and can be deactivated on the main
		// thread
		ActiveAndReadyToDeactivate
	};

	/////////////////////////////////////////
	// LMMS audio thread
	/////////////////////////////////////////

	//! Copy values from the LMMS core (connected models, MIDI events, ...) into
	//! the respective ports
	void copyModelsFromCore();

	//! Bring values from all ports to the LMMS core
	void copyModelsToCore();

	//! Run the CLAP plugin instance for @param frames frames
	void run(fpp_t frames);

	void handleMidiInputEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset);

	auto controlCount() const -> std::size_t;
	auto hasNoteInput() const -> bool;

	/////////////////////////////////////////
	// Instance
	/////////////////////////////////////////

	//! Destroy the plugin instance
	void destroy();

	auto isValid() const -> bool;

	auto host() const -> const clap_host* { return &m_host; };
	auto plugin() const -> const clap_plugin* { return m_plugin; }
	auto info() const -> const ClapPluginInfo& { return *m_pluginInfo; }

	/////////////////////////////////////////
	// Extensions
	/////////////////////////////////////////

	auto audioPorts() -> ClapAudioPorts& { return m_audioPorts; }
	auto gui() -> ClapGui& { return m_gui; }
	auto logger() const -> const ClapLog& { return m_log; }
	auto notePorts() -> ClapNotePorts& { return m_notePorts; }
	auto params() -> ClapParams& { return m_params; }
	auto state() -> ClapState& { return m_state; }
	auto threadPool() -> ClapThreadPool& { return m_threadPool; }
	auto timerSupport() -> ClapTimerSupport& { return m_timerSupport; }

	/////////////////////////////////////////
	// Host
	/////////////////////////////////////////

	//! Executes idle tasks
	void idle();

	/////////////////////////////////////////
	// Plugin
	/////////////////////////////////////////

	auto start() -> bool; //!< Loads, inits, and activates in that order
	auto restart() -> bool;

	auto load() -> bool;
	auto unload() -> bool;
	auto init() -> bool;
	auto activate() -> bool;
	auto deactivate() -> bool;

	auto processBegin(std::uint32_t frames) -> bool;
	void processNote(f_cnt_t offset, std::int8_t channel, std::int16_t key, std::uint8_t velocity, bool isOn);
	void processKeyPressure(f_cnt_t offset, std::int8_t channel, std::int16_t key, std::uint8_t pressure);
	auto process(std::uint32_t frames) -> bool;
	auto processEnd(std::uint32_t frames) -> bool;

	auto isActive() const -> bool;
	auto isProcessing() const -> bool;
	auto isSleeping() const -> bool;
	auto isErrorState() const -> bool;

private:

	/////////////////////////////////////////
	// Host API implementation
	/////////////////////////////////////////

	void setHost();
	clap_host m_host;

	static auto hostGetExtension(const clap_host* host, const char* extensionId) -> const void*;
	static void hostRequestCallback(const clap_host* host);
	static void hostRequestProcess(const clap_host* host);
	static void hostRequestRestart(const clap_host* host);
	static auto hostExtThreadCheckIsMainThread(const clap_host* host) -> bool;
	static auto hostExtThreadCheckIsAudioThread(const clap_host* host) -> bool;
	static void hostExtLatencyChanged(const clap_host* host);

	/////////////////////////////////////////
	// Plugin
	/////////////////////////////////////////

	template<typename... Args>
	void checkPluginStateCurrent(Args... possibilities);

	auto isPluginNextStateValid(PluginState next) -> bool;
	void setPluginState(PluginState state);

	template<typename T, class F>
	auto pluginExtensionInit(const T*& ext, const char* id, F* checkFunc) -> bool;

	const clap_plugin* m_plugin = nullptr;
	const ClapPluginInfo* m_pluginInfo; // TODO: Use weak_ptr instead?

	PluginState m_pluginState = PluginState::Inactive;

	std::vector<PluginIssue> m_pluginIssues;

	std::int64_t m_steadyTime = 0;

	/**
	 * Process-related
	*/
	clap::helpers::EventList m_evIn;  // TODO: Find better way to handle param and note events
	clap::helpers::EventList m_evOut; // TODO: Find better way to handle param and note events
	clap_process m_process{};

	/**
	 * MIDI
	 */
	// many things here may be moved into the `Instrument` class
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
	bool m_scheduleProcess = true; // TODO: ???
	bool m_scheduleMainThreadCallback = false;

	/**
	 * Plugin/Host extension pointers
	*/

	static constexpr const clap_host_latency s_hostExtLatency {
		&hostExtLatencyChanged
	};

	ClapAudioPorts m_audioPorts{ this };
	ClapGui m_gui{ this };
	ClapLog m_log{ this };
	ClapState m_state{ this };
	ClapNotePorts m_notePorts{ this };
	ClapParams m_params;
	ClapThreadCheck m_threadCheck{ this };
	ClapThreadPool m_threadPool{ this };
	ClapTimerSupport m_timerSupport{ this };
};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_INSTANCE_H
