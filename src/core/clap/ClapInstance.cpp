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
#include "Engine.h"
#include "AudioEngine.h"
#include "MidiEvent.h"

#include <QApplication>
#include <QThread>
#include <QDebug>

#include <algorithm>
#include <cassert>

#include "lmmsversion.h"

#include <clap/helpers/reducing-param-queue.hxx>

namespace lmms
{

namespace
{
	template<bool stereo>
	inline void copyBuffersHostToPlugin(const sampleFrame* hostBuf, float** pluginBuf,
		unsigned channel, fpp_t frames)
	{
		for (fpp_t f = 0; f < frames; ++f)
		{
			pluginBuf[0][f] = hostBuf[f][channel];
			if constexpr (stereo)
				pluginBuf[1][f] = hostBuf[f][channel + 1];
		}
	}

	inline void copyBuffersStereoHostToMonoPlugin(const sampleFrame* hostBuf, float** pluginBuf,
		unsigned channel, fpp_t frames)
	{
		for (fpp_t f = 0; f < frames; ++f)
		{
			pluginBuf[0][f] = (hostBuf[f][channel] + hostBuf[f][channel + 1]) / 2.0f;
		}
	}

	template<bool stereo>
	inline void copyBuffersPluginToHost(float** pluginBuf, sampleFrame* hostBuf,
		std::uint64_t constantMask, unsigned channel, fpp_t frames)
	{
		const bool isLeftConstant = (constantMask & (1 << 0)) != 0;
		if constexpr (stereo)
		{
			const bool isRightConstant = (constantMask & (1 << 1)) != 0;
			for (fpp_t f = 0; f < frames; ++f)
			{
				hostBuf[f][channel] = pluginBuf[0][isLeftConstant ? 0 : f];
				hostBuf[f][channel + 1] = pluginBuf[1][isRightConstant ? 0 : f];
			}
		}
		else
		{
			for (fpp_t f = 0; f < frames; ++f)
			{
				hostBuf[f][channel] = pluginBuf[0][isLeftConstant ? 0 : f];
			}
		}
	}

	inline void copyBuffersMonoPluginToStereoHost(float** pluginBuf, sampleFrame* hostBuf,
		std::uint64_t constantMask, unsigned channel, fpp_t frames)
	{
		const bool isConstant = (constantMask & (1 << 0)) != 0;
		for (fpp_t f = 0; f < frames; ++f)
		{
			hostBuf[f][channel] = hostBuf[f][channel + 1] = pluginBuf[0][isConstant ? 0 : f];
		}
	}
}


////////////////////////////////
// ClapInstance
////////////////////////////////

ClapInstance::ClapInstance(const ClapPluginInfo* pluginInfo, Model* parent)
	: LinkedModelGroup(parent), m_pluginInfo(pluginInfo)
{
	m_pluginState = PluginState::None;
	setHost();

	m_process = {};
	m_process.steady_time = -1; // Not supported yet

	if (!pluginLoad())
		return;
	if (!pluginInit())
		return;
	if (!pluginActivate())
		return;
}

ClapInstance::ClapInstance(ClapInstance&& other) noexcept
	: LinkedModelGroup(other.parentModel()),
	m_pluginInfo(std::move(other.m_pluginInfo)),
	m_pluginIssues(std::move(other.m_pluginIssues))
{
	qDebug() << "TODO: Move constructor not fully implemented yet";
	m_idleQueue = std::move(other.m_idleQueue);
	m_plugin = std::exchange(other.m_plugin, nullptr);

	// Update the host's host_data pointer
	setHost();
}

ClapInstance::~ClapInstance()
{
	qDebug() << "ClapInstance::~ClapInstance";
	destroy();
}

void ClapInstance::copyModelsFromCore()
{
	//
}

void ClapInstance::copyModelsToCore()
{
	//
}

void ClapInstance::copyBuffersFromCore(const sampleFrame* buf, unsigned firstChan, unsigned num, fpp_t frames)
{
	// LMMS to CLAP
	if (num > 1)
	{
		if (!isMonoInput())
		{
			// Stereo LMMS to Stereo CLAP
			copyBuffersHostToPlugin<true>(buf, m_audioInActive->data32, firstChan, frames);
		}
		else
		{
			// Stereo LMMS to Mono CLAP
			copyBuffersStereoHostToMonoPlugin(buf, m_audioInActive->data32, firstChan, frames);
		}
	}
	else
	{
		// Mono LMMS to Mono CLAP
		copyBuffersHostToPlugin<false>(buf, m_audioInActive->data32, firstChan, frames);
	}
}

void ClapInstance::copyBuffersToCore(sampleFrame* buf, unsigned firstChan, unsigned num, fpp_t frames) const
{
	// CLAP to LMMS
	if (num > 1)
	{
		if (!isMonoOutput())
		{
			// Stereo CLAP to Stereo LMMS
			copyBuffersPluginToHost<true>(m_audioOutActive->data32, buf, m_audioOutActive->constant_mask, firstChan, frames);
		}
		else
		{
			// Mono CLAP to Stereo LMMS
			copyBuffersMonoPluginToStereoHost(m_audioOutActive->data32, buf, m_audioOutActive->constant_mask, firstChan, frames);
		}
	}
	else
	{
		// Mono CLAP to Mono LMMS
		copyBuffersPluginToHost<false>(m_audioOutActive->data32, buf, m_audioOutActive->constant_mask, firstChan, frames);
	}
}

void ClapInstance::run(fpp_t frames)
{
	pluginProcessBegin(frames);
	pluginProcess(frames);
	pluginProcessEnd(frames);
}

void ClapInstance::handleMidiInputEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset)
{
	//
}

auto ClapInstance::hasNoteInput() const -> bool
{
	//
	return false;
}

void ClapInstance::destroy()
{
	hostIdle(); // ???

	pluginUnload();

	hostDestroy();
}

auto ClapInstance::isValid() const -> bool
{
	// TODO: Check if state is not None?
	return m_plugin != nullptr && !isPluginErrorState() && m_pluginIssues.empty();
}

auto ClapInstance::pluginLoad() -> bool
{
	qDebug() << "Loading plugin instance:" << m_pluginInfo->getDescriptor()->name;
	checkPluginStateCurrent(PluginState::None);
	checkPluginStateNext(PluginState::Loaded); // success
	checkPluginStateNext(PluginState::None); // failure (stay None)

	// Create plugin instance, destroying any previous plugin instance first
	const auto factory = m_pluginInfo->getFactory();
	m_plugin = factory->create_plugin(factory, getHost(), m_pluginInfo->getDescriptor()->id);
	if (!m_plugin)
	{
		qWarning() << "Failed to create instance of CLAP plugin";
		hostDestroy();
		return false;
	}

	setPluginState(PluginState::Loaded);
	return true;
}

auto ClapInstance::pluginUnload() -> bool
{
	pluginDeactivate();

	if (m_plugin)
	{
		m_plugin->destroy(m_plugin);
		m_plugin = nullptr;
	}

	setPluginState(PluginState::None);
	return true;
}

auto ClapInstance::pluginInit() -> bool
{
	qDebug() << "ClapInstance::pluginInit()";
	if (isPluginErrorState())
		return false;
	checkPluginStateCurrent(PluginState::Loaded);
	checkPluginStateNext(PluginState::Inactive); // success
	checkPluginStateNext(PluginState::LoadedWithError); // failure (init fail)

	if (getPluginState() != PluginState::Loaded)
		return false;

	if (!m_plugin->init(m_plugin))
	{
		qWarning() << "Could not init the plugin with id:" << getInfo().getDescriptor()->id;
		setPluginState(PluginState::LoadedWithError);
		m_plugin->destroy(m_plugin);
		return false;
	}

	// Clear everything just to be safe
	m_pluginIssues.clear();
	m_audioIn.reset();
	m_audioOut.reset();
	m_audioInActive = m_audioOutActive = nullptr;
	m_audioPortsIn.clear();
	m_audioPortsOut.clear();
	m_audioPortInActive = m_audioPortOutActive = nullptr;
	m_audioInBuffers.clear();
	m_audioOutBuffers.clear();

	// TODO: Need to init extensions before activating the plugin
	if (!pluginExtensionInit(m_pluginExtAudioPorts, CLAP_EXT_AUDIO_PORTS))
	{
		qWarning() << "The required CLAP audio port extension is not supported by the plugin";
		setPluginState(PluginState::LoadedWithError);
		return false;
	}

	// Effect, Instrument, and Tool are the only options
	const bool needInputPort = m_pluginInfo->getType() != Plugin::PluginTypes::Instrument;
	const bool needOutputPort = m_pluginInfo->getType() != Plugin::PluginTypes::Tool;

	auto readPorts = [this, needInputPort, needOutputPort](
		std::vector<AudioPort>& audioPorts,
		std::unique_ptr<clap_audio_buffer[]>& audioBuffers,
		std::vector<AudioBuffer>& rawAudioBuffers,
		bool is_input) -> AudioPort*
	{
		const auto portCount = m_pluginExtAudioPorts->count(m_plugin, is_input);

		if (is_input)
		{
			//if (portCount == 0 && m_pluginInfo->getType() == Plugin::PluginTypes::Effect)
			//	m_pluginIssues.emplace_back( ... );
		}
		else
		{
			if (portCount == 0 && needOutputPort)
				m_pluginIssues.emplace_back(PluginIssueType::noOutputChannel);
			//if (portCount > 2)
			//	m_pluginIssues.emplace_back(PluginIssueType::tooManyOutputChannels, std::to_string(outCount));
			
		}

		qDebug() << (is_input ? "Input ports:" : "Output ports:");

		clap_id monoPort = CLAP_INVALID_ID, stereoPort = CLAP_INVALID_ID;
		//clap_id mainPort = CLAP_INVALID_ID;
		for (uint32_t idx = 0; idx < portCount; ++idx)
		{
			clap_audio_port_info info;
			if (!m_pluginExtAudioPorts->get(m_plugin, idx, is_input, &info))
			{
				qWarning() << "Unknown error calling m_pluginExtAudioPorts->get(...)";
				m_pluginIssues.emplace_back(PluginIssueType::portHasNoDef);
				return nullptr;
			}

			qDebug() << "- port id:" << info.id;
			qDebug() << "- port name:" << info.name;
			qDebug() << "- port flags:" << info.flags;
			qDebug() << "- port channel_count:" << info.channel_count;
			qDebug() << "- port type:" << info.port_type;
			qDebug() << "- port in place pair:" << info.in_place_pair;

			if (idx == 0 && !(info.flags & CLAP_AUDIO_PORT_IS_MAIN))
			{
				qDebug() << "CLAP plugin audio port #0 is not main";
			}

			//if (info.flags & CLAP_AUDIO_PORT_IS_MAIN)
			//	mainPort = idx;

			AudioPortType type = AudioPortType::Unsupported;

			if (info.port_type)
			{
				if (strcmp(CLAP_PORT_MONO, info.port_type) == 0)
				{
					assert(info.channel_count == 1);
					type = AudioPortType::Mono;
					if (monoPort == CLAP_INVALID_ID)
						monoPort = idx;
				}

				if (strcmp(CLAP_PORT_STEREO, info.port_type) == 0)
				{
					assert(info.channel_count == 2);
					type = AudioPortType::Stereo;
					if (stereoPort == CLAP_INVALID_ID)
						stereoPort = idx;
				}
			}
			else
			{
				if (info.channel_count == 1)
				{
					type = AudioPortType::Mono;
					if (monoPort == CLAP_INVALID_ID)
						monoPort = idx;
				}
				else if (info.channel_count == 2)
				{
					type = AudioPortType::Stereo;
					if (stereoPort == CLAP_INVALID_ID)
						stereoPort = idx;
				}
			}

			audioPorts.emplace_back(AudioPort{info, idx, is_input, type, false});
		}

		if (is_input && !needInputPort)
			return nullptr;
		if (!is_input && !needOutputPort)
			return nullptr;

		assert(portCount == audioPorts.size());
		audioBuffers = std::make_unique<clap_audio_buffer[]>(audioPorts.size());
		for (uint32_t port = 0; port < audioPorts.size(); ++port)
		{
			const auto channelCount = audioPorts[port].info.channel_count;
			if (channelCount <= 0) { return nullptr; }

			audioBuffers[port].channel_count = channelCount;

			if (is_input && ((port != stereoPort && stereoPort != CLAP_INVALID_ID) || (port != monoPort && stereoPort == CLAP_INVALID_ID)))
			{
				// This input port will not be used by LMMS
				// TODO: Will a mono port ever need to be used if a stereo port is available?
				constexpr uint32_t maxChannels = sizeof(clap_audio_buffer::constant_mask) * 8;
				audioBuffers[port].constant_mask = (1u << std::min(maxChannels, channelCount)) - 1u;
			}
			else
			{
				audioBuffers[port].constant_mask = 0;
			}

			auto& rawBuffer = rawAudioBuffers.emplace_back(channelCount, DEFAULT_BUFFER_SIZE);
			audioBuffers[port].data32 = rawBuffer.data();
			audioBuffers[port].data64 = nullptr;
			audioBuffers[port].latency = 0; // TODO
		}

		if (stereoPort != CLAP_INVALID_ID)
		{
			if (is_input) { m_monoInput = false; } else { m_monoOutput = false; }
			auto port = &audioPorts[stereoPort];
			port->used = true;
			return port;
		}

		if (monoPort != CLAP_INVALID_ID)
		{
			if (is_input) { m_monoInput = true; } else { m_monoOutput = true; }
			auto port = &audioPorts[monoPort];
			port->used = true;
			return port;
		}

		// Missing a required port type that LMMS supports - i.e. an effect where the only input is surround sound
		qWarning() << "An" << (is_input ? "input" : "output") << "port is required, but CLAP plugin has none that are usable";
		m_pluginIssues.emplace_back(PluginIssueType::unknownPortType);
		return nullptr;
	};

	m_audioPortInActive = readPorts(m_audioPortsIn, m_audioIn, m_audioInBuffers, true);
	if (!m_pluginIssues.empty() || (!m_audioPortInActive && needInputPort))
	{
		setPluginState(PluginState::LoadedWithError);
		return false;
	}

	m_process.audio_inputs = m_audioIn.get();
	m_process.audio_inputs_count = m_audioPortsIn.size();
	m_audioInActive = m_audioPortInActive ? &m_audioIn[m_audioPortInActive->index] : nullptr;

	m_audioPortOutActive = readPorts(m_audioPortsOut, m_audioOut, m_audioOutBuffers, false);
	if (!m_pluginIssues.empty() || (!m_audioPortOutActive && needOutputPort))
	{
		setPluginState(PluginState::LoadedWithError);
		return false;
	}

	m_process.audio_outputs = m_audioOut.get();
	m_process.audio_outputs_count = m_audioPortsOut.size();
	m_audioOutActive = m_audioPortOutActive ? &m_audioOut[m_audioPortOutActive->index] : nullptr;

	// TODO: Temporary
	if (isMonoInput())
		assert(m_audioPortInActive->type == AudioPortType::Mono);
	else
		assert(m_audioPortInActive->type == AudioPortType::Stereo);

	if (isMonoOutput())
		assert(m_audioPortOutActive->type == AudioPortType::Mono);
	else
		assert(m_audioPortOutActive->type == AudioPortType::Stereo);

	// TODO: Then, need to scan params and quick controls:

	//scanParams();
	//scanQuickControls();

	setPluginState(PluginState::Inactive);

	return true;
}

auto ClapInstance::pluginActivate() -> bool
{
	qDebug() << "ClapInstance::pluginActivate()";

	// Must be on main thread
	assert(isMainThread());

	if (isPluginErrorState())
		return false;
	checkPluginStateCurrent(PluginState::Inactive);
	checkPluginStateNext(PluginState::ActiveAndSleeping); // success
	checkPluginStateNext(PluginState::InactiveWithError); // failure (activation fail)

	const auto sampleRate = Engine::audioEngine()->processingSampleRate();
	static_assert(DEFAULT_BUFFER_SIZE > MINIMUM_BUFFER_SIZE);

	assert(!isPluginActive());
	if (!m_plugin->activate(m_plugin, sampleRate, MINIMUM_BUFFER_SIZE, DEFAULT_BUFFER_SIZE))
	{
		setPluginState(PluginState::InactiveWithError);
		return false;
	}

	m_scheduleProcess = true;
	setPluginState(PluginState::ActiveAndSleeping);
	return true;
}

auto ClapInstance::pluginDeactivate() -> bool
{
	qDebug() << "ClapInstance::pluginDeactivate";
	if (!isPluginActive())
		return false;

	// TODO: May cause hang
	/*
	while (isPluginProcessing() || isPluginSleeping())
	{
		m_scheduleDeactivate = true;
		QThread::msleep(10);
	}
	m_scheduleDeactivate = false;
	*/

	m_plugin->deactivate(m_plugin);
	setPluginState(PluginState::Inactive);
	return true;
}

auto ClapInstance::pluginProcessBegin(uint32_t frames) -> bool
{
	m_process.frames_count = frames;
	//m_process.steady_time = m_steadyTime;
	return false;
}

auto ClapInstance::pluginProcess(uint32_t frames) -> bool
{
	//m_steadyTime += frames;

	// Must be audio thread
	if (!m_plugin)
		return false;

	// Can't process a plugin that is not active
	if (!isPluginActive())
		return false;

	// Do we want to deactivate the plugin?
	if (m_scheduleDeactivate)
	{
		m_scheduleDeactivate = false;
		if (m_pluginState == PluginState::ActiveAndProcessing)
			m_plugin->stop_processing(m_plugin);
		checkPluginStateNext(PluginState::ActiveAndReadyToDeactivate);
		setPluginState(PluginState::ActiveAndReadyToDeactivate);
		return true;
	}

	// We can't process a plugin which failed to start processing
	if (m_pluginState == PluginState::ActiveWithError)
		return false;

	m_process.transport = nullptr;

	m_process.in_events = m_evIn.clapInputEvents();
	m_process.out_events = m_evOut.clapOutputEvents();

	m_evOut.clear();
	generatePluginInputEvents();

	if (isPluginSleeping())
	{
		if (!m_scheduleProcess && m_evIn.empty())
		{
			// The plugin is sleeping, there is no request to wake it up and there are no events to
			// process
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
	if (isPluginProcessing())
		status = m_plugin->process(m_plugin, &m_process);

	handlePluginOutputEvents();

	m_evOut.clear();
	m_evIn.clear();

	m_engineToAppValueQueue.producerDone();

	// TODO: send plugin to sleep if possible

	return true;
}

auto ClapInstance::pluginProcessEnd(uint32_t frames) -> bool
{
	m_process.frames_count = frames;
	//m_process.steady_time = m_steadyTime;
	return false;
}

void ClapInstance::generatePluginInputEvents()
{
	m_appToEngineValueQueue.consume(
		[this](clap_id param_id, const AppToEngineParamQueueValue &value) {
			clap_event_param_value ev;
			ev.header.time = 0;
			ev.header.type = CLAP_EVENT_PARAM_VALUE;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.flags = 0;
			ev.header.size = sizeof(ev);
			ev.param_id = param_id;
			ev.cookie = m_hostShouldProvideParamCookie ? value.cookie : nullptr;
			ev.port_index = 0;
			ev.key = -1;
			ev.channel = -1;
			ev.note_id = -1;
			ev.value = value.value;
			m_evIn.push(&ev.header);
		});

	m_appToEngineModQueue.consume([this](clap_id param_id, const AppToEngineParamQueueValue &value) {
		clap_event_param_mod ev;
		ev.header.time = 0;
		ev.header.type = CLAP_EVENT_PARAM_MOD;
		ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
		ev.header.flags = 0;
		ev.header.size = sizeof(ev);
		ev.param_id = param_id;
		ev.cookie = m_hostShouldProvideParamCookie ? value.cookie : nullptr;
		ev.port_index = 0;
		ev.key = -1;
		ev.channel = -1;
		ev.note_id = -1;
		ev.amount = value.value;
		m_evIn.push(&ev.header);
	});
}

void ClapInstance::handlePluginOutputEvents()
{
	for (uint32_t i = 0; i < m_evOut.size(); ++i)
	{
		auto h = m_evOut.get(i);
		switch (h->type)
		{
			case CLAP_EVENT_PARAM_GESTURE_BEGIN:
			{
				auto ev = reinterpret_cast<const clap_event_param_gesture*>(h);
				bool &isAdj = m_isAdjustingParameter[ev->param_id];

				if (isAdj)
					throw std::logic_error("The plugin sent BEGIN_ADJUST twice");
				isAdj = true;

				EngineToAppParamQueueValue v;
				v.has_gesture = true;
				v.is_begin = true;
				m_engineToAppValueQueue.setOrUpdate(ev->param_id, v);
				break;
			}

			case CLAP_EVENT_PARAM_GESTURE_END:
			{
				auto ev = reinterpret_cast<const clap_event_param_gesture*>(h);
				bool &isAdj = m_isAdjustingParameter[ev->param_id];

				if (!isAdj)
				throw std::logic_error("The plugin sent END_ADJUST without a preceding BEGIN_ADJUST");
				isAdj = false;
				EngineToAppParamQueueValue v;
				v.has_gesture = true;
				v.is_begin = false;
				m_engineToAppValueQueue.setOrUpdate(ev->param_id, v);
				break;
			}

			case CLAP_EVENT_PARAM_VALUE:
			{
				auto ev = reinterpret_cast<const clap_event_param_value*>(h);
				EngineToAppParamQueueValue v;
				v.has_value = true;
				v.value = ev->value;
				m_engineToAppValueQueue.setOrUpdate(ev->param_id, v);
				break;
			}
		}
	}
}

void ClapInstance::paramFlushOnMainThread()
{
	assert(isMainThread());
	assert(!isPluginActive());

	m_scheduleParamFlush = false;

	m_evIn.clear();
	m_evOut.clear();

	generatePluginInputEvents();

	///if (canUsePluginParams())
	///	m_pluginParams->flush(m_plugin, m_evIn.clapInputEvents(), m_evOut.clapOutputEvents());
	handlePluginOutputEvents();

	m_evOut.clear();
	m_engineToAppValueQueue.producerDone();
}

auto ClapInstance::isPluginActive() const -> bool
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

auto ClapInstance::isPluginProcessing() const -> bool
{
	return m_pluginState == PluginState::ActiveAndProcessing;
}

auto ClapInstance::isPluginSleeping() const -> bool
{
	return m_pluginState == PluginState::ActiveAndSleeping;
}

auto ClapInstance::isPluginErrorState() const -> bool
{
	return m_pluginState == PluginState::None
		|| m_pluginState == PluginState::LoadedWithError
		|| m_pluginState == PluginState::InactiveWithError
		|| m_pluginState == PluginState::ActiveWithError;
}

void ClapInstance::checkPluginStateNext(PluginState next)
{
	switch (next)
	{
	case PluginState::None:
		assert(m_pluginState == PluginState::Inactive
			|| m_pluginState == PluginState::InactiveWithError
			|| m_pluginState == PluginState::Loaded
			|| m_pluginState == PluginState::LoadedWithError
			|| m_pluginState == PluginState::None); // TODO
		break;
	case PluginState::Loaded:
		assert(m_pluginState == PluginState::None);
		break;
	case PluginState::LoadedWithError:
		assert(m_pluginState == PluginState::Loaded);
		break;
	case PluginState::Inactive:
		assert(m_pluginState == PluginState::Loaded
			|| m_pluginState == PluginState::ActiveAndReadyToDeactivate);
		break;
	case PluginState::InactiveWithError:
		assert(m_pluginState == PluginState::Inactive);
		break;
	case PluginState::ActiveAndSleeping:
		assert(m_pluginState == PluginState::Inactive
			|| m_pluginState == PluginState::ActiveAndProcessing);
		break;
	case PluginState::ActiveAndProcessing:
		assert(m_pluginState == PluginState::ActiveAndSleeping);
		break;
	case PluginState::ActiveWithError:
		assert(m_pluginState == PluginState::ActiveAndProcessing);
		break;
	case PluginState::ActiveAndReadyToDeactivate:
		assert(m_pluginState == PluginState::ActiveAndProcessing
			|| m_pluginState == PluginState::ActiveAndSleeping
			|| m_pluginState == PluginState::ActiveWithError);
		break;
	default:
		throw std::runtime_error{"CLAP plugin state error"};
	}
}

void ClapInstance::setPluginState(PluginState state)
{
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

auto ClapInstance::isMainThread() -> bool
{
	return QThread::currentThread() == QCoreApplication::instance()->thread();
}

auto ClapInstance::isAudioThread() -> bool
{
	// Assume any non-GUI thread is an audio thread
	return QThread::currentThread() != QCoreApplication::instance()->thread();
}


////////////////////////////////
// ClapInstance host
////////////////////////////////

void ClapInstance::hostDestroy()
{
	// Clear queue just in case
	while (!m_idleQueue.empty())
	{
		m_idleQueue.pop();
	}
}

void ClapInstance::hostIdle()
{
	assert(isMainThread());

	// Try to send events to the audio engine
	m_appToEngineValueQueue.producerDone();
	m_appToEngineModQueue.producerDone();

	/*
	m_engineToAppValueQueue.consume(
		[this](clap_id param_id, const EngineToAppParamQueueValue &value) {
			auto it = m_params.find(param_id);
			if (it == m_params.end())
			{
				std::ostringstream msg;
				msg << "Plugin produced a CLAP_EVENT_PARAM_SET with an unknown param_id: " << param_id;
				throw std::invalid_argument(msg.str());
			}

			if (value.has_value)
				it->second->setValue(value.value);

			if (value.has_gesture)
				it->second->setIsAdjusting(value.is_begin);

			emit paramAdjusted(param_id);
		}
	);
	*/

	if (m_scheduleParamFlush && !isPluginActive())
	{
		paramFlushOnMainThread();
	}

	if (m_scheduleMainThreadCallback)
	{
		m_scheduleMainThreadCallback = false;
		m_plugin->on_main_thread(m_plugin);
	}

	if (m_scheduleRestart)
	{
		pluginDeactivate();
		m_scheduleRestart = false;
		pluginActivate();
	}
}

void ClapInstance::setHost()
{
	m_host.host_data = this;
	m_host.clap_version = CLAP_VERSION;
	m_host.name = "LMMS";
	m_host.version = LMMS_VERSION;
	m_host.vendor = nullptr;
	m_host.url = "https://lmms.io/";
	m_host.get_extension = hostGetExtension;
	m_host.request_callback = hostRequestCallback;
	m_host.request_process = hostRequestProcess;
	m_host.request_restart = hostRequestRestart;
}

void ClapInstance::hostPushToIdleQueue(std::function<bool()>&& functor)
{
	m_idleQueue.push(std::move(functor));
}

auto ClapInstance::fromHost(const clap_host* host) -> ClapInstance*
{
	if (!host)
		throw std::invalid_argument("Passed a null host pointer");

	auto h = static_cast<ClapInstance*>(host->host_data);
	if (!h)
		throw std::invalid_argument("Passed an invalid host pointer because the host_data is null");

	if (!h->getPlugin())
		throw std::logic_error("The plugin can't query for extensions during the create method. Wait "
								"for clap_plugin.init() call.");

	return h;
}

auto ClapInstance::hostGetExtension(const clap_host* host, const char* extension_id) -> const void*
{
	[[maybe_unused]] auto h = fromHost(host);
	// TODO

	if (ClapManager::kDebug)
		qDebug() << "--Plugin requested host extension:" << extension_id;

	const std::string_view extensionId = extension_id;
	if (extensionId == CLAP_EXT_LOG)
		return &m_hostExtLog;
	if (extensionId == CLAP_EXT_THREAD_CHECK)
		return &m_hostExtThreadCheck;

	return nullptr;
}

void ClapInstance::hostRequestCallback(const clap_host* host)
{
	const auto h = fromHost(host);

	qDebug() << "ClapInstance::hostRequestCallback";

	auto mainThreadCallback = [h]() -> bool {
		const auto plugin = h->getPlugin();
		plugin->on_main_thread(plugin);
		return false;
	};

	h->hostPushToIdleQueue(std::move(mainThreadCallback));
}

void ClapInstance::hostRequestProcess(const clap_host* host)
{
	[[maybe_unused]] auto h = fromHost(host);
	// TODO
	//h->_scheduleProcess = true;
	qDebug() << "ClapInstance::hostRequestProcess";
}

void ClapInstance::hostRequestRestart(const clap_host* host)
{
	[[maybe_unused]] auto h = fromHost(host);
	// TODO
	//h->_scheduleRestart = true;
	qDebug() << "ClapInstance::hostRequestRestart";
}

void ClapInstance::hostExtStateMarkDirty(const clap_host* host)
{
	//checkForMainThread();

	auto h = fromHost(host);

	if (!h->m_pluginExtState || !h->m_pluginExtState->save || !h->m_pluginExtState->load)
		throw std::logic_error("Plugin called clap_host_state.set_dirty() but the host does not "
			"provide a complete clap_plugin_state interface.");

	h->m_hostExtStateIsDirty = true;
}

void ClapInstance::hostExtLogLog(const clap_host_t* host, clap_log_severity severity, const char* msg)
{
	// Thread-safe
	if (!ClapManager::kDebug)
		return;

	std::string severityStr;
	switch (severity)
	{
	case CLAP_LOG_DEBUG:
		severityStr += "DEBUG"; break;
	case CLAP_LOG_INFO:
		severityStr += "INFO"; break;
	case CLAP_LOG_WARNING:
		severityStr += "WARNING"; break;
	case CLAP_LOG_ERROR:
		severityStr += "ERROR"; break;
	case CLAP_LOG_FATAL:
		severityStr += "FATAL"; break;
	case CLAP_LOG_HOST_MISBEHAVING:
		severityStr += "HOST_MISBEHAVING"; break;
	case CLAP_LOG_PLUGIN_MISBEHAVING:
		severityStr += "PLUGIN_MISBEHAVING"; break;
	default:
		severityStr += "UNKNOWN"; break;
	}

	qDebug().nospace() << "CLAP LOG: severity=" << severityStr.c_str() << "; msg='" << msg << "'";
}

auto ClapInstance::hostExtThreadCheckIsMainThread(const clap_host_t* host) -> bool
{
	return isMainThread();
}

auto ClapInstance::hostExtThreadCheckIsAudioThread(const clap_host_t* host) -> bool
{
	return isAudioThread();
}

/*
bool ClapInstance::canUsePluginParams() const noexcept
{
	return m_pluginParams && m_pluginParams->count && m_pluginParams->flush
		&& m_pluginParams->get_info && m_pluginParams->get_value && m_pluginParams->text_to_value
		&& m_pluginParams->value_to_text;
}

bool ClapInstance::canUsePluginGui() const noexcept
{
	return m_pluginGui && m_pluginGui->create && m_pluginGui->destroy && m_pluginGui->can_resize
	&& m_pluginGui->get_size && m_pluginGui->adjust_size && m_pluginGui->set_size
	&& m_pluginGui->set_scale && m_pluginGui->hide && m_pluginGui->show
	&& m_pluginGui->suggest_title && m_pluginGui->is_api_supported;
}
*/


} // namespace lmms

#endif // LMMS_HAVE_CLAP
