/*
 * AudioFile.h
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

#include <cstring>
#include <filesystem>
#include <sndfile.h>

#include "AudioBufferView.h"
#include "LmmsTypes.h"

namespace lmms {
//! @brief The AudioFile is a class that is abstraction reading from audio files on disk. DrumSynth files are also
//! supported. Only interleaved floating point audio data can be written to and read from the AudioFile object.
class AudioFile
{
public:
	//! Used when specifying the file offset for the @ref seek function.
	enum class Whence
	{
		Set,	 //!< The offset is set to the given value.
		Current, //!< The offset is set to its current value plus the given value.
		End		 //!< The offset is set to the end of the data plus the given value
	};

	//! The mode to open the file in.
	enum class Mode
	{
		Read,	  //!< Open the audio file for reading
		Write,	  //!< Open the audio file for writing
		ReadWrite //!< Open the audio file for both reading and writing
	};

	//! Cannot copy audio file objects.
	AudioFile(const AudioFile&) = delete;

	//! Move audio file object and its handle.
	AudioFile(AudioFile&&) noexcept;

	//! Cannot copy audio file objects.
	AudioFile& operator=(const AudioFile&) = delete;

	//! Move audio file object and its handle.
	AudioFile& operator=(AudioFile&&) noexcept;

	//! Load an audio file at the given @p path with the given @p mode .
	AudioFile(const std::filesystem::path& path, Mode mode);

	//! Closes the audio file.
	~AudioFile() noexcept;

	//! Write audio data to the audio file from @p src .
	//! @return The number of audio frames written.
	[[nodiscard]] auto write(InterleavedBufferView<float> src) -> f_cnt_t;

	//! Read audio data from the audio file into @p dst .
	//! @return The number of audio frames read.
	[[nodiscard]] auto read(InterleavedBufferView<float> dst) -> f_cnt_t;

	//! Change the file offset to start reading and writing at a different location in the audio file.
	//! @return The file offset or -1 if an error occurred.
	[[nodiscard]] auto seek(f_cnt_t frames, Whence whence) -> f_cnt_t;

	//! @return The sample rate of the audio file.
	auto sampleRate() const -> sample_rate_t { return m_info.samplerate; }

	//! @return The number of channels.
	auto channels() const -> ch_cnt_t { return m_info.channels; }

	//! @return The number of audio frames the file holds.
	auto frames() const -> f_cnt_t { return m_info.frames; }

private:
	class DrumSynthFile
	{
	public:
		DrumSynthFile() = default;
		explicit DrumSynthFile(const std::filesystem::path& path);

		DrumSynthFile(const DrumSynthFile&) = delete;
		DrumSynthFile& operator=(const DrumSynthFile&) = delete;

		DrumSynthFile(DrumSynthFile&& other) noexcept;
		DrumSynthFile& operator=(DrumSynthFile&& other) noexcept;

		~DrumSynthFile() noexcept;

		auto filelen() const -> sf_count_t;
		auto tell() const -> sf_count_t;

		auto seek(sf_count_t offset, int whence) -> sf_count_t;
		auto read(void* ptr, sf_count_t count) -> sf_count_t;
		auto write(const void* ptr, sf_count_t count) -> sf_count_t;

		static auto staticFilelen(void* userData) -> sf_count_t;
		static auto staticRead(void* ptr, sf_count_t count, void* userData) -> sf_count_t;
		static auto staticWrite(const void* ptr, sf_count_t count, void* userData) -> sf_count_t;
		static auto staticSeek(sf_count_t offset, int whence, void* userData) -> sf_count_t;
		static auto staticTell(void* userData) -> sf_count_t;

		static auto isDrumSynthPreset(const std::filesystem::path& path) -> bool;

	private:
		int_sample_t* m_data{};
		std::size_t m_size = 0;
		sf_count_t m_offset = 0;
	};

	static constexpr auto mode(Mode mode) -> int
	{
		switch (mode)
		{
		case Mode::Read:
			return SFM_READ;
		case Mode::Write:
			return SFM_WRITE;
		case Mode::ReadWrite:
			return SFM_RDWR;
		default:
			throw std::invalid_argument{"Invalid mode"};
		}
	}

	static constexpr auto whence(Whence whence) -> int
	{
		switch (whence)
		{
		case Whence::Set:
			return SF_SEEK_SET;
		case Whence::Current:
			return SF_SEEK_CUR;
		case Whence::End:
			return SF_SEEK_END;
		default:
			throw std::invalid_argument{"Invalid whence"};
		}
	}

	SNDFILE* m_sndfile = nullptr;
	SF_INFO m_info{};
	SF_VIRTUAL_IO m_virtualIo{};
	DrumSynthFile m_drumSynthFile;
};
} // namespace lmms

#endif // LMMS_AUDIO_FILE_H