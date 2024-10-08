/*
 * SampleDatabase.h
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

#ifndef LMMS_SAMPLE_DATABASE_H
#define LMMS_SAMPLE_DATABASE_H

#include <QString>
#include <filesystem>
#include <memory>
#include <unordered_map>

#include "SampleBuffer.h"

namespace lmms {
class SampleDatabase
{
public:
	/**
		Fetches a sample from the database through a path to an audio file,
		and returns the stored buffer.

		If `path` exists in the database, its last write time is checked with what is currently in the database. If
		there is a mismatch, the sample is reloaded from disk, its entry in the database is updated, and the sample is
		returned.

		If `path` does not exist in the database, the sample is loaded from disk and
		then returned.
	 */
	static auto fetch(const QString& path) -> std::shared_ptr<SampleBuffer>;

	/**
		Fetches a sample from the database through a Base64 string and a sample rate
		and returns the stored buffer.

		If an entry for a `base64` string with a certain `sampleRate` exists in the database, the stored sample is
		returned. Otherwise, if it does not exist in the database, the sample is loaded and then returned.
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

	template <typename T, typename ...Args>
	static auto get(const T& entry, std::unordered_map<T, std::weak_ptr<SampleBuffer>, Hash>& map, Args... args)
	{
		const auto it = map.find(entry);

		if (it == map.end())
		{
			const auto buffer = std::make_shared<SampleBuffer>(std::forward<Args>(args)...);
			map.insert(std::make_pair(entry, buffer));
			return buffer;
		}

		const auto entryLock = it->second.lock();
		if (!entryLock)
		{
			const auto buffer = std::make_shared<SampleBuffer>(std::forward<Args>(args)...);
			map[entry] = buffer;
			return buffer;
		}

		return entryLock;
	}

	inline static std::unordered_map<AudioFileEntry, std::weak_ptr<SampleBuffer>, Hash> s_audioFileMap;
	inline static std::unordered_map<Base64Entry, std::weak_ptr<SampleBuffer>, Hash> s_base64Map;
};
} // namespace lmms

#endif // LMMS_SAMPLE_DATABASE_H
