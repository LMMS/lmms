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
#include <string_view>
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
			if constexpr (stereo) { pluginBuf[1][f] = hostBuf[f][channel + 1]; }
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
	: LinkedModelGroup{parent}, m_pluginInfo{pluginInfo}
{
	m_pluginState = PluginState::None;
	setHost();

	m_process.steady_time = -1; // Not supported yet
	m_process.transport = ClapManager::transport();

	pluginStart();
}

ClapInstance::ClapInstance(ClapInstance&& other) noexcept
	: LinkedModelGroup(other.parentModel()),
	m_pluginInfo(std::move(other.m_pluginInfo)),
	m_pluginIssues(std::move(other.m_pluginIssues))
{
	assert(false && "Shouldn't see this");
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
	/*
	for (auto param : m_params)
	{
		if (!param || !param->model()) { continue; }

		// ERROR: Cannot do this on the audio thread - doing in main thread instead
		setParamValueByHost(*param, param->model()->value<float>());
	}
	*/
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
			//qDebug() << "***stereo lmms to stereo clap";
			copyBuffersHostToPlugin<true>(buf, m_audioInActive->data32, firstChan, frames);
		}
		else
		{
			// Stereo LMMS to Mono CLAP
			//qDebug() << "***stereo lmms to mono clap";
			copyBuffersStereoHostToMonoPlugin(buf, m_audioInActive->data32, firstChan, frames);
		}
	}
	else
	{
		// Mono LMMS to Mono CLAP
		//qDebug() << "***mono lmms to mono clap";
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
			//qDebug() << "***stereo clap to stereo lmms";
			copyBuffersPluginToHost<true>(m_audioOutActive->data32, buf, m_audioOutActive->constant_mask, firstChan, frames);
		}
		else
		{
			// Mono CLAP to Stereo LMMS
			//qDebug() << "***mono clap to stereo lmms";
			copyBuffersMonoPluginToStereoHost(m_audioOutActive->data32, buf, m_audioOutActive->constant_mask, firstChan, frames);
		}
	}
	else
	{
		// Mono CLAP to Mono LMMS
		//qDebug() << "***mono clap to mono lmms";
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
	//hostIdle(); // TODO: ??? May throw an exception, which should not happen in ClapInstance dtor

	pluginUnload();

	hostDestroy();
}

auto ClapInstance::isValid() const -> bool
{
	return m_plugin != nullptr && !isPluginErrorState() && m_pluginIssues.empty();
}

auto ClapInstance::pluginStart() -> bool
{
	if (!pluginLoad()) { return false; }
	if (!pluginInit()) { return false; }
	return pluginActivate();
}

auto ClapInstance::pluginRestart() -> bool
{
	if (!pluginUnload()) { return false; }
	return pluginStart();
}

auto ClapInstance::pluginLoad() -> bool
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
		hostDestroy();
		// TODO: Set state to NoneWithError?
		return false;
	}

	setPluginState(PluginState::Loaded);
	return true;
}

auto ClapInstance::pluginUnload() -> bool
{
	qDebug() << "Unloading plugin instance:" << m_pluginInfo->descriptor()->name;
	assert(isMainThread());

	pluginDeactivate();

	if (m_plugin)
	{
		m_plugin->destroy(m_plugin);
		m_plugin = nullptr;
	}

	m_paramMap.clear();
	m_params.clear();

	// Clear all plugin extensions
	m_pluginExtAudioPorts = nullptr;
	m_pluginExtState = nullptr;
	m_pluginExtParams = nullptr;

	setPluginState(PluginState::None);
	return true;
}

auto ClapInstance::pluginInit() -> bool
{
	qDebug() << "ClapInstance::pluginInit()";
	assert(isMainThread());

	if (isPluginErrorState()) { return false; }
	checkPluginStateCurrent(PluginState::Loaded);

	if (pluginState() != PluginState::Loaded) { return false; }

	if (!m_plugin->init(m_plugin))
	{
		qWarning() << "Could not init the plugin with id:" << info().descriptor()->id;
		setPluginState(PluginState::LoadedWithError);
		m_plugin->destroy(m_plugin);
		m_plugin = nullptr;
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

	// Initialize Audio Ports extension

	if (!pluginExtensionInit(m_pluginExtAudioPorts, CLAP_EXT_AUDIO_PORTS))
	{
		qWarning() << "The required CLAP audio port extension is not supported by the plugin";
		setPluginState(PluginState::LoadedWithError);
		return false;
	}

	// Effect, Instrument, and Tool are the only options
	const bool needInputPort = m_pluginInfo->type() != Plugin::Type::Instrument;
	constexpr bool needOutputPort = true;

	auto readPorts = [this, needInputPort, needOutputPort](
		std::vector<AudioPort>& audioPorts,
		std::unique_ptr<clap_audio_buffer[]>& audioBuffers,
		std::vector<AudioBuffer>& rawAudioBuffers,
		bool isInput) -> AudioPort*
	{
		const auto portCount = m_pluginExtAudioPorts->count(m_plugin, isInput);

		if (isInput)
		{
			//if (portCount == 0 && m_pluginInfo->getType() == Plugin::PluginTypes::Effect)
			//	m_pluginIssues.emplace_back( ... );

			qDebug() << "Input ports:" << portCount;
		}
		else
		{
			if (portCount == 0 && needOutputPort)
			{
				m_pluginIssues.emplace_back(PluginIssueType::NoOutputChannel);
			}
			//if (portCount > 2)
			//	m_pluginIssues.emplace_back(PluginIssueType::tooManyOutputChannels, std::to_string(outCount));

			qDebug() << "Output ports:" << portCount;
		}

		clap_id monoPort = CLAP_INVALID_ID;
		clap_id stereoPort = CLAP_INVALID_ID;
		//clap_id mainPort = CLAP_INVALID_ID;
		for (std::uint32_t idx = 0; idx < portCount; ++idx)
		{
			auto info = clap_audio_port_info{};
			info.id = CLAP_INVALID_ID;
			info.in_place_pair = CLAP_INVALID_ID;
			if (!m_pluginExtAudioPorts->get(m_plugin, idx, isInput, &info))
			{
				qWarning() << "Unknown error calling m_pluginExtAudioPorts->get(...)";
				m_pluginIssues.emplace_back(PluginIssueType::PortHasNoDef);
				return nullptr;
			}

			qDebug() << "- port id:" << info.id;
			qDebug() << "- port name:" << info.name;
			qDebug() << "- port flags:" << info.flags;

			if (idx == 0 && !(info.flags & CLAP_AUDIO_PORT_IS_MAIN))
			{
				qDebug() << "CLAP plugin audio port #0 is not main";
			}

			//if (info.flags & CLAP_AUDIO_PORT_IS_MAIN)
			//	mainPort = idx;

			qDebug() << "- port channel_count:" << info.channel_count;

			auto type = AudioPortType::Unsupported;
			if (info.port_type)
			{
				auto portType = std::string_view{info.port_type};
				qDebug() << "- port type:" << portType.data();
				if (portType == CLAP_PORT_MONO)
				{
					assert(info.channel_count == 1);
					type = AudioPortType::Mono;
					if (monoPort == CLAP_INVALID_ID) { monoPort = idx; }
				}

				if (portType == CLAP_PORT_STEREO)
				{
					assert(info.channel_count == 2);
					type = AudioPortType::Stereo;
					if (stereoPort == CLAP_INVALID_ID) { stereoPort = idx; }
				}
			}
			else
			{
				qDebug() << "- port type: (nullptr)";
				if (info.channel_count == 1)
				{
					type = AudioPortType::Mono;
					if (monoPort == CLAP_INVALID_ID) { monoPort = idx; }
				}
				else if (info.channel_count == 2)
				{
					type = AudioPortType::Stereo;
					if (stereoPort == CLAP_INVALID_ID) { stereoPort = idx; }
				}
			}

			qDebug() << "- port in place pair:" << info.in_place_pair;

			audioPorts.emplace_back(AudioPort{info, idx, isInput, type, false});
		}

		if (isInput && !needInputPort) { return nullptr; }
		if (!isInput && !needOutputPort) { return nullptr; }

		assert(portCount == audioPorts.size());
		audioBuffers = std::make_unique<clap_audio_buffer[]>(audioPorts.size());
		for (std::uint32_t port = 0; port < audioPorts.size(); ++port)
		{
			const auto channelCount = audioPorts[port].info.channel_count;
			if (channelCount <= 0) { return nullptr; }

			audioBuffers[port].channel_count = channelCount;

			if (isInput && ((port != stereoPort && stereoPort != CLAP_INVALID_ID) || (port != monoPort && stereoPort == CLAP_INVALID_ID)))
			{
				// This input port will not be used by LMMS
				// TODO: Will a mono port ever need to be used if a stereo port is available?
				constexpr std::uint32_t maxChannels = sizeof(clap_audio_buffer::constant_mask) * 8;
				audioBuffers[port].constant_mask = channelCount < maxChannels ? (1u << channelCount) - 1 : static_cast<std::uint64_t>(-1);
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
			if (isInput) { m_monoInput = false; } else { m_monoOutput = false; }
			auto port = &audioPorts[stereoPort];
			port->used = true;
			return port;
		}

		if (monoPort != CLAP_INVALID_ID)
		{
			if (isInput) { m_monoInput = true; } else { m_monoOutput = true; }
			auto port = &audioPorts[monoPort];
			port->used = true;
			return port;
		}

		// Missing a required port type that LMMS supports - i.e. an effect where the only input is surround sound
		qWarning() << "An" << (isInput ? "input" : "output") << "port is required, but CLAP plugin has none that are usable";
		m_pluginIssues.emplace_back(PluginIssueType::UnknownPortType);
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

	if (isMonoInput() && m_audioPortInActive->type != AudioPortType::Mono)
	{
		setPluginState(PluginState::LoadedWithError);
		return false;
	}
	if (!isMonoInput() && m_audioPortInActive->type != AudioPortType::Stereo)
	{
		setPluginState(PluginState::LoadedWithError);
		return false;
	}
	if (isMonoOutput() && m_audioPortOutActive->type != AudioPortType::Mono)
	{
		setPluginState(PluginState::LoadedWithError);
		return false;
	}
	if (!isMonoOutput() && m_audioPortOutActive->type != AudioPortType::Stereo)
	{
		setPluginState(PluginState::LoadedWithError);
		return false;
	}

	// Now initialize params and add param models

	if (pluginExtensionInit(m_pluginExtParams, CLAP_EXT_PARAMS))
	{
		try
		{
			hostExtParamsRescan(host(), CLAP_PARAM_RESCAN_ALL);
		}
		catch (const std::exception& e)
		{
			qWarning() << e.what() << '\n';
			return false;
		}

		qDebug() << "CLAP PARAMS: m_params.size():" << m_params.size();
		for (auto param : m_params)
		{
			if (param && param->model())
			{
				const auto uri = QString::fromUtf8(param->id().data());
				addModel(param->model(), uri);

				// Tell plugin when param value changes in host
				auto updateParam = [this, param]() {
					setParamValueByHost(*param, param->model()->value<float>());
				};

				// This is used for updating input parameters instead of copyModelsFromCore()
				connect(param->model(), &Model::dataChanged, this, updateParam);

				// Initially assign model value to param value
				updateParam();
			}
		}
	}
	else
	{
		qWarning() << "The params extension is not supported by the CLAP plugin";
	}

	//scanQuickControls();

	setPluginState(PluginState::Inactive);

	return true;
}

auto ClapInstance::pluginActivate() -> bool
{
	qDebug() << "ClapInstance::pluginActivate()";
	assert(isMainThread());

	if (isPluginErrorState()) { return false; }
	checkPluginStateCurrent(PluginState::Inactive);

	const auto sampleRate = static_cast<double>(Engine::audioEngine()->processingSampleRate());
	static_assert(DEFAULT_BUFFER_SIZE > MINIMUM_BUFFER_SIZE);

	assert(!isPluginActive());
	if (!m_plugin->activate(m_plugin, sampleRate, MINIMUM_BUFFER_SIZE, DEFAULT_BUFFER_SIZE))
	{
		setPluginState(PluginState::InactiveWithError);
		return false;
	}

	m_scheduleProcess = true; // TODO: ?????
	setPluginState(PluginState::ActiveAndSleeping);
	return true;
}

auto ClapInstance::pluginDeactivate() -> bool
{
	qDebug() << "ClapInstance::pluginDeactivate";
	assert(isMainThread());
	if (!isPluginActive()) { return false; }

	/*
	//TODO: Need to fix this
	while (isPluginProcessing() || isPluginSleeping())
	{
		m_scheduleDeactivate = true;
		QThread::msleep(10);
	}
	m_scheduleDeactivate = false;
	*/

	m_plugin->deactivate(m_plugin);
	setPluginState(PluginState::Inactive);
	qDebug() << "ClapInstance::pluginDeactivate end";
	return true;
}

auto ClapInstance::pluginProcessBegin(std::uint32_t frames) -> bool
{
	m_process.frames_count = frames;
	//m_process.steady_time = m_steadyTime;
	return false;
}

auto ClapInstance::pluginProcess(std::uint32_t frames) -> bool
{
	//m_steadyTime += frames;

	assert(isAudioThread());
	if (!m_plugin) { return false; }

	// Can't process a plugin that is not active
	if (!isPluginActive()) { return false; }

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
	generatePluginInputEvents();

	if (isPluginSleeping())
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
	if (isPluginProcessing())
	{
		status = m_plugin->process(m_plugin, &m_process);
	}

	handlePluginOutputEvents();

	m_evOut.clear();
	m_evIn.clear();

	m_engineToAppValueQueue.producerDone();

	// TODO: send plugin to sleep if possible

	return true;
}

auto ClapInstance::pluginProcessEnd(std::uint32_t frames) -> bool
{
	m_process.frames_count = frames;
	//m_process.steady_time = m_steadyTime;
	return false;
}

void ClapInstance::generatePluginInputEvents()
{
	m_appToEngineValueQueue.consume(
		[this](clap_id param_id, const HostToPluginParamQueueValue& value) {
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

	m_appToEngineModQueue.consume([this](clap_id param_id, const HostToPluginParamQueueValue& value) {
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
	for (std::uint32_t i = 0; i < m_evOut.size(); ++i)
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

				PluginToHostParamQueueValue v;
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
				PluginToHostParamQueueValue v;
				v.has_gesture = true;
				v.is_begin = false;
				m_engineToAppValueQueue.setOrUpdate(ev->param_id, v);
				break;
			}

			case CLAP_EVENT_PARAM_VALUE:
			{
				auto ev = reinterpret_cast<const clap_event_param_value*>(h);
				PluginToHostParamQueueValue v;
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

	if (canUsePluginParams())
	{
		m_pluginExtParams->flush(m_plugin, m_evIn.clapInputEvents(), m_evOut.clapOutputEvents());
	}

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
		return true; // TODO: Remove once ClapInstance::pluginDeactivate() is fixed
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
	if (isPluginErrorState()) { return; }

	// Try to send events to the audio engine
	m_appToEngineValueQueue.producerDone();
	m_appToEngineModQueue.producerDone();

	m_engineToAppValueQueue.consume(
		[this](clap_id paramId, const PluginToHostParamQueueValue& value) {
			const auto it = m_paramMap.find(paramId);
			if (it == m_paramMap.end())
			{
				std::string msg = "Plugin produced a CLAP_EVENT_PARAM_SET with an unknown param_id: " + std::to_string(paramId);
				throw std::invalid_argument{msg};
			}

			if (value.has_value) { it->second->setValue(value.value); }
			if (value.has_gesture) { it->second->setIsAdjusting(value.is_begin); }

			emit paramAdjusted(paramId);
		}
	);

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
	m_host.vendor = "";
	m_host.url = "https://lmms.io/";
	m_host.get_extension = &hostGetExtension;
	m_host.request_callback = &hostRequestCallback;
	m_host.request_process = &hostRequestProcess;
	m_host.request_restart = &hostRequestRestart;
}

void ClapInstance::hostPushToIdleQueue(std::function<bool()>&& functor)
{
	m_idleQueue.push(std::move(functor));
}

auto ClapInstance::fromHost(const clap_host* host) -> ClapInstance*
{
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

auto ClapInstance::hostGetExtension(const clap_host* host, const char* extensionId) -> const void*
{
	[[maybe_unused]] auto h = fromHost(host);
	if (!extensionId) { return nullptr; }

	if (ClapManager::debugging()) { qDebug() << "--Plugin requested host extension:" << extensionId; }

	const auto extensionIdView = std::string_view{extensionId};
	if (extensionIdView == CLAP_EXT_LOG) { return &m_hostExtLog; }
	if (extensionIdView == CLAP_EXT_THREAD_CHECK) { return &m_hostExtThreadCheck; }
	if (extensionIdView == CLAP_EXT_PARAMS) { return &m_hostExtParams; }
	if (extensionIdView == CLAP_EXT_LATENCY) { return &m_hostExtLatency; }

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

void ClapInstance::hostExtStateMarkDirty(const clap_host* host)
{
	assert(isMainThread());
	auto h = fromHost(host);

	if (!h->m_pluginExtState || !h->m_pluginExtState->save || !h->m_pluginExtState->load)
	{
		h->hostExtLogLog(host, CLAP_LOG_ERROR, "Plugin called clap_host_state.set_dirty() but the plugin does not "
			"provide a complete clap_plugin_state interface.");
		return;
	}

	h->m_hostExtStateIsDirty = true;
}

void ClapInstance::hostExtLogLog(const clap_host* host, clap_log_severity severity, const char* msg)
{
	// Thread-safe
	std::string_view severityStr;
	switch (severity)
	{
	case CLAP_LOG_DEBUG:
		if (!ClapManager::debugging()) { return; }
		severityStr = "DEBUG"; break;
	case CLAP_LOG_INFO:
		if (!ClapManager::debugging()) { return; }
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

	qDebug().nospace() << "CLAP LOG: severity=" << severityStr.data() << "; msg='" << msg << "'";
}

auto ClapInstance::hostExtThreadCheckIsMainThread(const clap_host* host) -> bool
{
	return isMainThread();
}

auto ClapInstance::hostExtThreadCheckIsAudioThread(const clap_host* host) -> bool
{
	return isAudioThread();
}

void ClapInstance::hostExtParamsRescan(const clap_host* host, std::uint32_t flags)
{
	qDebug() << "ClapInstance::hostExtParamsRescan";
	assert(isMainThread());
	auto h = fromHost(host);

	if (!h->canUsePluginParams()) { return; }

	// 1. It is forbidden to use CLAP_PARAM_RESCAN_ALL if the plugin is active
	if (h->isPluginActive() && (flags & CLAP_PARAM_RESCAN_ALL))
	{
		throw std::logic_error{"clap_host_params.recan(CLAP_PARAM_RESCAN_ALL) was called while the plugin is active!"};
	}

	// 2. Scan the params
	auto count = h->m_pluginExtParams->count(h->m_plugin);
	qDebug() << "CLAP PARAMS: count:" << count;
	std::unordered_set<clap_id> paramIds(count * 2);
	bool needToUpdateParamsCache = false;

	for (int32_t i = 0; i < count; ++i)
	{
		clap_param_info info{};
		info.id = CLAP_INVALID_ID;

		if (!h->m_pluginExtParams->get_info(h->m_plugin, i, &info))
		{
			throw std::logic_error{"clap_plugin_params.get_info() returned false!"};
		}

		ClapParam::check(info);

		if (info.id == CLAP_INVALID_ID)
		{
			std::string msg = "clap_plugin_params.get_info() reported a parameter with id = CLAP_INVALID_ID\n"
				" 2. name: " + std::string{info.name} + ", module: " + std::string{info.module};
			throw std::logic_error{msg};
		}

		auto it = h->m_paramMap.find(info.id);

		// Check that the parameter is not declared twice
		if (paramIds.count(info.id) > 0)
		{
			assert(it != h->m_paramMap.end());
			std::string msg = "the parameter with id: " + std::to_string(info.id) + " was declared twice.\n"
				" 1. name: " + std::string{it->second->info().name} + ", module: " + std::string{it->second->info().module} + "\n"
				" 2. name: " + std::string{info.name} + ", module: " + std::string{info.module};
			throw std::logic_error{msg};
		}
		paramIds.insert(info.id);

		if (it == h->m_paramMap.end())
		{
			if (!(flags & CLAP_PARAM_RESCAN_ALL))
			{
				std::string msg = "a new parameter was declared, but the flag CLAP_PARAM_RESCAN_ALL was not specified; "
					"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
				throw std::logic_error{msg};
			}

			double value = h->getParamValue(info);
			auto param = std::make_unique<ClapParam>(h, info, value);
			h->checkValidParamValue(*param, value);
			h->m_paramMap.insert_or_assign(info.id, std::move(param));
			needToUpdateParamsCache = true;
		}
		else
		{
			// Update param info
			if (!it->second->isInfoEqualTo(info))
			{
				if (!clapParamsRescanMayInfoChange(flags))
				{
					std::string msg = "a parameter's info did change, but the flag CLAP_PARAM_RESCAN_INFO was not specified; "
						"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
					throw std::logic_error{msg};
				}

				if (!(flags & CLAP_PARAM_RESCAN_ALL) && !it->second->isInfoCriticallyDifferentTo(info))
				{
					std::string msg = "a parameter's info has critical changes, but the flag CLAP_PARAM_RESCAN_ALL was not specified; "
						"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
					throw std::logic_error{msg};
				}

				it->second->setInfo(info);
			}

			double value = h->getParamValue(info);
			if (it->second->value() != value)
			{
				if (!clapParamsRescanMayValueChange(flags))
				{
					std::string msg = "a parameter's value did change but, but the flag CLAP_PARAM_RESCAN_VALUES was not specified; "
						"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
					throw std::logic_error{msg};
				}

				// Update param value
				h->checkValidParamValue(*it->second, value);
				it->second->setValue(value);
				it->second->setModulation(value);
			}
		}
	}

	// 3. Remove parameters which are gone
	for (auto it = h->m_paramMap.begin(); it != h->m_paramMap.end();)
	{
		if (paramIds.find(it->first) == paramIds.end())
		{
			if (!(flags & CLAP_PARAM_RESCAN_ALL))
			{
				const auto& info = it->second->info();
				std::string msg = "a parameter was removed, but the flag CLAP_PARAM_RESCAN_ALL was not specified; "
					"id: " + std::to_string(info.id) + ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
				throw std::logic_error{msg};
			}
			it = h->m_paramMap.erase(it);
			needToUpdateParamsCache = true;
		}
		else { ++it; }
	}

	if (needToUpdateParamsCache)
	{
		h->m_params.resize(h->m_paramMap.size());
		int i = 0;
		for (const auto& elem : h->m_paramMap)
		{
			h->m_params[i] = elem.second.get();
			++i;
		}
	}

	if (flags & CLAP_PARAM_RESCAN_ALL) { h->paramsChanged(); }
}

void ClapInstance::hostExtParamsClear(const clap_host* host, clap_id param_id, clap_param_clear_flags flags)
{
	assert(isMainThread());
	qDebug() << "ClapInstance::hostExtParamsClear";
	// TODO
}

void ClapInstance::hostExtParamsRequestFlush(const clap_host* host)
{
	qDebug() << "ClapInstance::hostExtParamsRequestFlush";
	auto h = fromHost(host);

	if (!h->isPluginActive() && hostExtThreadCheckIsMainThread(host))
	{
		// Perform the flush immediately
		h->paramFlushOnMainThread();
		return;
	}

	h->m_scheduleParamFlush = true;
}

void ClapInstance::hostExtLatencyChanged(const clap_host* host)
{
	/*
	 * LMMS currently does not use latency data, but implementing this extension
	 * fixes a crash that would occur in plugins built using the DISTRHO plugin
	 * framework prior to this commit:
	 * https://github.com/DISTRHO/DPF/commit/4f11f8cc49b24ede1735a16606e7bad5a52ab41d
	 */
	//qDebug() << "ClapInstance::hostExtLatencyChanged";
}

auto ClapInstance::canUsePluginParams() const noexcept -> bool
{
	return m_pluginExtParams && m_pluginExtParams->count && m_pluginExtParams->flush
		&& m_pluginExtParams->get_info && m_pluginExtParams->get_value && m_pluginExtParams->text_to_value
		&& m_pluginExtParams->value_to_text;
}

/*
bool ClapInstance::canUsePluginGui() const noexcept
{
	return m_pluginGui && m_pluginGui->create && m_pluginGui->destroy && m_pluginGui->can_resize
	&& m_pluginGui->get_size && m_pluginGui->adjust_size && m_pluginGui->set_size
	&& m_pluginGui->set_scale && m_pluginGui->hide && m_pluginGui->show
	&& m_pluginGui->suggest_title && m_pluginGui->is_api_supported;
}
*/

void ClapInstance::setParamValueByHost(ClapParam& param, double value)
{
	assert(isMainThread());

	param.setValue(value);

	m_appToEngineValueQueue.set(param.info().id, {param.info().cookie, value});
	m_appToEngineValueQueue.producerDone();
	hostExtParamsRequestFlush(host());
}

void ClapInstance::setParamModulationByHost(ClapParam& param, double value)
{
	assert(isMainThread());

	param.setModulation(value);

	m_appToEngineModQueue.set(param.info().id, {param.info().cookie, value});
	m_appToEngineModQueue.producerDone();
	hostExtParamsRequestFlush(host());
}

void ClapInstance::checkValidParamValue(const ClapParam& param, double value)
{
	assert(isMainThread());
	if (!param.isValueValid(value))
	{
		std::ostringstream msg;
		msg << "Invalid value for param. ";
		param.printInfo(msg);
		msg << "; value: " << value;
		// std::cerr << msg.str() << std::endl;
		throw std::invalid_argument{msg.str()};
	}
}

auto ClapInstance::getParamValue(const clap_param_info& info) const -> double
{
	assert(isMainThread());

	if (!canUsePluginParams()) { return 0.0; }

	double value = 0.0;
	if (m_pluginExtParams->get_value(m_plugin, info.id, &value)) { return value; }

	std::string msg = "failed to get the param value, id: " + std::to_string(info.id)
		+ ", name: " + std::string{info.name} + ", module: " + std::string{info.module};
	throw std::logic_error{msg};
}

auto ClapInstance::getParamValueText(const ClapParam* param) const -> std::string
{
	assert(param != nullptr);
	return param->getValueText(m_plugin, m_pluginExtParams);
}


} // namespace lmms

#endif // LMMS_HAVE_CLAP
