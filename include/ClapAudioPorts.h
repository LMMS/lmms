/*
 * ClapAudioPorts.h - Implements CLAP audio ports extension
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

#ifndef LMMS_CLAP_AUDIO_PORTS_H
#define LMMS_CLAP_AUDIO_PORTS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include <memory>
#include <vector>

#include <clap/ext/audio-ports.h>

#include "ClapExtension.h"
#include "PluginIssue.h"
#include "lmms_basics.h"

namespace lmms
{

//! RAII-enabled CLAP AudioBuffer
class ClapAudioBuffer
{
public:
	ClapAudioBuffer(std::uint32_t channels, std::uint32_t frames)
		: m_channels(channels), m_frames(frames)
	{
		m_data = new float*[m_channels]();
		for (std::uint32_t channel = 0; channel < m_channels; ++channel)
		{
			m_data[channel] = new float[m_frames]();
		}
	}

	ClapAudioBuffer(const ClapAudioBuffer&) = delete;
	ClapAudioBuffer& operator=(const ClapAudioBuffer&) = delete;

	ClapAudioBuffer(ClapAudioBuffer&& other) noexcept :
		m_channels(std::exchange(other.m_channels, 0)),
		m_frames(std::exchange(other.m_frames, 0)),
		m_data(std::exchange(other.m_data, nullptr))
	{
	}

	ClapAudioBuffer& operator=(ClapAudioBuffer&& other) noexcept
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
	std::uint32_t m_frames;
	float** m_data = nullptr;
};

class ClapAudioPorts final : public ClapExtension<clap_host_audio_ports, clap_plugin_audio_ports>
{
public:
	using ClapExtension::ClapExtension;
	~ClapAudioPorts() override { deinit(); }

	auto init(const clap_host* host, const clap_plugin* plugin, clap_process& process) noexcept -> bool;

	auto extensionId() const -> std::string_view override { return CLAP_EXT_AUDIO_PORTS; }
	auto hostExt() const -> const clap_host_audio_ports* override { return nullptr; } // not impl for host yet

	auto hasStereoInput() const { return m_hasStereoInput; } //!< Can call after init()
	auto hasStereoOutput() const { return m_hasStereoOutput; } //!< Can call after init()

	/**
	 * Copy buffer passed by the core into our ports
	 * @param buf buffer of sample frames, each sample frame is something like
	 *   a `float[<number-of-procs> * <channels per proc>]` array.
	 * @param firstChannel The offset for @p buf where we have to read our
	 *   first channel.
	 *   This marks the first sample in each sample frame where we read from.
	 *   If we are the 2nd of 2 mono procs, this can be greater than 0.
	 * @param numChannels Number of channels we must read from @param buf (starting at
	 *   @p offset)
	 */
	void copyBuffersFromCore(const sampleFrame* buf, unsigned firstChannel, unsigned numChannels, fpp_t frames);

	/**
	 * Copy our ports into buffers passed by the core
	 * @param buf buffer of sample frames, each sample frame is something like
	 *   a `float[<number-of-procs> * <channels per proc>]` array.
	 * @param firstChannel The offset for @p buf where we have to write our
	 *   first channel.
	 *   This marks the first sample in each sample frame where we write to.
	 *   If we are the 2nd of 2 mono procs, this can be greater than 0.
	 * @param numChannels Number of channels we must write to @param buf (starting at
	 *   @p offset)
	 */
	void copyBuffersToCore(sampleFrame* buf, unsigned firstChannel, unsigned numChannels, fpp_t frames) const;

private:
	auto checkSupported(const clap_plugin_audio_ports& ext) -> bool override;

	bool m_hasStereoInput = false;
	bool m_hasStereoOutput = false;

	std::vector<PluginIssue> m_issues;

	/**
	 * Process-related
	*/
	std::unique_ptr<clap_audio_buffer_t[]> m_audioIn, m_audioOut; // TODO: Why not use a std::vector?
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
		AudioPortType type = AudioPortType::Unsupported;
		bool used = false; //!< In use by LMMS
	};

	std::vector<AudioPort> m_audioPortsIn, m_audioPortsOut;
	AudioPort* m_audioPortInActive = nullptr; //!< Pointer to m_audioPortsIn element used by LMMS
	AudioPort* m_audioPortOutActive = nullptr; //!< Pointer to m_audioPortsOut element used by LMMS

};

} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_AUDIO_PORTS_H
