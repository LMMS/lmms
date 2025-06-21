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

#include <fstream>
#include <utility>

#include "AudioEngine.h"
#include "DrumSynth.h"
#include "Engine.h"
#include "PathUtil.h"

namespace lmms {
AudioFile::AudioFile(const std::filesystem::path& path, Mode mode)
{
	if (DrumSynthFile::isDrumSynthPreset(path))
	{
		if (mode != Mode::Read) { throw std::runtime_error{"Can only open DrumSynth files in read mode"}; }

		m_info.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16;
		m_info.channels = 2;
		m_info.samplerate = Engine::audioEngine()->outputSampleRate();

		m_virtualIo.get_filelen = DrumSynthFile::staticFilelen;
		m_virtualIo.seek = DrumSynthFile::staticSeek;
		m_virtualIo.tell = DrumSynthFile::staticTell;
		m_virtualIo.read = DrumSynthFile::staticRead;
		m_virtualIo.write = DrumSynthFile::staticWrite;

		m_drumSynthFile = DrumSynthFile{path};
		m_sndfile = sf_open_virtual(&m_virtualIo, AudioFile::mode(mode), &m_info, &m_drumSynthFile);
	}
	else
	{
#ifdef LMMS_BUILD_WIN32
		m_sndfile = sf_wchar_open(path.c_str(), AudioFile::mode(mode), &m_info);
#else
		m_sndfile = sf_open(path.c_str(), AudioFile::mode(mode), &m_info);
#endif
	}

	if (!m_sndfile) { throw std::runtime_error{"Failed to load audio file: " + std::string{sf_strerror(nullptr)}}; }
}

AudioFile::~AudioFile() noexcept
{
	sf_close(m_sndfile);
}

AudioFile::AudioFile(AudioFile&& other) noexcept
	: m_sndfile(std::exchange(other.m_sndfile, nullptr))
	, m_info(other.m_info)
{
}

AudioFile& AudioFile::operator=(AudioFile&& other) noexcept
{
	m_sndfile = std::exchange(other.m_sndfile, nullptr);
	m_info = other.m_info;
	return *this;
}

auto AudioFile::write(InterleavedBufferView<float> src) -> f_cnt_t
{
	if (src.channels() != m_info.channels) { throw std::invalid_argument{"Invalid channel count"}; }
	return sf_writef_float(m_sndfile, src.data(), src.frames());
}

auto AudioFile::read(InterleavedBufferView<float> dst) -> f_cnt_t
{
	if (dst.channels() != m_info.channels) { throw std::invalid_argument{"Invalid channel count"}; }
	return sf_readf_float(m_sndfile, dst.data(), dst.frames());
}

auto AudioFile::seek(f_cnt_t frames, Whence whence) -> f_cnt_t
{
	return sf_seek(m_sndfile, frames, AudioFile::whence(whence));
}

AudioFile::DrumSynthFile::DrumSynthFile(const std::filesystem::path& path)
	: m_size(
		  DrumSynth{}.GetDSFileSamples(PathUtil::fsConvert(path), m_data, 2, Engine::audioEngine()->outputSampleRate())
		  * 2 * sizeof(int_sample_t))
{
	if (m_size == 0 || m_data == nullptr) { throw std::runtime_error{"Failed to load DrumSynth file"}; }
}

AudioFile::DrumSynthFile::DrumSynthFile(DrumSynthFile&& other) noexcept
	: m_data(std::exchange(other.m_data, nullptr))
	, m_size(std::exchange(other.m_size, 0))
	, m_offset(std::exchange(other.m_offset, 0))
{
}

AudioFile::DrumSynthFile& AudioFile::DrumSynthFile::operator=(DrumSynthFile&& other) noexcept
{
	m_data = std::exchange(other.m_data, nullptr);
	m_size = std::exchange(other.m_size, 0);
	m_offset = std::exchange(other.m_offset, 0);
	return *this;
}

AudioFile::DrumSynthFile::~DrumSynthFile() noexcept
{
	delete[] m_data;
}

auto AudioFile::DrumSynthFile::filelen() const -> sf_count_t
{
	return m_size * sizeof(int_sample_t);
}

auto AudioFile::DrumSynthFile::tell() const -> sf_count_t
{
	return m_offset;
}

auto AudioFile::DrumSynthFile::seek(sf_count_t offset, int whence) -> sf_count_t
{
	switch (whence)
	{
	case SEEK_CUR:
		m_offset += offset;
		break;
	case SEEK_SET:
		m_offset = offset;
		break;
	case SEEK_END:
		m_offset = filelen() + offset;
		break;
	}

	return m_offset;
}

auto AudioFile::DrumSynthFile::read(void* ptr, sf_count_t count) -> sf_count_t
{
	const auto amount = std::min<sf_count_t>(count, m_size - m_offset);
	std::memcpy(ptr, &m_data[m_offset / sizeof(int_sample_t)], amount);
	m_offset += amount;
	return amount;
}

auto AudioFile::DrumSynthFile::write(const void* ptr, sf_count_t count) -> sf_count_t
{
	// Cannot write to DrumSynth files
	return 0;
}

auto AudioFile::DrumSynthFile::staticFilelen(void* userData) -> sf_count_t
{
	return static_cast<DrumSynthFile*>(userData)->filelen();
}

auto AudioFile::DrumSynthFile::staticRead(void* ptr, sf_count_t count, void* userData) -> sf_count_t
{
	return static_cast<DrumSynthFile*>(userData)->read(ptr, count);
}

auto AudioFile::DrumSynthFile::staticWrite(const void* ptr, sf_count_t count, void* userData) -> sf_count_t
{
	return static_cast<DrumSynthFile*>(userData)->write(ptr, count);
}

auto AudioFile::DrumSynthFile::staticSeek(sf_count_t offset, int whence, void* userData) -> sf_count_t
{
	return static_cast<DrumSynthFile*>(userData)->seek(offset, whence);
}

auto AudioFile::DrumSynthFile::staticTell(void* userData) -> sf_count_t
{
	return static_cast<DrumSynthFile*>(userData)->tell();
}

auto AudioFile::DrumSynthFile::isDrumSynthPreset(const std::filesystem::path& path) -> bool
{
	auto file = std::ifstream{path};
	if (!file.is_open()) { return false; }

	auto generalSection = std::string{};
	auto drumSynthVersion = std::string{};

	std::getline(file, generalSection);
	std::getline(file, drumSynthVersion);

	return generalSection.find("[General]") == 0 && drumSynthVersion.find("Version=DrumSynth") == 0;
}

} // namespace lmms
