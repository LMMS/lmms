/*
 * ClapInstance.cpp - Implementation of ClapInstance class
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

#include "ClapInstance.h"

#ifdef LMMS_HAVE_CLAP

#include <QApplication>
#include <QDomDocument>
#include <QDomElement>
#include <algorithm>
#include <cassert>
#include <thread>

#include "AudioEngine.h"
#include "ClapManager.h"
#include "ClapTransport.h"
#include "Engine.h"
#include "lmmsversion.h"
#include "MidiEvent.h"

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

auto ClapInstance::create(const std::string& pluginId, Model* parent) -> std::unique_ptr<ClapInstance>
{
	// CLAP API requires main thread for plugin loading
	assert(ClapThreadCheck::isMainThread());

	const auto manager = Engine::getClapManager();
	const auto info = manager->pluginInfo(pluginId);
	if (!info)
	{
		std::string msg = "No plugin found for ID \"" + pluginId + "\"";
		ClapLog::globalLog(CLAP_LOG_ERROR, msg);
		return nullptr;
	}

	ClapTransport::update();

	ClapLog::globalLog(CLAP_LOG_DEBUG, "Creating CLAP instance");

	auto instance = std::make_unique<ClapInstance>(Access{}, *info, parent);
	if (!instance->start())
	{
		ClapLog::globalLog(CLAP_LOG_ERROR, "Failed instantiating CLAP instance");
		return nullptr;
	}

	return instance;
}

ClapInstance::ClapInstance(Access, const ClapPluginInfo& pluginInfo, Model* parent)
	: QObject{parent}
	, m_pluginInfo{pluginInfo}
	, m_host {
		CLAP_VERSION,         // clap_version
		this,                 // host_data // NOTE: Need to update if class is copied/moved
		"LMMS",               // name
		"LMMS contributors",  // vendor
		"https://lmms.io/",   // url
		LMMS_VERSION,         // version
		&clapGetExtension,    // get_extension
		&clapRequestCallback, // request_callback
		&clapRequestProcess,  // request_process
		&clapRequestRestart   // request_restart
	}
	, m_midiInputBuf{s_maxMidiInputEvents}
	, m_midiInputReader{m_midiInputBuf}
	, m_params{parent, this, &m_evIn, &m_evOut}
	, m_presetLoader{parent, this}
{
	m_process.steady_time = 0;
	m_process.transport = ClapTransport::get();
}

ClapInstance::~ClapInstance()
{
#if 0
	logger().log(CLAP_LOG_DEBUG, "ClapInstance::~ClapInstance");
#endif

	destroy();
}

void ClapInstance::copyModelsFromCore()
{
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
	//m_params.rescan(CLAP_PARAM_RESCAN_VALUES);
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

	if (!hasNoteInput()) { return; }

	const auto ev = MidiInputEvent{event, time, offset};

	// Acquire lock and spin
	while (m_ringLock.test_and_set(std::memory_order_acquire)) {}

	const auto written = m_midiInputBuf.write(&ev, 1);

	m_ringLock.clear(std::memory_order_release);

	if (written != 1)
	{
		logger().log(CLAP_LOG_WARNING, "MIDI ringbuffer is too small! Discarding MIDI event.");
	}
}

auto ClapInstance::controlCount() const -> std::size_t
{
	return m_params.automatableCount(); // TODO: + 1 control if port config != Stereo?
}

auto ClapInstance::hasNoteInput() const -> bool
{
	return m_notePorts.hasInput();
}

void ClapInstance::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	elem.setAttribute("version", "0");

	audioPorts().saveSettings(doc, elem);
	params().saveParamConnections(doc, elem);

	// The CLAP standard strongly recommends using the state extension
	//     instead of manually saving parameter values
	if (!state().supported()) { return; }

	// TODO: Integrate save/load context into LMMS better
	const auto context = elem.ownerDocument().doctype().name() == "clonedtrack"
		? ClapState::Context::Duplicate
		: ClapState::Context::Project;

	const auto savedState = state().save(context).value_or("");
	elem.setAttribute("state", QString::fromUtf8(savedState.data(), savedState.size()));
}

void ClapInstance::loadSettings(const QDomElement& elem)
{
	[[maybe_unused]] const auto version = elem.attribute("version", "0").toInt();

	audioPorts().loadSettings(elem);
	params().loadParamConnections(elem);

	// The CLAP standard strongly recommends using the state extension
	//     instead of manually saving parameter values
	if (!state().supported()) { return; }

	// TODO: Integrate save/load context into LMMS better
	const auto context = elem.ownerDocument().doctype().name() == "clonedtrack"
		? ClapState::Context::Duplicate
		: ClapState::Context::Project;

	const auto savedState = elem.attribute("state", "").toStdString();
	if (!state().load(savedState, context)) { return; }

	// Parameters may have changed in the plugin;
	// Those values need to be reflected in host
	params().rescan(CLAP_PARAM_RESCAN_VALUES); // TODO: Is this correct?
}

void ClapInstance::destroy()
{
	//idle(); // TODO: ??? May throw an exception, which should not happen in ClapInstance dtor

	unload();
}

auto ClapInstance::isValid() const -> bool
{
	return m_plugin != nullptr && !isErrorState();
}

auto ClapInstance::start() -> bool
{
	if (!load()) { return false; }
	if (!init()) { return false; }
	return activate();
}

auto ClapInstance::restart() -> bool
{
#if 0
	{
		std::string msg = "Restarting plugin instance: " + std::string{m_pluginInfo->descriptor()->name};
		logger().log(CLAP_LOG_INFO, msg);
	}
#endif

	if (!deactivate()) { return false; }
	return activate();
}

auto ClapInstance::load() -> bool
{
#if 0
	{
		std::string msg = "Loading plugin instance: " + std::string{m_pluginInfo->descriptor()->name};
		logger().log(CLAP_LOG_INFO, msg);
	}
#endif

	assert(isCurrentStateValid(PluginState::None));

	// Create plugin instance, destroying any previous plugin instance first
	const auto& factory = info().factory();
	m_plugin = factory.create_plugin(&factory, host(), info().descriptor().id);
	if (!m_plugin)
	{
		logger().log(CLAP_LOG_ERROR, "Failed to create plugin instance");
		// TODO: Set state to NoneWithError?
		return false;
	}

	setPluginState(PluginState::Loaded);
	return true;
}

auto ClapInstance::unload() -> bool
{
#if 0
	{
		std::string msg = "Unloading plugin instance: " + std::string{m_pluginInfo->descriptor()->name};
		logger().log(CLAP_LOG_INFO, msg);
	}
#endif

	assert(ClapThreadCheck::isMainThread());

	// Deinitialize extensions
	m_audioPorts.deinit();
	//m_gui.deinit();
	m_log.deinit();
	m_notePorts.deinit();
	m_params.deinit();
	m_presetLoader.deinit();
	m_state.deinit();
	m_timerSupport.deinit();

	deactivate();

	if (m_plugin)
	{
		m_plugin->destroy(m_plugin);
		m_plugin = nullptr;
	}

	setPluginState(PluginState::None);
	return true;
}

auto ClapInstance::init() -> bool
{
	assert(ClapThreadCheck::isMainThread());

	if (isErrorState()) { return false; }
	assert(isCurrentStateValid(PluginState::Loaded));

	if (m_pluginState != PluginState::Loaded) { return false; }

	m_audioPorts.beginPluginInit();
	m_gui.beginPluginInit();
	m_log.beginPluginInit();
	m_notePorts.beginPluginInit();
	m_params.beginPluginInit();
	m_presetLoader.beginPluginInit();
	m_state.beginPluginInit();
	m_threadCheck.beginPluginInit();
	m_timerSupport.beginPluginInit();

	const bool success = m_plugin->init(m_plugin);

	m_audioPorts.endPluginInit();
	m_gui.endPluginInit();
	m_log.endPluginInit();
	m_notePorts.endPluginInit();
	m_params.endPluginInit();
	m_presetLoader.endPluginInit();
	m_state.endPluginInit();
	m_threadCheck.endPluginInit();
	m_timerSupport.endPluginInit();

	if (!success)
	{
		{
			std::string msg = "Could not init the plugin with id: " + std::string{info().descriptor().id};
			logger().log(CLAP_LOG_ERROR, msg);
		}
		setPluginState(PluginState::LoadedWithError);
		m_plugin->destroy(m_plugin);
		m_plugin = nullptr;
		return false;
	}

	if (!m_audioPorts.init(m_process))
	{
		setPluginState(PluginState::LoadedWithError);
		return false;
	}

	// TODO: What if this is the 2nd instance of a mono plugin?
	//m_gui.init();

	m_notePorts.init();
	if (!hasNoteInput() && info().type() == Plugin::Type::Instrument)
	{
		logger().log(CLAP_LOG_WARNING, "Plugin is instrument but doesn't implement note ports extension");
	}

	if (!m_params.init())
	{
		logger().log(CLAP_LOG_DEBUG, "Plugin does not support params extension");
	}

	m_presetLoader.init();
	m_state.init();
	m_timerSupport.init();

	setPluginState(PluginState::Inactive);

	return true;
}

auto ClapInstance::activate() -> bool
{
	assert(ClapThreadCheck::isMainThread());

	if (isErrorState()) { return false; }
	assert(isCurrentStateValid(PluginState::Inactive));

	const auto sampleRate = static_cast<double>(Engine::audioEngine()->outputSampleRate());
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
			assert(false);
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
		case CLAP_NOTE_DIALECT_MIDI:
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
			assert(false);
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
		m_scheduleDeactivate = false;
		if (m_pluginState == PluginState::ActiveAndProcessing)
		{
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
	m_process.frames_count = frames;
	m_process.steady_time += frames;
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

void ClapInstance::setPluginState(PluginState state)
{
	// Assert that it's okay to transition to the desired next state from the current state
	assert(isNextStateValid(state) && "Invalid state transition");

	m_pluginState = state;
	if (!ClapManager::debugging()) { return; }

	switch (state)
	{
		case PluginState::None:
			logger().log(CLAP_LOG_DEBUG, "Set state to None"); break;
		case PluginState::Loaded:
			logger().log(CLAP_LOG_DEBUG, "Set state to Loaded"); break;
		case PluginState::LoadedWithError:
			logger().log(CLAP_LOG_DEBUG, "Set state to LoadedWithError"); break;
		case PluginState::Inactive:
			logger().log(CLAP_LOG_DEBUG, "Set state to Inactive"); break;
		case PluginState::InactiveWithError:
			logger().log(CLAP_LOG_DEBUG, "Set state to InactiveWithError"); break;
		case PluginState::ActiveAndSleeping:
			logger().log(CLAP_LOG_DEBUG, "Set state to ActiveAndSleeping"); break;
		case PluginState::ActiveAndProcessing:
			logger().log(CLAP_LOG_DEBUG, "Set state to ActiveAndProcessing"); break;
		case PluginState::ActiveWithError:
			logger().log(CLAP_LOG_DEBUG, "Set state to ActiveWithError"); break;
		case PluginState::ActiveAndReadyToDeactivate:
			logger().log(CLAP_LOG_DEBUG, "Set state to ActiveAndReadyToDeactivate"); break;
	}
}

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

auto ClapInstance::isNextStateValid(PluginState next) const -> bool
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

auto ClapInstance::clapGetExtension(const clap_host* host, const char* extensionId) -> const void*
{
	auto h = detail::ClapExtensionHelper::fromHost(host);
	if (!h || !extensionId) { return nullptr; }

#if 0
	{
		std::string msg = "Plugin requested host extension: ";
		msg += (extensionId ? extensionId : "(NULL)");
		h->logger().log(CLAP_LOG_DEBUG, msg);
	}
#endif

	if (h->m_pluginState == PluginState::None)
	{
		h->logger().log(CLAP_LOG_PLUGIN_MISBEHAVING, "Plugin may not request host extension before plugin.init()");
		return nullptr;
	}

	const auto id = std::string_view{extensionId};
	//if (id == CLAP_EXT_AUDIO_PORTS)   { return h->audioPorts().hostExt(); }
	//if (id == CLAP_EXT_GUI)           { return h->gui().hostExt(); }
	if (id == CLAP_EXT_LATENCY)       { return &s_clapLatency; }
	if (id == CLAP_EXT_LOG)           { return h->logger().hostExt(); }
	if (id == CLAP_EXT_NOTE_PORTS)    { return h->notePorts().hostExt(); }
	if (id == CLAP_EXT_PARAMS)        { return h->params().hostExt(); }
	if (id == CLAP_EXT_PRESET_LOAD)   { return h->presetLoader().hostExt(); }
	if (id == CLAP_EXT_PRESET_LOAD_COMPAT) { return h->presetLoader().hostExt(); }
	if (id == CLAP_EXT_STATE)         { return h->state().hostExt(); }
	if (id == CLAP_EXT_THREAD_CHECK)  { return h->m_threadCheck.hostExt(); }
	if (id == CLAP_EXT_TIMER_SUPPORT) { return h->timerSupport().hostExt(); }

	return nullptr;
}

void ClapInstance::clapRequestCallback(const clap_host* host)
{
	const auto h = detail::ClapExtensionHelper::fromHost(host);
	if (!h) { return; }
	h->m_scheduleMainThreadCallback = true;
}

void ClapInstance::clapRequestProcess(const clap_host* host)
{
	auto h = detail::ClapExtensionHelper::fromHost(host);
	if (!h) { return; }
	h->m_scheduleProcess = true;
}

void ClapInstance::clapRequestRestart(const clap_host* host)
{
	auto h = detail::ClapExtensionHelper::fromHost(host);
	if (!h) { return; }
	h->m_scheduleRestart = true;
	h->logger().log(CLAP_LOG_DEBUG, ClapThreadCheck::isAudioThread()
		? "Req. restart on Audio thread" : "Req. restart on Main thread");
}

void ClapInstance::clapLatencyChanged([[maybe_unused]] const clap_host* host)
{
	/*
	 * LMMS currently does not use latency data, but implementing this extension
	 * fixes a crash that would occur in plugins built using the DISTRHO plugin
	 * framework prior to this commit:
	 * https://github.com/DISTRHO/DPF/commit/4f11f8cc49b24ede1735a16606e7bad5a52ab41d
	 */
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
