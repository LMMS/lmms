/*
 * SampleCache.h
 *
 * Copyright (c) 2024 saker
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

#ifndef LMMS_SAMPLE_CACHE_H
#define LMMS_SAMPLE_CACHE_H

#include <QString>
#include <filesystem>
#include <memory>
#include <unordered_map>

#include "SampleBuffer.h"

namespace lmms {
class SampleCache
{
public:
	/**
		Fetches a sample from the cache through a path to an audio file,
		and returns the stored buffer.

		If `path` exists in the cache, its last write time is checked with what is currently in the cache. If
		there is a mismatch, the sample is reloaded from disk, its entry in the cache is updated, and the sample is
		returned.

		If `path` does not exist in the cache, the sample is loaded from disk and
		then returned.
	 */
	static auto fetch(const QString& path) -> std::shared_ptr<SampleBuffer>;

	/**
		Fetches a sample from the cache through a Base64 string and a sample rate
		and returns the stored buffer.

		If an entry for a `base64` string with a certain `sampleRate` exists in the cache, the stored sample is
		returned. Otherwise, if it does not exist in the cache, the sample is loaded and then returned.
	 */
	static auto fetch(const QString& base64, int sampleRate) -> std::shared_ptr<SampleBuffer>;

private:
	struct AudioFileEntry
	{
		friend bool operator==(const AudioFileEntry& first, const AudioFileEntry& second) noexcept
		{
			return first.path == second.path && first.lastWriteTime == second.lastWriteTime;
		}

		std::filesystem::path path;
		std::filesystem::file_time_type lastWriteTime;
	};

	struct Base64Entry
	{
		friend bool operator==(const Base64Entry& first, const Base64Entry& second) noexcept
		{
			return first.base64 == second.base64 && first.sampleRate == second.sampleRate;
		}

		std::string base64;
		int sampleRate;
	};

	struct Hash
	{
		std::size_t operator()(const AudioFileEntry& entry) const noexcept
		{
			return std::filesystem::hash_value(entry.path);
		}

		std::size_t operator()(const Base64Entry& entry) const noexcept
		{
			return std::hash<std::string>()(entry.base64);
		}
	};

	inline static std::unordered_map<AudioFileEntry, std::weak_ptr<SampleBuffer>, Hash> s_audioFileMap;
	inline static std::unordered_map<Base64Entry, std::weak_ptr<SampleBuffer>, Hash> s_base64Map;
};
} // namespace lmms

#endif // LMMS_SAMPLE_CACHE_H
