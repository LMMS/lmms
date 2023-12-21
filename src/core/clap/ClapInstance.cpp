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
} // namespace

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
	: LinkedModelGroup{parent}
	, m_pluginInfo{pluginInfo}
	, m_midiInputBuf{s_maxMidiInputEvents}
	, m_midiInputReader{m_midiInputBuf}
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

void ClapInstance::copyBuffersFromCore(const sampleFrame* buf, unsigned firstChannel, unsigned numChannels, fpp_t frames)
{
	// LMMS to CLAP
	if (numChannels > 1)
	{
		if (hasStereoInput())
		{
			// Stereo LMMS to Stereo CLAP
			//qDebug() << "***stereo lmms to stereo clap";
			copyBuffersHostToPlugin<true>(buf, m_audioInActive->data32, firstChannel, frames);
		}
		else
		{
			// Stereo LMMS to Mono CLAP
			//qDebug() << "***stereo lmms to mono clap";
			copyBuffersStereoHostToMonoPlugin(buf, m_audioInActive->data32, firstChannel, frames);
		}
	}
	else
	{
		// Mono LMMS to Mono CLAP
		//qDebug() << "***mono lmms to mono clap";
		copyBuffersHostToPlugin<false>(buf, m_audioInActive->data32, firstChannel, frames);
	}
}

void ClapInstance::copyBuffersToCore(sampleFrame* buf, unsigned firstChannel, unsigned numChannels, fpp_t frames) const
{
	// CLAP to LMMS
	if (numChannels > 1)
	{
		if (hasStereoOutput())
		{
			// Stereo CLAP to Stereo LMMS
			//qDebug() << "***stereo clap to stereo lmms";
			copyBuffersPluginToHost<true>(m_audioOutActive->data32, buf, m_audioOutActive->constant_mask, firstChannel, frames);
		}
		else
		{
			// Mono CLAP to Stereo LMMS
			//qDebug() << "***mono clap to stereo lmms";
			copyBuffersMonoPluginToStereoHost(m_audioOutActive->data32, buf, m_audioOutActive->constant_mask, firstChannel, frames);
		}
	}
	else
	{
		// Mono CLAP to Mono LMMS
		//qDebug() << "***mono clap to mono lmms";
		copyBuffersPluginToHost<false>(m_audioOutActive->data32, buf, m_audioOutActive->constant_mask, firstChannel, frames);
	}
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

auto ClapInstance::hasNoteInput() const -> bool
{
	return m_noteDialect != 0;
}

void ClapInstance::destroy()
{
	//hostIdle(); // TODO: ??? May throw an exception, which should not happen in ClapInstance dtor

	unload();

	hostDestroy();
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
		hostDestroy();
		// TODO: Set state to NoneWithError?
		return false;
	}

	setPluginState(PluginState::Loaded);
	return true;
}

auto ClapInstance::unload() -> bool
{
	qDebug() << "Unloading plugin instance:" << m_pluginInfo->descriptor()->name;
	assert(isMainThread());

	m_pluginGui.reset();
	m_pluginTimerSupport.deinit();

	deactivate();

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
	m_pluginExtGui = nullptr;
	m_pluginExtNotePorts = nullptr;

	setPluginState(PluginState::None);
	return true;
}

auto ClapInstance::init() -> bool
{
	qDebug() << "ClapInstance::init";
	assert(isMainThread());

	if (isErrorState()) { return false; }
	checkPluginStateCurrent(PluginState::Loaded);

	if (state() != PluginState::Loaded) { return false; }

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
	LinkedModelGroup::clearModels();

	// Initialize Audio Ports extension

	// TODO: Create ClapAudioPorts class
	if (!pluginExtensionInit(m_pluginExtAudioPorts, CLAP_EXT_AUDIO_PORTS,
		+[](const clap_plugin_audio_ports* p) { return p->count && p->get; }))
	{
		log(CLAP_LOG_ERROR, "Plugin does not implement the required audio port extension");
		setPluginState(PluginState::LoadedWithError);
		return false;
	}

	// Effect, Instrument, and Tool are the only options
	const bool needInputPort = m_pluginInfo->type() != Plugin::Type::Instrument;
	constexpr bool needOutputPort = true;

	auto readPorts = [&](
		std::vector<AudioPort>& audioPorts,
		std::unique_ptr<clap_audio_buffer[]>& audioBuffers,
		std::vector<AudioBuffer>& rawAudioBuffers,
		bool isInput) -> AudioPort*
	{
		if (isInput && !needInputPort)
		{
			log(CLAP_LOG_DEBUG, "Don't need audio input ports - skipping.");
			return nullptr;
		}

		if (!isInput && !needOutputPort)
		{
			log(CLAP_LOG_DEBUG, "Don't need audio output ports - skipping.");
			return nullptr;
		}

		const auto portCount = m_pluginExtAudioPorts->count(m_plugin, isInput);

		if (isInput)
		{
			m_hasStereoInput = false; // initialize

			//if (portCount == 0 && m_pluginInfo->getType() == Plugin::PluginTypes::Effect)
			//	m_pluginIssues.emplace_back( ... );

			qDebug() << "Input ports:" << portCount;
		}
		else
		{
			m_hasStereoOutput = false; // initialize

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

		assert(portCount == audioPorts.size());
		audioBuffers = std::make_unique<clap_audio_buffer[]>(audioPorts.size());
		for (std::uint32_t port = 0; port < audioPorts.size(); ++port)
		{
			const auto channelCount = audioPorts[port].info.channel_count;
			if (channelCount <= 0) { return nullptr; }

			audioBuffers[port].channel_count = channelCount;
			if (isInput && port != monoPort && (port != stereoPort || stereoPort == CLAP_INVALID_ID))
			{
				// This input port will not be used by LMMS
				// TODO: Will a mono port ever need to be used if a stereo port is available?
				audioBuffers[port].constant_mask = static_cast<std::uint64_t>(-1);
			}
			else
			{
				audioBuffers[port].constant_mask = 0;
			}

			auto& rawBuffer = rawAudioBuffers.emplace_back(channelCount, DEFAULT_BUFFER_SIZE);
			audioBuffers[port].data32 = rawBuffer.data();
			audioBuffers[port].data64 = nullptr; // Not supported by LMMS
			audioBuffers[port].latency = 0; // TODO
		}

		if (stereoPort != CLAP_INVALID_ID)
		{
			if (isInput) { m_hasStereoInput = true; } else { m_hasStereoOutput = true; }
			auto port = &audioPorts[stereoPort];
			port->used = true;
			return port;
		}

		if (monoPort != CLAP_INVALID_ID)
		{
			auto port = &audioPorts[monoPort];
			port->used = true;
			return port;
		}

		// Missing a required port type that LMMS supports - i.e. an effect where the only input is surround sound
		qWarning() << "An" << (isInput ? "input" : "output") << "port is required, but CLAP plugin has none that are usable";
		m_pluginIssues.emplace_back(PluginIssueType::UnknownPortType); // TODO: Add better entry to PluginIssueType
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

	if (needInputPort)
	{
		if (!hasStereoInput() && m_audioPortInActive->type != AudioPortType::Mono)
		{
			setPluginState(PluginState::LoadedWithError);
			return false;
		}
		if (hasStereoInput() && m_audioPortInActive->type != AudioPortType::Stereo)
		{
			setPluginState(PluginState::LoadedWithError);
			return false;
		}
	}

	if (needOutputPort)
	{
		if (!hasStereoOutput() && m_audioPortOutActive->type != AudioPortType::Mono)
		{
			setPluginState(PluginState::LoadedWithError);
			return false;
		}
		if (hasStereoOutput() && m_audioPortOutActive->type != AudioPortType::Stereo)
		{
			setPluginState(PluginState::LoadedWithError);
			return false;
		}
	}

	// Now initialize params and add param models

	if (pluginExtensionInit(m_pluginExtParams, CLAP_EXT_PARAMS, &ClapParam::extensionSupported))
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

		// TODO: Move this code to hostExtParamsRescan()?
		qDebug() << "CLAP PARAMS: m_params.size():" << m_params.size();
		for (auto param : m_params)
		{
			if (param && param->model())
			{
				const auto uri = QString::fromUtf8(param->id().data());
				LinkedModelGroup::addModel(param->model(), uri);

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

	// Initialize GUI
	// TODO: What if this is the 2nd instance of a mono plugin?
	m_pluginGui.reset();
	if (pluginExtensionInit(m_pluginExtGui, CLAP_EXT_GUI, &ClapGui::extensionSupported))
	{
		m_pluginGui = std::make_unique<ClapGui>(m_pluginInfo, m_plugin, m_pluginExtGui);
	}

	// Set up note ports
	m_notePortIndex = 0;
	m_noteDialect = 0;
	if (pluginExtensionInit(m_pluginExtNotePorts, CLAP_EXT_NOTE_PORTS,
		+[](const clap_plugin_note_ports* p){ return p->count && p->get; }))
	{
		hostExtNotePortsRescan(host(), CLAP_NOTE_PORTS_RESCAN_ALL);
	}

	if (!hasNoteInput() && info().type() == Plugin::Type::Instrument)
	{
		log(CLAP_LOG_WARNING, "Plugin is instrument but doesn't implement note port extension");
	}

	// Timer support
	m_pluginTimerSupport.init(m_plugin);

	setPluginState(PluginState::Inactive);

	return true;
}

auto ClapInstance::activate() -> bool
{
	qDebug() << "ClapInstance::activate";
	assert(isMainThread());

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

	m_scheduleProcess = true; // TODO: ?????
	setPluginState(PluginState::ActiveAndSleeping);
	return true;
}

auto ClapInstance::deactivate() -> bool
{
	// NOTE: This method assumes that process() cannot be called concurrently
	qDebug() << "ClapInstance::deactivate";
	assert(isMainThread());
	if (!isActive()) { return false; }

	// TODO: m_pluginTimerSupport.killTimers()?

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
	assert(isAudioThread());
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
			ev.port_index = static_cast<std::int16_t>(m_notePortIndex);
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
			ev.port_index = m_notePortIndex;
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
	assert(isAudioThread());

	switch (m_noteDialect)
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
			ev.port_index = static_cast<std::int16_t>(m_notePortIndex);
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
			ev.port_index = m_notePortIndex;
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
	assert(isAudioThread());
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
	generatePluginInputEvents();

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

	handlePluginOutputEvents();

	m_evOut.clear();
	m_evIn.clear();

	m_pluginToHostValueQueue.producerDone();

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

void ClapInstance::generatePluginInputEvents()
{
	m_hostToPluginValueQueue.consume(
		[this](clap_id paramId, const HostToPluginParamQueueValue& value) {
			clap_event_param_value ev;
			ev.header.time = 0;
			ev.header.type = CLAP_EVENT_PARAM_VALUE;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.flags = 0;
			ev.header.size = sizeof(ev);
			ev.param_id = paramId;
			ev.cookie = s_hostShouldProvideParamCookie ? value.cookie : nullptr;
			ev.port_index = 0;
			ev.key = -1;
			ev.channel = -1;
			ev.note_id = -1;
			ev.value = value.value;
			m_evIn.push(&ev.header);
		});

	m_hostToPluginModQueue.consume(
		[this](clap_id paramId, const HostToPluginParamQueueValue& value) {
			clap_event_param_mod ev;
			ev.header.time = 0;
			ev.header.type = CLAP_EVENT_PARAM_MOD;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.flags = 0;
			ev.header.size = sizeof(ev);
			ev.param_id = paramId;
			ev.cookie = s_hostShouldProvideParamCookie ? value.cookie : nullptr;
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
	// TODO: Are LMMS models being updated with values here?
	for (std::uint32_t i = 0; i < m_evOut.size(); ++i)
	{
		auto h = m_evOut.get(i);
		switch (h->type)
		{
			case CLAP_EVENT_PARAM_GESTURE_BEGIN:
			{
				auto ev = reinterpret_cast<const clap_event_param_gesture*>(h);
				bool& isAdj = m_isAdjustingParameter[ev->param_id];

				if (isAdj)
				{
					throw std::logic_error("The plugin sent BEGIN_ADJUST twice");
				}
				isAdj = true;

				PluginToHostParamQueueValue v;
				v.hasGesture = true;
				v.isBegin = true;
				m_pluginToHostValueQueue.setOrUpdate(ev->param_id, v);
				break;
			}

			case CLAP_EVENT_PARAM_GESTURE_END:
			{
				auto ev = reinterpret_cast<const clap_event_param_gesture*>(h);
				bool& isAdj = m_isAdjustingParameter[ev->param_id];

				if (!isAdj)
				{
					throw std::logic_error("The plugin sent END_ADJUST without a preceding BEGIN_ADJUST");
				}
				isAdj = false;

				PluginToHostParamQueueValue v;
				v.hasGesture = true;
				v.isBegin = false;
				m_pluginToHostValueQueue.setOrUpdate(ev->param_id, v);
				break;
			}

			case CLAP_EVENT_PARAM_VALUE:
			{
				auto ev = reinterpret_cast<const clap_event_param_value*>(h);
				PluginToHostParamQueueValue v;
				v.hasValue = true;
				v.value = ev->value;
				m_pluginToHostValueQueue.setOrUpdate(ev->param_id, v);
				break;
			}
		}
	}
}

void ClapInstance::paramFlushOnMainThread()
{
	//qDebug() << "ClapInstance::paramFlushOnMainThread";
	assert(isMainThread());
	assert(!isActive());

	m_scheduleParamFlush = false;

	m_evIn.clear();
	m_evOut.clear();

	generatePluginInputEvents();

	if (m_pluginExtParams)
	{
		m_pluginExtParams->flush(m_plugin, m_evIn.clapInputEvents(), m_evOut.clapOutputEvents());
	}

	handlePluginOutputEvents();

	m_evOut.clear();
	m_pluginToHostValueQueue.producerDone();
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

auto ClapInstance::isMainThread() -> bool
{
	return QThread::currentThread() == QCoreApplication::instance()->thread();
}

auto ClapInstance::isAudioThread() -> bool
{
	// Assume any non-GUI thread is an audio thread
	return QThread::currentThread() != QCoreApplication::instance()->thread();
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
	assert(isMainThread());
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
	if (isErrorState()) { return; }

	// Try to send events to the audio engine
	m_hostToPluginValueQueue.producerDone();
	m_hostToPluginModQueue.producerDone();

	m_pluginToHostValueQueue.consume(
		[this](clap_id paramId, const PluginToHostParamQueueValue& value) {
			const auto it = m_paramMap.find(paramId);
			if (it == m_paramMap.end())
			{
				std::string msg = "Plugin produced a CLAP_EVENT_PARAM_SET with an unknown param_id: " + std::to_string(paramId);
				throw std::invalid_argument{msg};
			}

			if (value.hasValue) { it->second->setValue(value.value); }
			if (value.hasGesture) { it->second->setIsAdjusting(value.isBegin); }

			emit paramAdjusted(paramId);
		}
	);

	if (m_scheduleParamFlush && !isActive())
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

void ClapInstance::hostPushToIdleQueue(std::function<bool()>&& functor)
{
	// TODO: Remove this method and m_idleQueue?
	m_idleQueue.push(std::move(functor));
}

auto ClapInstance::hostGetExtension(const clap_host* host, const char* extensionId) -> const void*
{
	[[maybe_unused]] auto h = fromHost(host);
	if (!extensionId) { return nullptr; }

	if (ClapManager::debugging()) { qDebug() << "--Plugin requested host extension:" << extensionId; }

	const auto id = std::string_view{extensionId};
	if (id == CLAP_EXT_LOG)           { return &s_hostExtLog; }
	if (id == CLAP_EXT_THREAD_CHECK)  { return &s_hostExtThreadCheck; }
	if (id == CLAP_EXT_PARAMS)        { return &s_hostExtParams; }
	if (id == CLAP_EXT_LATENCY)       { return &s_hostExtLatency; }
	if (id == CLAP_EXT_GUI)           { return &s_hostExtGui; }
	if (id == CLAP_EXT_NOTE_PORTS)    { return &s_hostExtNotePorts; }
	if (id == CLAP_EXT_TIMER_SUPPORT) { return &s_hostExtTimerSupport; }

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
		h->log(CLAP_LOG_PLUGIN_MISBEHAVING, "Plugin called clap_host_state.set_dirty() but the plugin does not "
			"provide a complete clap_plugin_state interface.");
		return;
	}

	h->m_hostExtStateIsDirty = true;
}

void ClapInstance::hostExtLogLog(const clap_host* host, clap_log_severity severity, const char* msg)
{
	// Thread-safe
	auto h = fromHost(host);
	h->log(severity, msg);
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
	// TODO: Update LinkedModelGroup when parameters are added/removed?
	qDebug() << "ClapInstance::hostExtParamsRescan";
	assert(isMainThread());
	auto h = fromHost(host);

	if (!h->m_pluginExtParams) { return; }

	// 1. It is forbidden to use CLAP_PARAM_RESCAN_ALL if the plugin is active
	if (h->isActive() && (flags & CLAP_PARAM_RESCAN_ALL))
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

void ClapInstance::hostExtParamsClear(const clap_host* host, clap_id paramId, clap_param_clear_flags flags)
{
	assert(isMainThread());
	qDebug() << "ClapInstance::hostExtParamsClear";
	// TODO
}

void ClapInstance::hostExtParamsRequestFlush(const clap_host* host)
{
	//qDebug() << "ClapInstance::hostExtParamsRequestFlush";
	auto h = fromHost(host);

	if (!h->isActive() && hostExtThreadCheckIsMainThread(host))
	{
		// Perform the flush immediately
		h->paramFlushOnMainThread();
		return;
	}

	h->m_scheduleParamFlush = true;
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

void ClapInstance::hostExtGuiResizeHintsChanged(const clap_host* host)
{
	auto gui = fromHost(host)->gui();
	gui->clapResizeHintsChanged();
}

auto ClapInstance::hostExtGuiRequestResize(const clap_host* host, std::uint32_t width, std::uint32_t height) -> bool
{
	auto gui = fromHost(host)->gui();
	return gui->clapRequestResize(width, height);
}

auto ClapInstance::hostExtGuiRequestShow(const clap_host* host) -> bool
{
	auto gui = fromHost(host)->gui();
	return gui->clapRequestShow();
}

auto ClapInstance::hostExtGuiRequestHide(const clap_host* host) -> bool
{
	auto gui = fromHost(host)->gui();
	return gui->clapRequestHide();
}

void ClapInstance::hostExtGuiRequestClosed(const clap_host* host, bool wasDestroyed)
{
	auto gui = fromHost(host)->gui();
	gui->clapRequestClosed(wasDestroyed);
}

auto ClapInstance::hostExtNotePortsSupportedDialects([[maybe_unused]] const clap_host* host) -> std::uint32_t
{
	return CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
}

void ClapInstance::hostExtNotePortsRescan(const clap_host* host, std::uint32_t flags)
{
	qDebug() << "ClapInstance::hostExtNotePortsRescan";
	auto h = fromHost(host);

	if (!h->m_pluginExtNotePorts) { return; }

	if (flags & CLAP_NOTE_PORTS_RESCAN_ALL)
	{
		if (h->isActive())
		{
			h->log(CLAP_LOG_PLUGIN_MISBEHAVING, "Host cannot rescan note ports while plugin is active");
			return;
		}

		/*
		 * I'm using a priority system to choose the note port we use.
		 * This may not be very useful in practice.
		 *
		 * Highest to lowest priority:
		 *   - CLAP preferred + MIDI (and CLAP) supported
		 *   - MIDI preferred + CLAP (and MIDI) supported
		 *   - Other preferred + CLAP and MIDI supported
		 *   - CLAP supported
		 *   - MIDI supported
		 *   - (no note input support)
		 */
		class PriorityHelper
		{
		public:
			using IndexDialectPair = std::pair<std::uint16_t, std::uint32_t>;
			auto check(std::uint16_t index, const clap_note_port_info& info)
			{
				// MIDI preferred + CLAP (and MIDI) supported
				if (info.preferred_dialect == CLAP_NOTE_DIALECT_MIDI && (info.supported_dialects & CLAP_NOTE_DIALECT_CLAP))
				{
					m_cache[0] = { index, CLAP_NOTE_DIALECT_MIDI };
					m_best = std::min(0u, m_best);
				}
				// CLAP and MIDI supported
				else if ((info.supported_dialects & CLAP_NOTE_DIALECT_CLAP) && (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI))
				{
					m_cache[1] = { index, CLAP_NOTE_DIALECT_CLAP };
					m_best = std::min(1u, m_best);
				}
				// CLAP supported
				else if (info.supported_dialects & CLAP_NOTE_DIALECT_CLAP)
				{
					m_cache[2] = { index, CLAP_NOTE_DIALECT_CLAP };
					m_best = std::min(2u, m_best);
				}
				// MIDI supported
				else if (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI)
				{
					m_cache[3] = { index, CLAP_NOTE_DIALECT_MIDI };
					m_best = std::min(3u, m_best);
				}
			}
			auto getBest() -> IndexDialectPair
			{
				if (m_best == m_cache.size())
				{
					// No note port supported by host
					return {0, 0};
				}
				return m_cache[m_best];
			}
		private:
			std::array<IndexDialectPair, 4> m_cache; // priority 2 thru 5
			unsigned m_best = static_cast<unsigned>(m_cache.size()); // best port seen so far
		} priorityHelper;

		const auto count = h->m_pluginExtNotePorts->count(h->plugin(), true);
		assert(count < (std::numeric_limits<std::int16_t>::max)()); // just in case
		for (std::uint16_t idx = 0; idx < static_cast<std::uint16_t>(count); ++idx)
		{
			auto info = clap_note_port_info{};
			info.id = CLAP_INVALID_ID;
			if (!h->m_pluginExtNotePorts->get(h->plugin(), idx, true, &info))
			{
				h->log(CLAP_LOG_DEBUG, "Failed to read note port info");
				return;
			}

			// Just in case
			if (info.id == CLAP_INVALID_ID)
			{
				h->log(CLAP_LOG_PLUGIN_MISBEHAVING, "Note port info contains invalid id");
				continue;
			}

			// Check for #1 priority option: CLAP preferred + MIDI (and CLAP) supported
			if (info.preferred_dialect == CLAP_NOTE_DIALECT_CLAP && (info.supported_dialects & CLAP_NOTE_DIALECT_MIDI))
			{
				h->m_notePortIndex = idx;
				h->m_noteDialect = CLAP_NOTE_DIALECT_CLAP;
				return;
			}

			// Check for match with lesser-priority options
			priorityHelper.check(idx, info);
		}

		std::tie(h->m_notePortIndex, h->m_noteDialect) = priorityHelper.getBest();
		// TODO: Also set the 2nd supported note dialect? Or use a boolean to indicate both CLAP and MIDI are supported?
	}
	else if (flags & CLAP_NOTE_PORTS_RESCAN_NAMES)
	{
		// Not implemented
	}
}

void ClapInstance::setParamValueByHost(ClapParam& param, double value)
{
	assert(isMainThread());

	param.setValue(value);

	m_hostToPluginValueQueue.set(param.info().id, {param.info().cookie, value});
	m_hostToPluginValueQueue.producerDone();
	hostExtParamsRequestFlush(host());
}

void ClapInstance::setParamModulationByHost(ClapParam& param, double value)
{
	assert(isMainThread());

	param.setModulation(value);

	m_hostToPluginModQueue.set(param.info().id, {param.info().cookie, value});
	m_hostToPluginModQueue.producerDone();
	hostExtParamsRequestFlush(host());
}

void ClapInstance::checkValidParamValue(const ClapParam& param, double value)
{
	assert(isMainThread());
	if (!param.isValueValid(value))
	{
		std::string msg = "Invalid value for param. " + param.getInfoString() + "; value: " + std::to_string(value);
		throw std::invalid_argument{msg};
	}
}

auto ClapInstance::getParamValue(const clap_param_info& info) const -> double
{
	assert(isMainThread());

	if (!m_pluginExtParams) { return 0.0; }

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
