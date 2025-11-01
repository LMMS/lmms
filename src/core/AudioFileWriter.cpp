/*
 * AudioFileWriter.cpp
 *
 * Copyright (c) 2025 saker <sakertooth@gmail.com>
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

#include "AudioFileWriter.h"

#include <sndfile.h>

namespace {
using namespace lmms;

SNDFILE* openAudioFile(const std::filesystem::path& path, int mode, SF_INFO* sfinfo)
{
#ifdef LMMS_BUILD_WIN32
	return sf_wchar_open(path.c_str(), mode, sfinfo);
#else
	return sf_open(path.c_str(), mode, sfinfo);
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
		break;
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
struct AudioFileWriter::Impl
{
	Impl(std::filesystem::path path, AudioFileFormat format, OutputSettings settings);
	SF_INFO m_info;
	SNDFILE* m_sndfile;
	std::filesystem::path m_path;
};

AudioFileWriter::AudioFileWriter(std::filesystem::path path, AudioFileFormat format, OutputSettings settings)
	: m_impl(std::make_unique<Impl>(std::move(path), format, settings))
{
	if (!m_impl->m_sndfile) { throw std::runtime_error{sf_strerror(m_impl->m_sndfile)}; }
}

AudioFileWriter::~AudioFileWriter() = default;

AudioFileWriter::Impl::Impl(std::filesystem::path path, AudioFileFormat format, OutputSettings settings)
	: m_info(sfInfoFromOutputSettings(format, settings))
	, m_sndfile(openAudioFile(path, SFM_WRITE, &m_info))
	, m_path(std::move(path))
{
	// This only works for a select number of formats.
	// Formats like MP3 silently fail/do nothing (according to local testing).
	// one option could be to introduce TagLib and use that instead for metadata,
	// or we transition to using FFmpeg which can handle metadata.
	sf_set_string(m_sndfile, SF_STR_COMMENT, "Created with LMMS");

	if (format == AudioFileFormat::FLAC)
	{
		auto compressionLevel = settings.getCompressionLevel();
		sf_command(m_sndfile, SFC_SET_COMPRESSION_LEVEL, &compressionLevel, sizeof(int));
	}
	else if (format == AudioFileFormat::MP3)
	{
		constexpr auto minBitRate = 32;
		constexpr auto maxBitRate = 320;
		const auto targetBitRate = settings.bitrate();

		auto mode = SF_BITRATE_MODE_CONSTANT;
		sf_command(m_sndfile, SFC_SET_BITRATE_MODE, &mode, sizeof(int));

		auto compressionLevel = (maxBitRate - targetBitRate) / static_cast<double>(maxBitRate - minBitRate);
		sf_command(m_sndfile, SFC_SET_COMPRESSION_LEVEL, &compressionLevel, sizeof(double));
	}
	else if (format == AudioFileFormat::OGG)
	{
		assert(m_info.channels == 1 || m_info.channels == 2 && "invalid channel count");

		constexpr auto monoBitrateTargets = std::array<double, 12>{
			32000., 48000., 60000., 70000., 80000., 86000., 96000., 110000., 120000., 140000., 160000., 240001.};

		constexpr auto stereoBitrateTargets = std::array<double, 12>{22500. * 2, 32000. * 2, 40000. * 2, 48000. * 2,
			56000. * 2, 64000. * 2, 80000. * 2, 96000. * 2, 112000. * 2, 128000. * 2, 160000. * 2, 250001. * 2};

		const auto& bitrateTargets = m_info.channels == 1 ? monoBitrateTargets : stereoBitrateTargets;
		const auto bitrateTargetIt
			= std::lower_bound(bitrateTargets.begin(), bitrateTargets.end(), settings.bitrate() * 1000);
		assert(bitrateTargetIt != bitrateTargets.end() && "invalid bitrate");

		const auto upperIndex = std::distance(bitrateTargets.begin(), bitrateTargetIt);
		const auto lowerIndex = upperIndex == 0 ? 0 : upperIndex - 1;

		const auto bitrateLow = bitrateTargets[lowerIndex];
		const auto bitrateHigh = bitrateTargets[upperIndex];
		const auto bitrateFractionalIndex = (settings.bitrate() * 1000 - bitrateLow) / (bitrateHigh - bitrateLow);
		const auto bitrateIndex = lowerIndex + bitrateFractionalIndex;

		const auto qualityLevel = 1.1 / static_cast<double>(bitrateTargets.size() - 1) * bitrateIndex - .1;
		auto compressionLevel = std::clamp(1 - qualityLevel, 0., 1.);
		sf_command(m_sndfile, SFC_SET_COMPRESSION_LEVEL, &compressionLevel, sizeof(double));
	}
}

auto AudioFileWriter::read(InterleavedBufferView<float> dst) -> std::size_t
{
	assert(dst.channels() == m_impl->m_info.channels && "invalid channel count");
	return sf_readf_float(m_impl->m_sndfile, dst.data(), dst.frames());
}

auto AudioFileWriter::write(InterleavedBufferView<const float> src) -> std::size_t
{
	assert(src.channels() == m_impl->m_info.channels && "invalid channel count");
	return sf_writef_float(m_impl->m_sndfile, src.data(), src.frames());
}

auto AudioFileWriter::frames() const -> f_cnt_t
{
	return m_impl->m_info.frames;
}

auto AudioFileWriter::channels() const -> ch_cnt_t
{
	return m_impl->m_info.channels;
}

auto AudioFileWriter::sampleRate() const -> sample_rate_t
{
	return m_impl->m_info.samplerate;
}

auto AudioFileWriter::path() const -> const std::filesystem::path&
{
	return m_impl->m_path;
}

} // namespace lmms
