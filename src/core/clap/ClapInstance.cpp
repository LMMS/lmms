/*
 * ClapInstance.cpp - Implementation of ClapInstance class
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

#include "ClapInstance.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapManager.h"
#include "ClapTransport.h"
#include "Engine.h"
#include "AudioEngine.h"
#include "MidiEvent.h"

#include <QApplication>
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>

#include <algorithm>
#include <string_view>
#include <cassert>

#ifdef __MINGW32__
#include <mingw.thread.h>
#else
#include <thread>
#endif

#include "lmmsversion.h"

namespace lmms
{

//! Container for everything required to store MIDI events going to the plugin
// TODO: Move to MidiEvent.h for both LV2 and CLAP?
struct MidiInputEvent
{
	MidiEvent ev;
	TimePos time;
	f_cnt_t offset;
};


////////////////////////////////
// ClapInstance
////////////////////////////////

ClapInstance::ClapInstance(const ClapPluginInfo* pluginInfo, Model* parent)
	: QObject{parent}
	, m_pluginInfo{pluginInfo}
	, m_midiInputBuf{s_maxMidiInputEvents}
	, m_midiInputReader{m_midiInputBuf}
	, m_params{parent, this, &m_evIn, &m_evOut}
{
	m_pluginState = PluginState::None;
	setHost();

	m_process.transport = ClapTransport::get();

	start();
}

ClapInstance::~ClapInstance()
{
	qDebug() << "ClapInstance::~ClapInstance";
	destroy();
}

void ClapInstance::copyModelsFromCore()
{
	/*
	for (auto param : m_params)
	{
		if (!param || !param->model()) { continue; }

		// ERROR: Cannot do this on the audio thread - doing in main thread instead
		setParamValueByHost(*param, param->model()->value<float>());
	}
	*/

	// TODO: Handle parameter events similar to midi input? (with ringbuffer)

	if (!hasNoteInput()) { return; }

	// TODO: Midi events and parameter events may not be ordered according to increasing sample offset
	//       The event.header.time values need to be sorted in increasing order.

	while (m_midiInputReader.read_space() > 0)
	{
		const auto [event, ignore, offset] = m_midiInputReader.read(1)[0];
		(void)ignore;
		switch (event.type())
		{
			case MidiNoteOff:
				processNote(offset, event.channel(), event.key(), event.velocity(), false);
				break;
			case MidiNoteOn:
				processNote(offset, event.channel(), event.key(), event.velocity(), true);
				break;
			case MidiKeyPressure:
				processKeyPressure(offset, event.channel(), event.key(), event.velocity());
				break;
			default:
				break;
		}
	}
}

void ClapInstance::copyModelsToCore()
{
	//
}

void ClapInstance::run(fpp_t frames)
{
	processBegin(frames);
	process(frames);
	processEnd(frames);
}

void ClapInstance::handleMidiInputEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	// TODO: Use MidiInputEvent from LV2 code, moved to common location?

	// TODO: Early return if plugin does not support midi input or note events
	if (!hasNoteInput()) { return; }

	const auto ev = MidiInputEvent{event, time, offset};

	// Acquire lock and spin
	while (m_ringLock.test_and_set(std::memory_order_acquire)) {}

	const auto written = m_midiInputBuf.write(&ev, 1);

	m_ringLock.clear(std::memory_order_release);

	if (written != 1)
	{
		qWarning("MIDI ringbuffer is too small! Discarding MIDI event.");
	}
}

auto ClapInstance::controlCount() const -> std::size_t
{
	return m_params.modelNum();
}

auto ClapInstance::hasNoteInput() const -> bool
{
	return m_notePorts.hasInput();
}

void ClapInstance::destroy()
{
	//idle(); // TODO: ??? May throw an exception, which should not happen in ClapInstance dtor

	unload();
}

auto ClapInstance::isValid() const -> bool
{
	return m_plugin != nullptr && !isErrorState() && m_pluginIssues.empty();
}

auto ClapInstance::start() -> bool
{
	if (!load()) { return false; }
	if (!init()) { return false; }
	return activate();
}

auto ClapInstance::restart() -> bool
{
	qDebug() << "ClapInstance::restart";

	if (!deactivate()) { return false; }
	return activate();
}

auto ClapInstance::load() -> bool
{
	qDebug() << "Loading plugin instance:" << m_pluginInfo->descriptor()->name;
	checkPluginStateCurrent(PluginState::None);

	// Create plugin instance, destroying any previous plugin instance first
	const auto factory = m_pluginInfo->factory();
	assert(factory != nullptr);
	m_plugin = factory->create_plugin(factory, host(), m_pluginInfo->descriptor()->id);
	if (!m_plugin)
	{
		qWarning() << "Failed to create instance of CLAP plugin";
		// TODO: Set state to NoneWithError?
		return false;
	}

	setPluginState(PluginState::Loaded);
	return true;
}

auto ClapInstance::unload() -> bool
{
	qDebug() << "Unloading plugin instance:" << m_pluginInfo->descriptor()->name;
	assert(ClapThreadCheck::isMainThread());

	m_gui.deinit();
	m_timerSupport.deinit();

	deactivate();

	if (m_plugin)
	{
		m_plugin->destroy(m_plugin);
		m_plugin = nullptr;
	}

	// Clear all plugin extensions
	m_audioPorts.deinit();
	m_params.deinit();
	m_state.deinit();
	m_notePorts.deinit();

	setPluginState(PluginState::None);
	return true;
}

auto ClapInstance::init() -> bool
{
	qDebug() << "ClapInstance::init";
	assert(ClapThreadCheck::isMainThread());

	if (isErrorState()) { return false; }
	checkPluginStateCurrent(PluginState::Loaded);

	if (m_pluginState != PluginState::Loaded) { return false; }

	if (!m_plugin->init(m_plugin))
	{
		std::string msg = "Could not init the plugin with id:" + std::string(info().descriptor()->id);
		log(CLAP_LOG_ERROR, msg.c_str());
		setPluginState(PluginState::LoadedWithError);
		m_plugin->destroy(m_plugin);
		m_plugin = nullptr;
		return false;
	}

	m_pluginIssues.clear();

	if (!m_audioPorts.init(host(), m_plugin, m_process))
	{
		setPluginState(PluginState::LoadedWithError);
		return false;
	}

	// TODO: What if this is the 2nd instance of a mono plugin?
	m_gui.init(host(), m_plugin);

	m_notePorts.init(host(), m_plugin);
	if (!hasNoteInput() && info().type() == Plugin::Type::Instrument)
	{
		log(CLAP_LOG_WARNING, "Plugin is instrument but doesn't implement note ports extension");
	}

	if (!m_params.init(host(), m_plugin))
	{
		log(CLAP_LOG_DEBUG, "Plugin does not support params extension");
	}

	m_state.init(host(), m_plugin);
	m_threadPool.init(host(), m_plugin);
	m_timerSupport.init(host(), m_plugin);

	setPluginState(PluginState::Inactive);

	return true;
}

auto ClapInstance::activate() -> bool
{
	qDebug() << "ClapInstance::activate";
	assert(ClapThreadCheck::isMainThread());

	if (isErrorState()) { return false; }
	checkPluginStateCurrent(PluginState::Inactive);

	const auto sampleRate = static_cast<double>(Engine::audioEngine()->processingSampleRate());
	static_assert(DEFAULT_BUFFER_SIZE > MINIMUM_BUFFER_SIZE);

	assert(!isActive());
	if (!m_plugin->activate(m_plugin, sampleRate, MINIMUM_BUFFER_SIZE, DEFAULT_BUFFER_SIZE))
	{
		setPluginState(PluginState::InactiveWithError);
		return false;
	}

	m_scheduleProcess = true;
	setPluginState(PluginState::ActiveAndSleeping);
	return true;
}

auto ClapInstance::deactivate() -> bool
{
	// NOTE: This method assumes that process() cannot be called concurrently
	qDebug() << "ClapInstance::deactivate";
	assert(ClapThreadCheck::isMainThread());
	if (!isActive()) { return false; }

	// TODO: m_timerSupport.killTimers()?

	// stop_processing() needs to be called on the audio thread,
	// but the main thread will hang if I try to use m_scheduleDeactivate
	// and poll until the process() event is called - it never seems to be called
	// TODO: Could try spoofing the thread check extension instead of
	//       creating a thread here, but maybe that wouldn't be safe
	auto thread = std::thread{[this] {
		if (m_pluginState == PluginState::ActiveAndProcessing)
		{
			m_plugin->stop_processing(m_plugin);
		}
		setPluginState(PluginState::ActiveAndReadyToDeactivate);
	}};
	thread.join();

	m_plugin->deactivate(m_plugin);
	setPluginState(PluginState::Inactive);

	return true;
}

auto ClapInstance::processBegin(std::uint32_t frames) -> bool
{
	m_process.frames_count = frames;
	m_process.steady_time = m_steadyTime;
	return false;
}

void ClapInstance::processNote(f_cnt_t offset, std::int8_t channel, std::int16_t key, std::uint8_t velocity, bool isOn)
{
	assert(ClapThreadCheck::isAudioThread());
	assert(channel >= 0 && channel <= 15);
	assert(key >= 0 && key <= 127);
	assert(velocity >= 0 && velocity <= 127);

	// NOTE: I've read that Bitwig Studio always sends CLAP dialect note events regardless of plugin's note dialect
	switch (CLAP_NOTE_DIALECT_CLAP)
	{
		case CLAP_NOTE_DIALECT_CLAP:
		{
			clap_event_note ev;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.type = isOn ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
			ev.header.time = static_cast<std::uint32_t>(offset);
			ev.header.flags = 0;
			ev.header.size = sizeof(ev);
			ev.note_id = -1; // TODO
			ev.port_index = static_cast<std::int16_t>(m_notePorts.portIndex());
			ev.channel = channel;
			ev.key = key;
			ev.velocity = velocity / 127.0;

			m_evIn.push(&ev.header);
			break;
		}
		case CLAP_NOTE_DIALECT_MIDI:
		{
			clap_event_midi ev;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.type = CLAP_EVENT_MIDI;
			ev.header.time = static_cast<std::uint32_t>(offset);
			ev.header.flags = 0;
			ev.header.size = sizeof(ev);
			ev.port_index = m_notePorts.portIndex();
			ev.data[0] = static_cast<std::uint8_t>((isOn ? 0x90 : 0x80) | (channel & 0x0F));
			ev.data[1] = static_cast<std::uint8_t>(key);
			ev.data[2] = static_cast<std::uint8_t>(velocity / 127.0);

			m_evIn.push(&ev.header);
			break;
		}
		default:
			assert(false); // Shouldn't ever happen
			break;
	}
}

void ClapInstance::processKeyPressure(f_cnt_t offset, std::int8_t channel, std::int16_t key, std::uint8_t pressure)
{
	assert(ClapThreadCheck::isAudioThread());

	switch (m_notePorts.dialect())
	{
		case CLAP_NOTE_DIALECT_CLAP:
		{
			clap_event_note_expression ev;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.type = CLAP_EVENT_NOTE_EXPRESSION;
			ev.header.time = static_cast<std::uint32_t>(offset);
			ev.header.flags = 0;
			ev.header.size = sizeof(ev);
			ev.expression_id = CLAP_NOTE_EXPRESSION_VOLUME;
			ev.note_id = 0; // TODO
			ev.port_index = static_cast<std::int16_t>(m_notePorts.portIndex());
			ev.channel = channel;
			ev.key = key;
			ev.value = pressure / 32.0; // 0..127 --> 0..4

			m_evIn.push(&ev.header);
			break;
		}
		case CLAP_NOTE_DIALECT_MIDI: [[fallthrough]];
		case CLAP_NOTE_DIALECT_MIDI_MPE:
		{
			clap_event_midi ev;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.type = CLAP_EVENT_MIDI;
			ev.header.time = static_cast<std::uint32_t>(offset);
			ev.header.flags = 0;
			ev.header.size = sizeof(ev);
			ev.port_index = m_notePorts.portIndex();
			ev.data[0] = static_cast<std::uint8_t>(MidiEventTypes::MidiKeyPressure | channel);
			ev.data[1] = static_cast<std::uint8_t>(key);
			ev.data[2] = static_cast<std::uint8_t>(pressure / 127.0);

			m_evIn.push(&ev.header);
			break;
		}
		default:
			assert(false); // Shouldn't ever happen
			break;
	}
}

auto ClapInstance::process(std::uint32_t frames) -> bool
{
	assert(ClapThreadCheck::isAudioThread());
	if (!m_plugin) { return false; }

	// Can't process a plugin that is not active
	if (!isActive()) { return false; }

	// Do we want to deactivate the plugin?
	if (m_scheduleDeactivate)
	{
		qDebug() << "ClapInstance::process - Schedule deactivate";
		m_scheduleDeactivate = false;
		if (m_pluginState == PluginState::ActiveAndProcessing)
		{
			qDebug() << "ClapInstance::process - stop_processing()";
			m_plugin->stop_processing(m_plugin);
		}
		setPluginState(PluginState::ActiveAndReadyToDeactivate);
		return true;
	}

	// We can't process a plugin which failed to start processing
	if (m_pluginState == PluginState::ActiveWithError) { return false; }

	m_process.in_events = m_evIn.clapInputEvents();
	m_process.out_events = m_evOut.clapOutputEvents();

	m_evOut.clear();
	m_params.generatePluginInputEvents();

	if (isSleeping())
	{
		if (!m_scheduleProcess && m_evIn.empty())
		{
			// The plugin is sleeping, there is no request to wake it up
			// and there are no events to process
			return true;
		}

		m_scheduleProcess = false;
		if (!m_plugin->start_processing(m_plugin))
		{
			// The plugin failed to start processing
			setPluginState(PluginState::ActiveWithError);
			return false;
		}

		setPluginState(PluginState::ActiveAndProcessing);
	}

	[[maybe_unused]] int32_t status = CLAP_PROCESS_SLEEP;
	if (isProcessing())
	{
		status = m_plugin->process(m_plugin, &m_process);
	}

	m_params.handlePluginOutputEvents();

	m_evOut.clear();
	m_evIn.clear();

	m_params.processEnd();

	// TODO: send plugin to sleep if possible

	return true;
}

auto ClapInstance::processEnd(std::uint32_t frames) -> bool
{
	m_steadyTime += frames;
	m_process.frames_count = frames;
	m_process.steady_time = m_steadyTime;
	return false;
}

auto ClapInstance::isActive() const -> bool
{
	switch (m_pluginState)
	{
	case PluginState::ActiveAndSleeping: [[fallthrough]];
	case PluginState::ActiveWithError: [[fallthrough]];
	case PluginState::ActiveAndProcessing: [[fallthrough]];
	case PluginState::ActiveAndReadyToDeactivate:
		return true;
	default:
		return false;
	}
}

auto ClapInstance::isProcessing() const -> bool
{
	return m_pluginState == PluginState::ActiveAndProcessing;
}

auto ClapInstance::isSleeping() const -> bool
{
	return m_pluginState == PluginState::ActiveAndSleeping;
}

auto ClapInstance::isErrorState() const -> bool
{
	return m_pluginState == PluginState::None
		|| m_pluginState == PluginState::LoadedWithError
		|| m_pluginState == PluginState::InactiveWithError
		|| m_pluginState == PluginState::ActiveWithError;
}

void ClapInstance::log(clap_log_severity severity, const char* msg) const
{
	// Thread-safe
	if ((severity == CLAP_LOG_DEBUG || severity == CLAP_LOG_INFO)
		&& !ClapManager::debugging()) { return; }

	std::string_view severityStr;
	switch (severity)
	{
	case CLAP_LOG_DEBUG:
		severityStr = "DEBUG"; break;
	case CLAP_LOG_INFO:
		severityStr = "INFO"; break;
	case CLAP_LOG_WARNING:
		severityStr = "WARNING"; break;
	case CLAP_LOG_ERROR:
		severityStr = "ERROR"; break;
	case CLAP_LOG_FATAL:
		severityStr = "FATAL"; break;
	case CLAP_LOG_HOST_MISBEHAVING:
		severityStr = "HOST_MISBEHAVING"; break;
	case CLAP_LOG_PLUGIN_MISBEHAVING:
		severityStr = "PLUGIN_MISBEHAVING"; break;
	default:
		severityStr = "UNKNOWN"; break;
	}

	qDebug().nospace() << "[" << severityStr.data() << "] [" << info().descriptor()->id << "] [CLAP] - " << msg;
}

auto ClapInstance::fromHost(const clap_host* host) -> ClapInstance*
{
	// TODO: Return nullptr instead of throwing exceptions?
	if (!host) { throw std::invalid_argument{"Passed a null host pointer"}; }

	auto h = static_cast<ClapInstance*>(host->host_data);
	if (!h) { throw std::invalid_argument{"Passed an invalid host pointer because the host_data is null"}; }

	if (!h->plugin())
	{
		throw std::logic_error{"The plugin can't query for extensions during the create method. Wait "
			"for clap_plugin.init() call."};
	}

	return h;
}

template<typename... Args>
void ClapInstance::checkPluginStateCurrent(Args... possibilities)
{
	assert(((m_pluginState == possibilities) || ...));
}

auto ClapInstance::isPluginNextStateValid(PluginState next) -> bool
{
	switch (next)
	{
	case PluginState::None:
		return m_pluginState == PluginState::Inactive
			|| m_pluginState == PluginState::InactiveWithError
			|| m_pluginState == PluginState::Loaded
			|| m_pluginState == PluginState::LoadedWithError
			|| m_pluginState == PluginState::None; // TODO: NoneWithError?
	case PluginState::Loaded:
		return m_pluginState == PluginState::None;
	case PluginState::LoadedWithError:
		return m_pluginState == PluginState::Loaded;
	case PluginState::Inactive:
		return m_pluginState == PluginState::Loaded
			|| m_pluginState == PluginState::ActiveAndReadyToDeactivate;
	case PluginState::InactiveWithError:
		return m_pluginState == PluginState::Inactive;
	case PluginState::ActiveAndSleeping:
		return m_pluginState == PluginState::Inactive
			|| m_pluginState == PluginState::ActiveAndProcessing;
	case PluginState::ActiveAndProcessing:
		return m_pluginState == PluginState::ActiveAndSleeping;
	case PluginState::ActiveWithError:
		return m_pluginState == PluginState::ActiveAndProcessing;
	case PluginState::ActiveAndReadyToDeactivate:
		return m_pluginState == PluginState::ActiveAndProcessing
			|| m_pluginState == PluginState::ActiveAndSleeping
			|| m_pluginState == PluginState::ActiveWithError;
		break;
	default:
		throw std::runtime_error{"CLAP plugin state error"};
	}
	return false;
}

void ClapInstance::setPluginState(PluginState state)
{
	// Assert that it's okay to transition to the desired next state from the current state
	assert(isPluginNextStateValid(state) && "Invalid state transition");

	m_pluginState = state;
	switch (state)
	{
		case PluginState::None:
			qDebug() << "Set state to None"; break;
		case PluginState::Loaded:
			qDebug() << "Set state to Loaded"; break;
		case PluginState::LoadedWithError:
			qDebug() << "Set state to LoadedWithError"; break;
		case PluginState::Inactive:
			qDebug() << "Set state to Inactive"; break;
		case PluginState::InactiveWithError:
			qDebug() << "Set state to InactiveWithError"; break;
		case PluginState::ActiveAndSleeping:
			qDebug() << "Set state to ActiveAndSleeping"; break;
		case PluginState::ActiveAndProcessing:
			qDebug() << "Set state to ActiveAndProcessing"; break;
		case PluginState::ActiveWithError:
			qDebug() << "Set state to ActiveWithError"; break;
		case PluginState::ActiveAndReadyToDeactivate:
			qDebug() << "Set state to ActiveAndReadyToDeactivate"; break;
	}
}

template<typename T, class F>
auto ClapInstance::pluginExtensionInit(const T*& ext, const char* id, F* checkFunc) -> bool
{
	static_assert(std::is_same_v<F, bool(const T*) noexcept> || std::is_same_v<F, bool(const T*)>);
	assert(ClapThreadCheck::isMainThread());
	assert(ext == nullptr && "Plugin extension already initialized");
	ext = static_cast<const T*>(m_plugin->get_extension(m_plugin, id));
	if (!ext) { return false; }
	if (!checkFunc(ext))
	{
		// Extension doesn't implement the functions required by LMMS
		ext = nullptr;
		return false;
	}
	return true;
}

////////////////////////////////
// ClapInstance host
////////////////////////////////

void ClapInstance::idle()
{
	assert(ClapThreadCheck::isMainThread());
	if (isErrorState()) { return; }

	m_params.idle();

	if (m_scheduleMainThreadCallback)
	{
		m_scheduleMainThreadCallback = false;
		m_plugin->on_main_thread(m_plugin);
	}

	if (m_scheduleRestart)
	{
		deactivate();
		m_scheduleRestart = false;
		activate();
	}
}

void ClapInstance::setHost()
{
	m_host.host_data = this;
	m_host.clap_version = CLAP_VERSION;
	m_host.name = "LMMS";
	m_host.version = LMMS_VERSION;
	m_host.vendor = "";
	m_host.url = "https://lmms.io/";
	m_host.get_extension = &hostGetExtension;
	m_host.request_callback = &hostRequestCallback;
	m_host.request_process = &hostRequestProcess;
	m_host.request_restart = &hostRequestRestart;
}

auto ClapInstance::hostGetExtension(const clap_host* host, const char* extensionId) -> const void*
{
	auto h = fromHost(host);
	if (!h || !extensionId) { return nullptr; }

	if (ClapManager::debugging()) { qDebug() << "--Plugin requested host extension:" << extensionId; }

	const auto id = std::string_view{extensionId};
	//if (id == CLAP_EXT_AUDIO_PORTS)   { return h->audioPorts().hostExt(); }
	if (id == CLAP_EXT_GUI)           { return h->gui().hostExt(); }
	if (id == CLAP_EXT_LATENCY)       { return &s_hostExtLatency; }
	if (id == CLAP_EXT_LOG)           { return &s_hostExtLog; }
	if (id == CLAP_EXT_NOTE_PORTS)    { return h->notePorts().hostExt(); }
	if (id == CLAP_EXT_PARAMS)        { return h->params().hostExt(); }
	if (id == CLAP_EXT_STATE)         { return h->state().hostExt(); }
	if (id == CLAP_EXT_THREAD_CHECK)  { return ClapThreadCheck::hostExt(); }
	if (id == CLAP_EXT_THREAD_POOL)   { return h->threadPool().hostExt(); }
	if (id == CLAP_EXT_TIMER_SUPPORT) { return h->timerSupport().hostExt(); }

	return nullptr;
}

void ClapInstance::hostRequestCallback(const clap_host* host)
{
	qDebug() << "ClapInstance::hostRequestCallback";
	const auto h = fromHost(host);
	h->m_scheduleMainThreadCallback = true;
}

void ClapInstance::hostRequestProcess(const clap_host* host)
{
	qDebug() << "ClapInstance::hostRequestProcess";
	auto h = fromHost(host);
	h->m_scheduleProcess = true;
}

void ClapInstance::hostRequestRestart(const clap_host* host)
{
	qDebug() << "ClapInstance::hostRequestRestart";
	auto h = fromHost(host);
	h->m_scheduleRestart = true;
}

void ClapInstance::hostExtLogLog(const clap_host* host, clap_log_severity severity, const char* msg)
{
	// Thread-safe
	auto h = fromHost(host);
	h->log(severity, msg);
}

void ClapInstance::hostExtLatencyChanged([[maybe_unused]] const clap_host* host)
{
	/*
	 * LMMS currently does not use latency data, but implementing this extension
	 * fixes a crash that would occur in plugins built using the DISTRHO plugin
	 * framework prior to this commit:
	 * https://github.com/DISTRHO/DPF/commit/4f11f8cc49b24ede1735a16606e7bad5a52ab41d
	 */
	//qDebug() << "ClapInstance::hostExtLatencyChanged";
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
