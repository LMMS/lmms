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

#include "ClapFile.h"
#include "ClapParam.h"

#include "LinkedModelGroups.h"
#include "Plugin.h"
#include "MidiEvent.h"
#include "TimePos.h"

#include <memory>
#include <queue>
#include <functional>
#include <cassert>
#include <cstddef>

#include <clap/clap.h>
#include <clap/helpers/event-list.hh>
#include <clap/helpers/reducing-param-queue.hh>

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
class ClapInstance final : public LinkedModelGroup
{
	Q_OBJECT;

public:
	ClapInstance() = delete;
	ClapInstance(const ClapPluginInfo* pluginInfo, Model* parent);
	ClapInstance(const ClapInstance&) = delete;
	ClapInstance& operator=(const ClapInstance&) = delete;
	ClapInstance(ClapInstance&& other) noexcept;
	ClapInstance& operator=(ClapInstance&& rhs) noexcept = delete;
	~ClapInstance() override;

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

	/**
	 * Copy buffer passed by the core into our ports
	 * @param buf buffer of sample frames, each sample frame is something like
	 *   a `float[<number-of-procs> * <channels per proc>]` array.
	 * @param firstChan The offset for @p buf where we have to read our
	 *   first channel.
	 *   This marks the first sample in each sample frame where we read from.
	 *   If we are the 2nd of 2 mono procs, this can be greater than 0.
	 * @param num Number of channels we must read from @param buf (starting at
	 *   @p offset)
	 */
	void copyBuffersFromCore(const sampleFrame* buf, unsigned firstChan, unsigned num, fpp_t frames);

	/**
	 * Copy our ports into buffers passed by the core
	 * @param buf buffer of sample frames, each sample frame is something like
	 *   a `float[<number-of-procs> * <channels per proc>]` array.
	 * @param firstChan The offset for @p buf where we have to write our
	 *   first channel.
	 *   This marks the first sample in each sample frame where we write to.
	 *   If we are the 2nd of 2 mono procs, this can be greater than 0.
	 * @param num Number of channels we must write to @param buf (starting at
	 *   @p offset)
	 */
	void copyBuffersToCore(sampleFrame* buf, unsigned firstChan, unsigned num, fpp_t frames) const;

	//! Run the CLAP plugin instance for @param frames frames
	void run(fpp_t frames);

	void handleMidiInputEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset);

	auto controlCount() const -> std::size_t { return LinkedModelGroup::modelNum(); }
	auto hasNoteInput() const -> bool;

	/////////////////////////////////////////
	// Instance
	/////////////////////////////////////////

	//! Destroy the plugin instance
	void destroy();

	auto isValid() const -> bool;
	auto isMonoInput() const -> bool { return m_monoInput; } //!< Can call after init()
	auto isMonoOutput() const -> bool { return m_monoOutput; } //!< Can call after init()

	auto host() const -> const clap_host* { return &m_host; }
	auto plugin() const -> const clap_plugin* { return m_plugin; }
	auto info() const -> const ClapPluginInfo& { return *m_pluginInfo; }
	auto params() const -> const std::vector<ClapParam*>& { return m_params; }

	/////////////////////////////////////////
	// Host
	/////////////////////////////////////////

	void hostDestroy();

	//! Executes tasks in idle queue
	void hostIdle();

	/////////////////////////////////////////
	// Plugin
	/////////////////////////////////////////

	auto state() const -> PluginState { return m_pluginState; };

	auto start() -> bool; //!< Loads, inits, and activates in that order
	auto restart() -> bool;

	auto load() -> bool;
	auto unload() -> bool;
	auto init() -> bool;
	auto activate() -> bool;
	auto deactivate() -> bool;

	auto processBegin(std::uint32_t frames) -> bool;
	void processNoteOff(int sampleOffset, int channel, int key, int velocity);
	void processNoteOn(int sampleOffset, int channel, int key, int velocity);
	auto process(std::uint32_t frames) -> bool;
	auto processEnd(std::uint32_t frames) -> bool;

	void paramFlushOnMainThread();
	void handlePluginOutputEvents();
	void generatePluginInputEvents();
	auto getParamValueText(const ClapParam* param) const -> std::string;

	auto isActive() const -> bool;
	auto isProcessing() const -> bool;
	auto isSleeping() const -> bool;
	auto isErrorState() const -> bool;

signals:

	void paramsChanged(); //!< Called when CLAP plugin changes params and LMMS core needs to update
	//void quickControlsPagesChanged();
	//void quickControlsSelectedPageChanged();
	void paramAdjusted(clap_id paramId);

private:

	/////////////////////////////////////////
	// Host
	/////////////////////////////////////////

	void setHost();
	void hostPushToIdleQueue(std::function<bool()>&& functor);
	static auto fromHost(const clap_host* host) -> ClapInstance*;
	static auto hostGetExtension(const clap_host* host, const char* extension_id) -> const void*;
	static void hostRequestCallback(const clap_host* host);
	static void hostRequestProcess(const clap_host* host);
	static void hostRequestRestart(const clap_host* host);
	static void hostExtStateMarkDirty(const clap_host* host);
	static void hostExtLogLog([[maybe_unused]] const clap_host* host, clap_log_severity severity, const char* msg);
	static auto hostExtThreadCheckIsMainThread(const clap_host* host) -> bool;
	static auto hostExtThreadCheckIsAudioThread(const clap_host* host) -> bool;
	static void hostExtParamsRescan(const clap_host* host, std::uint32_t flags);
	static void hostExtParamsClear(const clap_host* host, clap_id param_id, clap_param_clear_flags flags);
	static void hostExtParamsRequestFlush(const clap_host* host);
	static void hostExtLatencyChanged([[maybe_unused]] const clap_host* host);

	auto canUsePluginParams() const noexcept -> bool;
	//bool canUsePluginGui() const noexcept;
	//static const char* currentClapGuiApi();

	void setParamValueByHost(ClapParam& param, double value);
	void setParamModulationByHost(ClapParam& param, double value);
	void checkValidParamValue(const ClapParam& param, double value);
	auto getParamValue(const clap_param_info& info) const -> double;
	static auto clapParamsRescanMayValueChange(std::uint32_t flags) -> bool { return flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_VALUES); }
	static auto clapParamsRescanMayInfoChange(std::uint32_t flags) -> bool { return flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_INFO); }

	clap_host m_host;
	std::queue<std::function<bool()>> m_idleQueue;

	//int64_t m_steadyTime = 0;

	/////////////////////////////////////////
	// Plugin
	/////////////////////////////////////////

	template<typename... Args> void checkPluginStateCurrent(Args... possibilities)
	{
		assert(((m_pluginState == possibilities) || ...));
	}
	auto isPluginNextStateValid(PluginState next) -> bool;
	void setPluginState(PluginState state);
	static inline auto isMainThread() -> bool;
	static inline auto isAudioThread() -> bool;

	template<typename T>
	auto pluginExtensionInit(const T*& ext, const char* id) -> bool
	{
		assert(isMainThread());
		assert(ext == nullptr && "Plugin extension already initialized");
		ext = static_cast<const T*>(m_plugin->get_extension(m_plugin, id));
		return ext != nullptr;
	}

	const clap_plugin* m_plugin = nullptr;
	const ClapPluginInfo* m_pluginInfo; // TODO: Use weak_ptr instead?

	PluginState m_pluginState = PluginState::Inactive;
	bool m_monoInput = false;
	bool m_monoOutput = false;

	std::vector<PluginIssue> m_pluginIssues;

	/**
	 * Process-related
	*/
	std::unique_ptr<clap_audio_buffer_t[]> m_audioIn, m_audioOut; // TODO: Why not use a std::vector?
	clap_audio_buffer_t* m_audioInActive = nullptr; //!< Pointer to m_audioIn element used by LMMS
	clap_audio_buffer_t* m_audioOutActive = nullptr; //!< Pointer to m_audioOut element used by LMMS

	//! RAII-enabled CLAP AudioBuffer
	class AudioBuffer
	{
	public:
		AudioBuffer(std::uint32_t channels, std::uint32_t frames)
			: m_channels(channels), m_frames(frames)
		{
			m_data = new float*[m_channels]();
			for (std::uint32_t channel = 0; channel < m_channels; ++channel)
			{
				m_data[channel] = new float[m_frames]();
			}
		}

		AudioBuffer(const AudioBuffer&) = delete;
		AudioBuffer& operator=(const AudioBuffer&) = delete;

		AudioBuffer(AudioBuffer&& other) noexcept :
			m_channels(std::exchange(other.m_channels, 0)),
			m_frames(std::exchange(other.m_frames, 0)),
			m_data(std::exchange(other.m_data, nullptr))
		{
		}

		AudioBuffer& operator=(AudioBuffer&& other) noexcept
		{
			if (this != &other)
			{
				free();
				m_channels = std::exchange(other.m_channels, 0);
				m_frames = std::exchange(other.m_frames, 0);
				m_data = std::exchange(other.m_data, nullptr);
			}
			return *this;
		}

		~AudioBuffer() { free(); }

		//! [channel][frame]
		auto data() const -> float** { return m_data; }

	private:

		void free() noexcept
		{
			if (!m_data) { return; }
			for (std::uint32_t channel = 0; channel < m_channels; ++channel)
			{
				if (m_data[channel]) { delete[] m_data[channel]; }
			}
			delete[] m_data;
		}

		std::uint32_t m_channels;
		std::uint32_t m_frames;
		float** m_data = nullptr;
	};

	std::vector<AudioBuffer> m_audioInBuffers, m_audioOutBuffers; //!< [port][channel][frame]

	clap::helpers::EventList m_evIn;
	clap::helpers::EventList m_evOut;
	clap_process m_process{};

	/**
	 * Parameter update queues
	*/
	std::unordered_map<clap_id, std::unique_ptr<ClapParam>> m_paramMap;
	std::vector<ClapParam*> m_params; //!< Cache for faster iteration

	struct HostToPluginParamQueueValue
	{
		void* cookie;
		double value;
	};

	struct PluginToHostParamQueueValue
	{
		void update(const PluginToHostParamQueueValue& v) noexcept
		{
			if (v.hasValue)
			{
				hasValue = true;
				value = v.value;
			}

			if (v.hasGesture)
			{
				hasGesture = true;
				isBegin = v.isBegin;
			}
		}

		bool hasValue = false;
		bool hasGesture = false;
		bool isBegin = false;
		double value = 0;
	};

	clap::helpers::ReducingParamQueue<clap_id, HostToPluginParamQueueValue> m_hostToPluginValueQueue;
	clap::helpers::ReducingParamQueue<clap_id, HostToPluginParamQueueValue> m_hostToPluginModQueue;
	clap::helpers::ReducingParamQueue<clap_id, PluginToHostParamQueueValue> m_pluginToHostValueQueue;

	std::unordered_map<clap_id, bool> m_isAdjustingParameter;

	static constexpr bool m_hostShouldProvideParamCookie = true;

	/**
	 * Scheduling
	*/
	bool m_scheduleRestart = false;
	bool m_scheduleDeactivate = false;
	bool m_scheduleProcess = true; // TODO: ???
	bool m_scheduleParamFlush = false;
	bool m_scheduleMainThreadCallback = false;

	/**
	 * Ports
	 */
	enum class AudioPortType
	{
		Unsupported,
		Mono,
		Stereo
	};

	struct AudioPort
	{
		clap_audio_port_info info{};
		std::uint32_t index = 0; //!< Index on plugin side, not m_audioPorts***
		bool isInput = false;
		AudioPortType type = AudioPortType::Unsupported;
		bool used = false; //!< In use by LMMS
	};

	std::vector<AudioPort> m_audioPortsIn, m_audioPortsOut;
	AudioPort* m_audioPortInActive = nullptr; //!< Pointer to m_audioPortsIn element used by LMMS
	AudioPort* m_audioPortOutActive = nullptr; //!< Pointer to m_audioPortsOut element used by LMMS

	/**
	 * Plugin/Host extension pointers
	*/
	const clap_plugin_audio_ports* m_pluginExtAudioPorts = nullptr;

	const clap_plugin_state* m_pluginExtState = nullptr;
	static const constexpr clap_host_state m_hostExtState {
		&ClapInstance::hostExtStateMarkDirty
	};

	static const constexpr clap_host_log m_hostExtLog {
		&ClapInstance::hostExtLogLog
	};

	static const constexpr clap_host_thread_check m_hostExtThreadCheck {
		&ClapInstance::hostExtThreadCheckIsMainThread,
		&ClapInstance::hostExtThreadCheckIsAudioThread
	};

	const clap_plugin_params* m_pluginExtParams = nullptr;
	static const constexpr clap_host_params m_hostExtParams {
		&ClapInstance::hostExtParamsRescan,
		&ClapInstance::hostExtParamsClear,
		&ClapInstance::hostExtParamsRequestFlush,
	};

	static const constexpr clap_host_latency m_hostExtLatency {
		&ClapInstance::hostExtLatencyChanged
	};

	/**
	 * Plugin/Host extension data
	*/
	bool m_hostExtStateIsDirty = false;
};

}

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_INSTANCE_H
