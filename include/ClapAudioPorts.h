/*
 * ClapAudioPorts.h - Implements CLAP audio ports extension
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

#ifndef LMMS_CLAP_AUDIO_PORTS_H
#define LMMS_CLAP_AUDIO_PORTS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <memory>
#include <vector>
#include <clap/ext/audio-ports.h>

#include "ClapExtension.h"
#include "LmmsTypes.h"
#include "PluginIssue.h"
#include "PluginPortConfig.h"
#include "SampleFrame.h"
#include "lmms_export.h"

namespace lmms
{

//! RAII-enabled CLAP AudioBuffer
class ClapAudioBuffer
{
public:
	ClapAudioBuffer(std::uint32_t channels, fpp_t frames)
		: m_channels{channels}
		, m_frames{frames}
	{
		if (channels == 0) { return; }
		m_data = new float*[m_channels]();
		for (std::uint32_t channel = 0; channel < m_channels; ++channel)
		{
			m_data[channel] = new float[m_frames]();
		}
	}

	ClapAudioBuffer(const ClapAudioBuffer&) = delete;
	auto operator=(const ClapAudioBuffer&) -> ClapAudioBuffer& = delete;

	ClapAudioBuffer(ClapAudioBuffer&& other) noexcept
		: m_channels{std::exchange(other.m_channels, 0)}
		, m_frames{std::exchange(other.m_frames, 0)}
		, m_data{std::exchange(other.m_data, nullptr)}
	{
	}

	auto operator=(ClapAudioBuffer&& other) noexcept -> ClapAudioBuffer&
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

	~ClapAudioBuffer() { free(); }

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
	fpp_t m_frames;
	float** m_data = nullptr;
};

class LMMS_EXPORT ClapAudioPorts final
	: public ClapExtension<clap_host_audio_ports, clap_plugin_audio_ports>
	, public PluginPortConfig
{
public:
	ClapAudioPorts(ClapInstance* parent);
	~ClapAudioPorts() override;

	auto init(clap_process& process) noexcept -> bool;

	auto extensionId() const -> std::string_view override { return CLAP_EXT_AUDIO_PORTS; }

	void copyBuffersFromCore(const SampleFrame* buffer, fpp_t frames);
	void copyBuffersToCore(SampleFrame* buffer, fpp_t frames) const;

private:
	auto hostExtImpl() const -> const clap_host_audio_ports* override { return nullptr; } // not impl for host yet
	auto checkSupported(const clap_plugin_audio_ports& ext) -> bool override;

	std::vector<PluginIssue> m_issues; // TODO: Remove?

	/**
	 * Process-related
	*/
	std::vector<clap_audio_buffer_t> m_audioIn, m_audioOut;
	clap_audio_buffer_t* m_audioInActive = nullptr; //!< Pointer to m_audioIn element used by LMMS
	clap_audio_buffer_t* m_audioOutActive = nullptr; //!< Pointer to m_audioOut element used by LMMS

	std::vector<ClapAudioBuffer> m_audioInBuffers, m_audioOutBuffers; //!< [port][channel][frame]

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
		PluginPortConfig::PortType type = PluginPortConfig::PortType::None; // None = unsupported
		bool used = false; //!< In use by LMMS
	};

	std::vector<AudioPort> m_audioPortsIn, m_audioPortsOut;
	AudioPort* m_audioPortInActive = nullptr; //!< Pointer to m_audioPortsIn element used by LMMS
	AudioPort* m_audioPortOutActive = nullptr; //!< Pointer to m_audioPortsOut element used by LMMS

};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_AUDIO_PORTS_H
