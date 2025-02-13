/*
 * SampleFile.cpp - abstraction for audio files on the filesystem
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

#include "SampleFile.h"

#include <QString>
#include <algorithm>
#include <sndfile.h>

#include "AudioEngine.h"
#include "DrumSynth.h"
#include "Engine.h"
#include "PathUtil.h"
#include "SampleFrame.h"

namespace lmms {
SampleFile::SampleFile(const std::filesystem::path& path, Mode mode)
	: m_path(path)
{
	auto codecs = std::array<std::unique_ptr<Codec>, 2>{
		std::make_unique<LibSndFileCodec>(), std::make_unique<DrumSynthCodec>()};

	for (auto& codec : codecs)
	{
		if (codec->open(path, mode))
		{
			m_codec = std::move(codec);
			return;
		}
	}

	throw std::runtime_error{"No codec could process the audio file given"};
}

SampleFile::SampleFile(SampleFile&& other) noexcept
	: m_path(std::move(other.m_path))
	, m_codec(std::move(other.m_codec))
{
}

SampleFile& SampleFile::operator=(SampleFile&& other) noexcept
{
	m_path = std::move(other.m_path);
	m_codec = std::move(other.m_codec);
	return *this;
}

auto SampleFile::supportedTypes() -> std::vector<Type>
{
	static const auto s_audioTypes = [] {
		auto types = std::vector<Type>();

		const auto libSndFileTypes = LibSndFileCodec{}.supportedTypes();
		const auto drumSynthTypes = DrumSynthCodec{}.supportedTypes();

		types.insert(types.end(), libSndFileTypes.begin(), libSndFileTypes.end());
		types.insert(types.end(), drumSynthTypes.begin(), drumSynthTypes.end());

		std::sort(types.begin(), types.end(), [&](const Type& a, const Type& b) { return a.name < b.name; });
		return types;
	}();
	return s_audioTypes;
}

auto SampleFile::LibSndFileCodec::open(const std::filesystem::path& path, SampleFile::Mode mode) -> bool
{
#ifdef _WIN32
	m_sndfile = sf_wchar_open(path.c_str(), LibSndFileCodec::sndfileMode(mode), &m_sfInfo);
#else
	m_sndfile = sf_open(path.c_str(), LibSndFileCodec::sndfileMode(mode), &m_sfInfo);
#endif
	return m_sndfile != nullptr;
}

void SampleFile::LibSndFileCodec::close()
{
	if (m_sndfile == nullptr) { return; }
	sf_close(m_sndfile);
}

auto SampleFile::LibSndFileCodec::read(SampleFrame* dst, std::size_t size) -> std::size_t
{
	if (m_sndfile == nullptr) { return 0; }
	if (m_sfInfo.channels == DEFAULT_CHANNELS) { return sf_readf_float(m_sndfile, dst->data(), size); }

	auto tmp = std::vector<float>(size * m_sfInfo.channels);
	sf_readf_float(m_sndfile, tmp.data(), size);

	for (auto i = std::size_t{0}; i < size; ++i)
	{
		if (m_sfInfo.channels == 1)
		{
			dst[i][0] = tmp[i];
			dst[i][1] = tmp[i];
		}
		else
		{
			dst[i][0] = tmp[i * m_sfInfo.channels];
			dst[i][1] = tmp[i * m_sfInfo.channels + 1];
		}
	}

	return tmp.size();
}

auto SampleFile::LibSndFileCodec::write(const SampleFrame* src, std::size_t size) -> std::size_t
{
	if (m_sndfile == nullptr) { return 0; }
	if (m_sfInfo.channels == DEFAULT_CHANNELS) { return sf_writef_float(m_sndfile, src->data(), size); }

	auto tmp = std::vector<float>(size * m_sfInfo.channels);
	for (auto i = std::size_t{0}; i < size; ++i)
	{
		if (m_sfInfo.channels == 1)
		{
			tmp[i] = src[i].average();
		}
		else
		{
			tmp[i * m_sfInfo.channels] = src[i][0];
			tmp[i * m_sfInfo.channels + 1] = src[i][1];
		}
	}

	sf_writef_float(m_sndfile, tmp.data(), tmp.size());
	return tmp.size();
}

auto SampleFile::LibSndFileCodec::seek(std::size_t offset, int whence) -> std::size_t
{
	return sf_seek(m_sndfile, offset, whence);
}

auto SampleFile::LibSndFileCodec::sndfileMode(SampleFile::Mode mode) -> int
{
	switch (mode)
	{
	case SampleFile::Mode::Read:
		return SFM_READ;
	case SampleFile::Mode::Write:
		return SFM_WRITE;
	case SampleFile::Mode::ReadAndWrite:
		return SFM_RDWR;
	default:
		return -1;
	}
}

auto SampleFile::LibSndFileCodec::supportedTypes() -> std::vector<SampleFile::Type>
{
	static const auto s_audioTypes = [] {
		auto types = std::vector<Type>();
		auto sfFormatInfo = SF_FORMAT_INFO{};
		auto simpleTypeCount = 0;
		sf_command(nullptr, SFC_GET_SIMPLE_FORMAT_COUNT, &simpleTypeCount, sizeof(int));

		for (int simple = 0; simple < simpleTypeCount; ++simple)
		{
			sfFormatInfo.format = simple;
			sf_command(nullptr, SFC_GET_SIMPLE_FORMAT, &sfFormatInfo, sizeof(sfFormatInfo));

			auto it = std::find_if(
				types.begin(), types.end(), [&](const Type& type) { return sfFormatInfo.extension == type.extension; });
			if (it != types.end()) { continue; }

			auto name = std::string{sfFormatInfo.extension};
			std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return std::toupper(ch); });

			types.push_back(Type{std::move(name), sfFormatInfo.extension});
		}

		std::sort(types.begin(), types.end(), [&](const Type& a, const Type& b) { return a.name < b.name; });
		return types;
	}();
	return s_audioTypes;
}

auto SampleFile::DrumSynthCodec::open(const std::filesystem::path& path, SampleFile::Mode mode) -> bool
{
	const auto qPath = PathUtil::qStringFromPath(path);

	const auto sampleRate = Engine::audioEngine()->outputSampleRate();
	auto ptr = m_ptr.get();
	auto error = DrumSynth{}.GetDSFileSamples(qPath, ptr, DEFAULT_CHANNELS, sampleRate);

	if (error != 0) { m_ptr.reset(ptr); }
	return error != 0;
}

auto SampleFile::DrumSynthCodec::read(SampleFrame* dst, std::size_t size) -> std::size_t
{
	for (auto i = std::size_t{0}; i < size; ++i)
	{
		const auto left = m_ptr.get()[i * DEFAULT_CHANNELS];
		const auto right = m_ptr.get()[i * DEFAULT_CHANNELS + 1];

		const auto normalizedLeft = left / OUTPUT_SAMPLE_MULTIPLIER;
		const auto normalizedRight = right / OUTPUT_SAMPLE_MULTIPLIER;

		dst[i] = SampleFrame{normalizedLeft, normalizedRight};
	}

	m_pos += size * DEFAULT_CHANNELS;
	return m_pos;
}

auto SampleFile::DrumSynthCodec::write(const SampleFrame* src, std::size_t size) -> std::size_t
{
	for (auto i = std::size_t{0}; i < size; ++i)
	{
		m_ptr.get()[i * DEFAULT_CHANNELS] = src[i][0];
		m_ptr.get()[i * DEFAULT_CHANNELS + 1] = src[i][1];
	}

	m_pos += size * DEFAULT_CHANNELS;
	return m_pos;
}

auto SampleFile::DrumSynthCodec::seek(std::size_t offset, int whence) -> std::size_t
{
	static constexpr auto seekSet = 0;
	static constexpr auto seekCur = 1;
	static constexpr auto seekEnd = 2;

	switch (whence)
	{
	case seekSet:
		m_pos = offset;
		break;
	case seekCur:
		m_pos += offset;
		break;
	case seekEnd:
		m_pos += m_size + offset;
		break;
	}

	return m_pos;
}

} // namespace lmms