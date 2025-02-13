/*
 * AudioFile.h - abstraction for audio files on the filesystem
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

#ifndef LMMS_AUDIO_FILE_H
#define LMMS_AUDIO_FILE_H

#include <filesystem>
#include <sndfile.h>
#include <vector>

#include "AudioEngine.h"
#include "Engine.h"
#include "lmms_basics.h"

namespace lmms {

class SampleFrame;

class AudioFile
{
public:
	enum class Mode
	{
		Read,
		Write,
		ReadAndWrite
	};

	struct Type
	{
		std::string name;
		std::string extension;
	};

	AudioFile(const std::filesystem::path& path, Mode mode);

	~AudioFile()
	{
		if (m_codec.get() != nullptr) { m_codec->close(); }
	}

	AudioFile(const AudioFile&) = delete;
	AudioFile& operator=(const AudioFile&) = delete;

	AudioFile(AudioFile&&) noexcept;
	AudioFile& operator=(AudioFile&&) noexcept;

	auto read(SampleFrame* dst, std::size_t size) const -> std::size_t
	{
		return m_codec.get() != nullptr ? m_codec->read(dst, size) : 0;
	}

	auto write(const SampleFrame* src, std::size_t size) -> std::size_t
	{
		return m_codec.get() != nullptr ? m_codec->write(src, size) : 0;
	}

	auto seek(std::size_t offset, int whence) -> std::size_t
	{
		return m_codec.get() != nullptr ? m_codec->seek(offset, whence) : 0;
	}

	auto frames() const -> std::size_t { return m_codec.get() != nullptr ? m_codec->frames() : 0; }

	auto sampleRate() const -> int { return m_codec.get() != nullptr ? m_codec->sampleRate() : 0; }

	auto path() const -> const std::filesystem::path& { return m_path; }
	auto bytes() const -> std::size_t { return std::filesystem::file_size(m_path); }

	static auto supportedTypes() -> std::vector<Type>;

private:
	class Codec
	{
	public:
		virtual ~Codec() = default;

		virtual auto open(const std::filesystem::path& path, AudioFile::Mode mode) -> bool = 0;
		virtual void close() = 0;

		virtual auto read(SampleFrame* dst, std::size_t size) -> std::size_t = 0;
		virtual auto write(const SampleFrame* src, std::size_t size) -> std::size_t = 0;

		virtual auto seek(std::size_t offset, int whence) -> std::size_t = 0;

		virtual auto frames() const -> std::size_t = 0;
		virtual auto sampleRate() const -> int = 0;

		virtual auto supportedTypes() -> std::vector<AudioFile::Type> = 0;
	};

	class LibSndFileCodec : public Codec
	{
	public:
		auto open(const std::filesystem::path& path, AudioFile::Mode mode) -> bool override;
		void close() override;

		auto read(SampleFrame* dst, std::size_t size) -> std::size_t override;
		auto write(const SampleFrame* src, std::size_t size) -> std::size_t override;

		auto seek(std::size_t offset, int whence) -> std::size_t override;

		auto frames() const -> std::size_t override { return m_sfInfo.frames; }
		auto sampleRate() const -> int override { return m_sfInfo.samplerate; }

		auto supportedTypes() -> std::vector<AudioFile::Type> override;

	private:
		static auto sndfileMode(AudioFile::Mode) -> int;
		SNDFILE* m_sndfile = nullptr;
		SF_INFO m_sfInfo;
	};

	class DrumSynthCodec : public Codec
	{
	public:
		auto open(const std::filesystem::path& path, AudioFile::Mode mode) -> bool override;
		void close() override {}

		auto read(SampleFrame* dst, std::size_t size) -> std::size_t override;
		auto write(const SampleFrame* src, std::size_t size) -> std::size_t override;

		auto seek(std::size_t offset, int whence) -> std::size_t override;

		auto frames() const -> std::size_t override { return m_size / DEFAULT_CHANNELS; }
		auto sampleRate() const -> int override { return Engine::audioEngine()->outputSampleRate(); }

		auto supportedTypes() -> std::vector<AudioFile::Type> override
		{
			static auto type = std::vector<Type>{Type{"DrumSynth", "ds"}};
			return type;
		}

	private:
		std::unique_ptr<int16_t[]> m_ptr; // NOLINT
		std::size_t m_pos = 0;
		std::size_t m_size = 0;
	};

	std::filesystem::path m_path;
	std::unique_ptr<Codec> m_codec;
};
} // namespace lmms

#endif // LMMS_AUDIO_FILE_H
