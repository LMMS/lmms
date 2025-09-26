/*
 * AudioFile.cpp
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#include "AudioFile.h"

#include <sndfile.h>

namespace {
using namespace lmms;

SNDFILE* openAudioFile(std::filesystem::path path, int mode, SF_INFO* sfinfo)
{
#ifdef LMMS_BUILD_WIN32
	return sf_wchar_open(path.wstring().c_str(), mode, sfinfo);
#else
	return sf_open(path.string().c_str(), mode, sfinfo);
#endif
}

SF_INFO sfInfoFromOutputSettings(AudioFileFormat format, OutputSettings settings)
{
    auto sfFormat = 0;
    auto sfBitDepth = 0;
    auto sfChannels = 0;

    switch (settings.getBitDepth())
    {
	case OutputSettings::BitDepth::Depth16Bit:
        sfBitDepth = SF_FORMAT_PCM_16;
        break;
	case OutputSettings::BitDepth::Depth24Bit:
        sfBitDepth = SF_FORMAT_PCM_24;
        break;
	case OutputSettings::BitDepth::Depth32Bit:
        sfBitDepth = SF_FORMAT_FLOAT;
		break;
	}

	switch (format)
    {
	case AudioFileFormat::WAV:
        sfFormat = SF_FORMAT_WAV | sfBitDepth;
        break;
	case AudioFileFormat::FLAC:
        sfFormat = SF_FORMAT_FLAC | sfBitDepth;
        break;
	case AudioFileFormat::OGG:
        sfFormat = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
        break;
	case AudioFileFormat::MP3:
        sfFormat = SF_FORMAT_MPEG | SF_FORMAT_MPEG_LAYER_III;
        break;
    default:
        return SF_INFO{};
	}

    switch (settings.getStereoMode())
    {
	case OutputSettings::StereoMode::Stereo:
	case OutputSettings::StereoMode::JointStereo:
        sfChannels = 2;
        break;
	case OutputSettings::StereoMode::Mono:
        sfChannels = 1;
		break;
	}

	return {.samplerate = static_cast<int>(settings.getSampleRate()), .channels = sfChannels, .format = sfFormat};
}

} // namespace

namespace lmms {
struct AudioFile::Impl
{
	Impl(std::filesystem::path path);
	Impl(std::filesystem::path path, AudioFileFormat format, OutputSettings settings);
	SF_INFO m_info;
	SNDFILE* m_sndfile;
	std::filesystem::path m_path;
};

AudioFile::AudioFile(std::filesystem::path path)
	: m_impl(std::make_unique<Impl>(path))
{
	if (!m_impl->m_sndfile)
	{
		throw std::runtime_error{"failed to construct audio file: " + std::string{sf_strerror(m_impl->m_sndfile)}};
	}
}

AudioFile::AudioFile(std::filesystem::path path, AudioFileFormat format, OutputSettings settings)
	: m_impl(std::make_unique<Impl>(path, format, settings))
{
    if (!m_impl->m_sndfile)
	{
		throw std::runtime_error{"failed to construct audio file: " + std::string{sf_strerror(m_impl->m_sndfile)}};
	}
}

AudioFile::~AudioFile() = default;

AudioFile::Impl::Impl(std::filesystem::path path)
	: m_info()
	, m_sndfile(openAudioFile(path, SFM_READ, &m_info))
	, m_path(path)
{
}

AudioFile::Impl::Impl(std::filesystem::path path, AudioFileFormat format, OutputSettings settings)
	: m_info(sfInfoFromOutputSettings(format, settings))
	, m_sndfile(openAudioFile(path, SFM_WRITE, &m_info))
	, m_path(path)
{
	if (format == AudioFileFormat::FLAC)
	{
		auto compressionLevel = settings.getCompressionLevel();
		sf_command(m_sndfile, SFC_SET_COMPRESSION_LEVEL, &compressionLevel, sizeof(int));
	}
	else if (format == AudioFileFormat::MP3 || format == AudioFileFormat::OGG)
	{
		constexpr auto minBitRate = 32;
		constexpr auto maxBitRate = 320;
		const auto targetBitRate = settings.bitrate();

		auto compressionLevel = (maxBitRate - targetBitRate) / static_cast<double>(maxBitRate - minBitRate);
		sf_command(m_sndfile, SFC_SET_COMPRESSION_LEVEL, &compressionLevel, sizeof(double));
	}
}

void AudioFile::read(InterleavedBufferView<float> dst)
{
    assert(dst.channels() == m_impl->m_info.channels && "invalid channel count");
    sf_readf_float(m_impl->m_sndfile, dst.data(), dst.frames());
}

void AudioFile::write(InterleavedBufferView<const float> src)
{
	assert(src.channels() == m_impl->m_info.channels && "invalid channel count");
	sf_writef_float(m_impl->m_sndfile, src.data(), src.frames());
}

auto AudioFile::frames() const -> f_cnt_t
{
	return m_impl->m_info.frames;
}

auto AudioFile::channels() const -> ch_cnt_t
{
	return m_impl->m_info.channels;
}

auto AudioFile::sampleRate() const -> sample_rate_t
{
	return m_impl->m_info.samplerate;
}

auto AudioFile::path() const -> std::filesystem::path
{
	return m_impl->m_path;
}

} // namespace lmms