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

#include <array>
#include <fstream>
#include <sndfile.h>
#include <vector>
#include "AudioEngine.h"

#ifdef LMMS_HAVE_MP3LAME
#include <lame/lame.h>
#endif

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
		m_info.samplerate = settings.getSampleRate();
		m_info.channels = settings.getStereoMode() == OutputSettings::StereoMode::Mono ? 1 : 2;
		m_info.format = formatFromSettings(format, settings.getBitDepth());

#ifdef LMMS_BUILD_WIN32
		m_sndfile = sf_wchar_open(path.c_str(), SFM_WRITE, &m_info);
#else
		m_sndfile = sf_open(path.c_str(), SFM_WRITE, &m_info);
#endif

		if (!m_sndfile) { throw std::runtime_error{"failed to load file using sndfile"}; }

		switch (format)
		{
		case AudioFileFormat::FLAC:
			applyFlacSettings(settings);
			return;
		case AudioFileFormat::OGG:
			applyOggSettings(settings);
			return;
		default:
			break;
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
	auto bitDepthFromSettings(OutputSettings::BitDepth bitDepth) -> int
	{
		switch (bitDepth)
		{
		case OutputSettings::BitDepth::Depth16Bit:
			return SF_FORMAT_PCM_16;
		case OutputSettings::BitDepth::Depth24Bit:
			return SF_FORMAT_PCM_24;
		case OutputSettings::BitDepth::Depth32Bit:
			return SF_FORMAT_PCM_32;
		default:
			return 0;
		}
	}

	auto formatFromSettings(AudioFileFormat format, OutputSettings::BitDepth bitDepth) -> int
	{
		auto sfBitDepth = bitDepthFromSettings(bitDepth);
		switch (format)
		{
		case AudioFileFormat::WAV:
			return SF_FORMAT_WAV | sfBitDepth;
		case AudioFileFormat::FLAC:
			return SF_FORMAT_FLAC | sfBitDepth;
		case AudioFileFormat::OGG:
			return SF_FORMAT_OGG | SF_FORMAT_VORBIS;
		default:
			return 0;
		}
	}

	void applyFlacSettings(OutputSettings settings)
	{
		auto compressionLevel = settings.getCompressionLevel();
		sf_command(m_sndfile, SFC_SET_COMPRESSION_LEVEL, &compressionLevel, sizeof(int));
	}

	void applyOggSettings(OutputSettings settings)
	{
		constexpr auto minNominalBitrate = SUPPORTED_OGG_BITRATES.front();
		constexpr auto maxNominalBitrate = SUPPORTED_OGG_BITRATES.back();
		auto compressionLevel = std::lerp(1, 0, (settings.bitrate() - minNominalBitrate) / (maxNominalBitrate - minNominalBitrate));
		sf_command(m_sndfile, SFC_SET_COMPRESSION_LEVEL, &compressionLevel, sizeof(double));
	}

	SNDFILE* m_sndfile;
	SF_INFO m_info{};
};

#ifdef LMMS_HAVE_MP3LAME
class LameBackend : public Backend
{
public:
	LameBackend(const LameBackend&) = delete;
	LameBackend(LameBackend&&) = delete;
	LameBackend& operator=(const LameBackend&) = delete;
	LameBackend& operator=(LameBackend&&) = delete;

	LameBackend(const std::filesystem::path& path, AudioFileFormat format, OutputSettings settings)
		: m_lame(lame_init())
		, m_flushBuffer(7200)
	{
		lame_set_num_channels(m_lame, settings.getStereoMode() == OutputSettings::StereoMode::Mono ? 1 : 2);
		lame_set_in_samplerate(m_lame, settings.getSampleRate());
		lame_set_brate(m_lame, settings.bitrate());
		lame_set_mode(m_lame, modeFromSettings(settings.getStereoMode()));

		id3tag_init(m_lame);
		id3tag_set_comment(m_lame, "Created with LMMS");
	}

	~LameBackend()
	{
		lame_encode_flush(m_lame, m_flushBuffer.data(), m_flushBuffer.size());
		m_file.write(reinterpret_cast<const char*>(m_flushBuffer.data()), m_flushBuffer.size());
		lame_close(m_lame);
	}

	auto write(InterleavedBufferView<const float> src) -> std::size_t override
	{
		assert(src.channels() == 1 || src.channels() == 2 && "unsupported channel count");

		m_encodeBuffer.resize(1.25 * src.frames() + 7200);

		if (src.channels() == 2)
		{
			lame_encode_buffer_interleaved_ieee_float(
				m_lame, src.data(), src.frames(), m_encodeBuffer.data(), m_encodeBuffer.size());
		}
		else if (src.channels() == 1)
		{
			lame_encode_buffer_float(
				m_lame, src.data(), src.data(), src.frames(), m_encodeBuffer.data(), m_encodeBuffer.size());
		}

		m_file.write(reinterpret_cast<const char*>(m_encodeBuffer.data()), m_encodeBuffer.size());
		return src.frames();
	}

	auto frames() const -> f_cnt_t override { return lame_get_frameNum(m_lame); }

	auto channels() const -> ch_cnt_t override { return lame_get_num_channels(m_lame); }

	auto sampleRate() const -> sample_rate_t override { return lame_get_in_samplerate(m_lame); }

private:
	auto modeFromSettings(OutputSettings::StereoMode stereoMode) -> MPEG_mode
	{
		switch (stereoMode)
		{
		case OutputSettings::StereoMode::Stereo:
			return STEREO;
		case OutputSettings::StereoMode::JointStereo:
			return JOINT_STEREO;
		case OutputSettings::StereoMode::Mono:
			return MONO;
		default:
			return NOT_SET;
		}
	}

	lame_global_flags* m_lame;
	std::ofstream m_file;
	std::vector<unsigned char> m_encodeBuffer;
	std::vector<unsigned char> m_flushBuffer;
};
#endif
} // namespace

struct AudioFileWriter::Impl
{
	Impl(std::filesystem::path path, AudioFileFormat format, OutputSettings settings)
	{
		switch(format)
		{
		case AudioFileFormat::WAV:
		case AudioFileFormat::FLAC:
		case AudioFileFormat::OGG:
			m_backend = std::make_unique<SndfileBackend>(path, format, settings);
			break;
#ifdef LMMS_HAVE_MP3LAME
		case AudioFileFormat::MP3:
			// Note: Sndfile supports MP3 in version 1.1.0 and greater but is still missing some features, such as adding
			// comments to the files and an option for joint stereo (which might be removed in the future if there is little
			// need for it.). We also are still using 1.0.29 in our CI builds.
			m_backend = std::make_unique<LameBackend>(path, format, settings);
			break;
#endif
		}
	}

	std::unique_ptr<Backend> m_backend;
};

AudioFileWriter::AudioFileWriter(std::filesystem::path path, AudioFileFormat format, OutputSettings settings)
	: m_impl(std::make_unique<Impl>(path, format, settings))
	, m_path(path)
{
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
