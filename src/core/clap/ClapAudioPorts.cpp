/*
 * ClapAudioPorts.cpp - Implements CLAP audio ports extension
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

#include "ClapAudioPorts.h"

#ifdef LMMS_HAVE_CLAP

#include <cassert>

#include "ClapInstance.h"
#include "AudioEngine.h"

namespace lmms
{

namespace
{
	template<bool stereo>
	inline void copyBuffersHostToPlugin(const SampleFrame* hostBuf, float** pluginBuf,
		unsigned channel, fpp_t frames)
	{
		for (fpp_t f = 0; f < frames; ++f)
		{
			pluginBuf[0][f] = hostBuf[f][channel];
			if constexpr (stereo) { pluginBuf[1][f] = hostBuf[f][channel + 1]; }
		}
	}

	inline void copyBuffersStereoHostToMonoPlugin(const SampleFrame* hostBuf, float** pluginBuf,
		unsigned channel, fpp_t frames)
	{
		for (fpp_t f = 0; f < frames; ++f)
		{
			pluginBuf[0][f] = (hostBuf[f][channel] + hostBuf[f][channel + 1]) / 2.0f;
		}
	}

	template<bool stereo>
	inline void copyBuffersPluginToHost(float** pluginBuf, SampleFrame* hostBuf,
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

	inline void copyBuffersMonoPluginToStereoHost(float** pluginBuf, SampleFrame* hostBuf,
		std::uint64_t constantMask, unsigned channel, fpp_t frames)
	{
		const bool isConstant = (constantMask & (1 << 0)) != 0;
		for (fpp_t f = 0; f < frames; ++f)
		{
			hostBuf[f][channel] = hostBuf[f][channel + 1] = pluginBuf[0][isConstant ? 0 : f];
		}
	}
} // namespace

ClapAudioPorts::ClapAudioPorts(ClapInstance* parent)
	: ClapExtension{parent}
	, PluginPortConfig{parent->parent()}
{
}

ClapAudioPorts::~ClapAudioPorts()
{
	deinit();
}

auto ClapAudioPorts::init(clap_process& process) noexcept -> bool
{
	// TODO: Refactor so that regular init() method can be used (Possible lazy extension init issues)

	// NOTE: I'm using this init() method instead of implementing initImpl() because I need the `process` parameter
	if (!ClapExtension::init())
	{
		logger().log(CLAP_LOG_ERROR, "Plugin does not implement the required audio port extension");
		return false;
	}

	// Clear everything just to be safe
	m_issues.clear();
	m_audioIn.clear();
	m_audioOut.clear();
	m_audioInActive = m_audioOutActive = nullptr;
	m_audioPortsIn.clear();
	m_audioPortsOut.clear();
	m_audioPortInActive = m_audioPortOutActive = nullptr;
	m_audioInBuffers.clear();
	m_audioOutBuffers.clear();

	// Effect, Instrument, and Tool are the only options
	const bool needInputPort = instance()->info().type() != Plugin::Type::Instrument;
	constexpr bool needOutputPort = true;

	bool hasStereoInput = false;
	bool hasStereoOutput = false;

	auto readPorts = [&](
		std::vector<AudioPort>& audioPorts,
		std::vector<clap_audio_buffer>& audioBuffers,
		std::vector<ClapAudioBuffer>& rawAudioBuffers,
		bool isInput) -> AudioPort*
	{
		if (isInput && !needInputPort)
		{
			logger().log(CLAP_LOG_DEBUG, "Skipping plugin's audio input ports (not needed)");
			return nullptr;
		}

		if (!isInput && !needOutputPort)
		{
			logger().log(CLAP_LOG_DEBUG, "Skipping plugin's audio output ports (not needed)");
			return nullptr;
		}

		const auto portCount = pluginExt()->count(this->plugin(), isInput);

		if (isInput)
		{
			hasStereoInput = false; // initialize

			//if (portCount == 0 && m_pluginInfo->getType() == Plugin::PluginTypes::Effect)
			//	m_issues.emplace_back( ... );
		}
		else
		{
			hasStereoOutput = false; // initialize

			if (portCount == 0 && needOutputPort)
			{
				m_issues.emplace_back(PluginIssueType::NoOutputChannel);
			}
			//if (portCount > 2)
			//	m_issues.emplace_back(PluginIssueType::tooManyOutputChannels, std::to_string(outCount));
		}

#if 0
	{
		std::string msg = (isInput ? "Input ports: " : "Output ports: ") + std::to_string(portCount);
		logger()->log(CLAP_LOG_DEBUG, msg);
	}
#endif

		clap_id monoPort = CLAP_INVALID_ID;
		clap_id stereoPort = CLAP_INVALID_ID;
		//clap_id mainPort = CLAP_INVALID_ID;
		for (std::uint32_t idx = 0; idx < portCount; ++idx)
		{
			auto info = clap_audio_port_info{};
			info.id = CLAP_INVALID_ID;
			info.in_place_pair = CLAP_INVALID_ID;
			if (!pluginExt()->get(this->plugin(), idx, isInput, &info))
			{
				logger().log(CLAP_LOG_ERROR, "Unknown error calling clap_plugin_audio_ports.get()");
				m_issues.emplace_back(PluginIssueType::PortHasNoDef);
				return nullptr;
			}

			if (idx == 0 && !(info.flags & CLAP_AUDIO_PORT_IS_MAIN))
			{
				logger().log(CLAP_LOG_DEBUG, "Plugin audio port #0 is not main");
			}

			//if (info.flags & CLAP_AUDIO_PORT_IS_MAIN)
			//	mainPort = idx;

			auto type = PluginPortConfig::PortType::None;
			if (info.port_type)
			{
				auto portType = std::string_view{info.port_type};
				if (portType == CLAP_PORT_MONO)
				{
					assert(info.channel_count == 1);
					type = PluginPortConfig::PortType::Mono;
					if (monoPort == CLAP_INVALID_ID) { monoPort = idx; }
				}

				if (portType == CLAP_PORT_STEREO)
				{
					assert(info.channel_count == 2);
					type = PluginPortConfig::PortType::Stereo;
					if (stereoPort == CLAP_INVALID_ID) { stereoPort = idx; }
				}
			}
			else
			{
				if (info.channel_count == 1)
				{
					type = PluginPortConfig::PortType::Mono;
					if (monoPort == CLAP_INVALID_ID) { monoPort = idx; }
				}
				else if (info.channel_count == 2)
				{
					type = PluginPortConfig::PortType::Stereo;
					if (stereoPort == CLAP_INVALID_ID) { stereoPort = idx; }
				}
			}

#if 0
			{
				std::string msg = "---audio port---\nid: ";
				msg += std::to_string(info.id);
				msg += "\nname: ";
				msg += info.name;
				msg += "\nflags: ";
				msg += std::to_string(info.flags);
				msg += "\nchannel count: ";
				msg += std::to_string(info.channel_count);
				msg += "\ntype: ";
				msg += (info.port_type ? info.port_type : "(null)");
				msg += "\nin place pair: ";
				msg += std::to_string(info.in_place_pair);
				logger()->log(CLAP_LOG_DEBUG, msg);
			}
#endif

			audioPorts.emplace_back(AudioPort{info, idx, isInput, type, false});
		}

		assert(portCount == audioPorts.size());
		audioBuffers.reserve(audioPorts.size());
		for (std::uint32_t port = 0; port < audioPorts.size(); ++port)
		{
			auto& buffer = audioBuffers.emplace_back();

			const auto channelCount = audioPorts[port].info.channel_count;
			if (channelCount <= 0)
			{
				logger().log(CLAP_LOG_WARNING, "Audio port channel count is zero");
				//return nullptr;
			}

			buffer.channel_count = channelCount;
			if (isInput && port != monoPort && (port != stereoPort || stereoPort == CLAP_INVALID_ID))
			{
				// This input port will not be used by LMMS
				// TODO: Will a mono port ever need to be used if a stereo port is available?
				buffer.constant_mask = static_cast<std::uint64_t>(-1);
			}
			else
			{
				buffer.constant_mask = 0;
			}

			auto& rawBuffer = rawAudioBuffers.emplace_back(channelCount, DEFAULT_BUFFER_SIZE);
			buffer.data32 = rawBuffer.data();
			buffer.data64 = nullptr; // Not supported by LMMS
			buffer.latency = 0; // TODO: latency extension
		}

		if (stereoPort != CLAP_INVALID_ID)
		{
			if (isInput) { hasStereoInput = true; } else { hasStereoOutput = true; }
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
		{
			std::string msg = std::string{isInput ? "An input" : "An output"}
				+ " audio port is required, but plugin has none that are usable";
			logger().log(CLAP_LOG_ERROR, msg);
		}
		m_issues.emplace_back(PluginIssueType::UnknownPortType); // TODO: Add better entry to PluginIssueType
		return nullptr;
	};

	m_audioPortInActive = readPorts(m_audioPortsIn, m_audioIn, m_audioInBuffers, true);
	if (!m_issues.empty() || (!m_audioPortInActive && needInputPort))
	{
		return false;
	}

	process.audio_inputs = m_audioIn.data();
	process.audio_inputs_count = m_audioPortsIn.size();
	m_audioInActive = m_audioPortInActive ? &m_audioIn[m_audioPortInActive->index] : nullptr;

	m_audioPortOutActive = readPorts(m_audioPortsOut, m_audioOut, m_audioOutBuffers, false);
	if (!m_issues.empty() || (!m_audioPortOutActive && needOutputPort))
	{
		return false;
	}

	process.audio_outputs = m_audioOut.data();
	process.audio_outputs_count = m_audioPortsOut.size();
	m_audioOutActive = m_audioPortOutActive ? &m_audioOut[m_audioPortOutActive->index] : nullptr;

	if (needInputPort)
	{
		if (!hasStereoInput && m_audioPortInActive->type != PluginPortConfig::PortType::Mono)
		{
			return false;
		}
		if (hasStereoInput && m_audioPortInActive->type != PluginPortConfig::PortType::Stereo)
		{
			return false;
		}
	}

	if (needOutputPort)
	{
		if (!hasStereoOutput && m_audioPortOutActive->type != PluginPortConfig::PortType::Mono)
		{
			return false;
		}
		if (hasStereoOutput && m_audioPortOutActive->type != PluginPortConfig::PortType::Stereo)
		{
			return false;
		}
	}

	setPortType(
		m_audioPortInActive ? m_audioPortInActive->type : PluginPortConfig::PortType::None,
		m_audioPortOutActive ? m_audioPortOutActive->type : PluginPortConfig::PortType::None);

	return true;
}

auto ClapAudioPorts::checkSupported(const clap_plugin_audio_ports& ext) -> bool
{
	return ext.count && ext.get;
}

void ClapAudioPorts::copyBuffersFromCore(const SampleFrame* buffer, fpp_t frames)
{
	switch (portConfig<true>())
	{
		case PluginPortConfig::Config::None:
			break;
		case PluginPortConfig::Config::Stereo:
			// LMMS stereo to CLAP stereo input
			copyBuffersHostToPlugin<true>(buffer, m_audioInActive->data32, 0, frames);
			break;
		case PluginPortConfig::Config::MonoMix:
			// LMMS stereo downmix to mono for CLAP mono input
			copyBuffersStereoHostToMonoPlugin(buffer, m_audioInActive->data32, 0, frames);
			break;
		case PluginPortConfig::Config::LeftOnly:
			// LMMS left channel to CLAP mono input; LMMS right channel bypassed
			copyBuffersHostToPlugin<false>(buffer, m_audioInActive->data32, 0, frames);
			break;
		case PluginPortConfig::Config::RightOnly:
			// LMMS right channel to CLAP mono input; LMMS left channel bypassed
			copyBuffersHostToPlugin<false>(buffer, m_audioInActive->data32, 1, frames);
			break;
	}
}

void ClapAudioPorts::copyBuffersToCore(SampleFrame* buffer, fpp_t frames) const
{
	switch (portConfig<false>())
	{
		case PluginPortConfig::Config::None:
			break;
		case PluginPortConfig::Config::Stereo:
			// CLAP stereo output to LMMS stereo
			copyBuffersPluginToHost<true>(m_audioOutActive->data32, buffer,
				m_audioOutActive->constant_mask, 0, frames);
			break;
		case PluginPortConfig::Config::MonoMix:
			// CLAP mono output upmix to stereo for LMMS stereo
			copyBuffersMonoPluginToStereoHost(m_audioOutActive->data32, buffer,
				m_audioOutActive->constant_mask, 0, frames);
			break;
		case PluginPortConfig::Config::LeftOnly:
			// CLAP mono output to LMMS left channel
			copyBuffersPluginToHost<false>(m_audioOutActive->data32, buffer,
				m_audioOutActive->constant_mask, 0, frames);
			break;
		case PluginPortConfig::Config::RightOnly:
			// CLAP mono output to LMMS right channel
			copyBuffersPluginToHost<false>(m_audioOutActive->data32, buffer,
				m_audioOutActive->constant_mask, 1, frames);
			break;
	}
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
