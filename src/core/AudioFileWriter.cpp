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

namespace lmms {

namespace {

class Backend
{
public:
	Backend() = default;
	Backend(const Backend&) = default;
	Backend(Backend&&) = delete;
	Backend& operator=(const Backend&) = default;
	Backend& operator=(Backend&&) = delete;

	virtual ~Backend() = default;
	virtual auto write(InterleavedBufferView<const float> src) -> std::size_t = 0;
	virtual auto frames() const -> f_cnt_t = 0;
	virtual auto channels() const -> ch_cnt_t = 0;
	virtual auto sampleRate() const -> sample_rate_t = 0;
};

class SndfileBackend : public Backend
{
public:
	SndfileBackend(const std::filesystem::path& path, AudioFileFormat format, OutputSettings settings)
	{
		auto sfBitDepth = 0;
		auto sfFormat = 0;

		switch (settings.getBitDepth())
		{
		case OutputSettings::BitDepth::Depth16Bit:
			sfBitDepth = SF_FORMAT_PCM_16;
			break;
		case OutputSettings::BitDepth::Depth24Bit:
			sfBitDepth = SF_FORMAT_PCM_24;
			break;
		case OutputSettings::BitDepth::Depth32Bit:
			sfBitDepth = SF_FORMAT_PCM_32;
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
		default:
			break;
		}

		m_info.samplerate = settings.getSampleRate();
		m_info.channels = settings.getStereoMode() == OutputSettings::StereoMode::Mono ? 1 : 2;
		m_info.format = sfFormat;

#ifdef LMMS_BUILD_WIN32
		m_sndfile = sf_wchar_open(path.c_str(), SFM_WRITE, &m_info);
#else
		m_sndfile = sf_open(path.c_str(), SFM_WRITE, &m_info);
#endif

		if (!m_sndfile) { throw std::runtime_error{"failed to load file using sndfile"}; }

		if (format == AudioFileFormat::FLAC)
		{
			auto compressionLevel = settings.getCompressionLevel();
			sf_command(m_sndfile, SFC_SET_COMPRESSION_LEVEL, &compressionLevel, sizeof(int));
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

	auto write(InterleavedBufferView<const float> src) -> std::size_t override
	{
		assert(m_info.channels() == src.channels());
		return sf_writef_float(m_sndfile, src.data(), src.frames());
	}

	auto frames() const -> f_cnt_t override
	{
		return m_info.frames;
	}

	auto channels() const -> ch_cnt_t override
	{
		return m_info.channels;
	}

	auto sampleRate() const -> sample_rate_t override
	{
		return m_info.samplerate;
	}

private:
	SNDFILE* m_sndfile;
	SF_INFO m_info{};
};

class LameBackend : public Backend
{
public:
	LameBackend(const std::filesystem::path& path, AudioFileFormat format, OutputSettings settings)
	{
	}

	auto write(InterleavedBufferView<const float> src) -> std::size_t override
	{
	}

	auto frames() const -> f_cnt_t override
	{
	}

	auto channels() const -> ch_cnt_t override
	{
	}

	auto sampleRate() const -> sample_rate_t override
	{
	}
};

}

struct AudioFileWriter::Impl
{
	std::unique_ptr<Backend> m_backend;
};

AudioFileWriter::AudioFileWriter(std::filesystem::path path, AudioFileFormat format, OutputSettings settings)
	: m_path(path)
{
	m_impl = std::make_unique<Impl>();

	if (format == AudioFileFormat::MP3)
	{
		// Note: Sndfile supports MP3 in version 1.1.0 and greater but is still missing some features, such as adding
		// comments to the files and an option for joint stereo (which might be removed in the future if there is little
		// need for it.). We also are still using 1.0.29 in our CI builds.
		m_impl->m_backend = std::make_unique<LameBackend>(path, format, settings);
	}
	else
	{
		m_impl->m_backend = std::make_unique<SndfileBackend>(path, format, settings);
	}
}

AudioFileWriter::~AudioFileWriter() = default;

auto AudioFileWriter::write(InterleavedBufferView<const float> src) -> std::size_t
{
	return m_impl->m_backend->write(src);
}

auto AudioFileWriter::frames() const -> f_cnt_t
{
	return m_impl->m_backend->frames();
}

auto AudioFileWriter::channels() const -> ch_cnt_t
{
	return m_impl->m_backend->channels();
}

auto AudioFileWriter::sampleRate() const -> sample_rate_t
{
	return m_impl->m_backend->sampleRate();
}

auto AudioFileWriter::path() const -> const std::filesystem::path&
{
	return m_path;
}

} // namespace lmms
