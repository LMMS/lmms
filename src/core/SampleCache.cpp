/*
 * SampleCache.cpp - SampleBuffer cache
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "SampleCache.h"

#include <QTimer>
#include <QObject>
#include <unordered_map>

#include "lmms_basics.h"

namespace lmms {

std::map<std::pair<QString, SampleBuffer::Source>, std::weak_ptr<const SampleBuffer>> SampleCache::s_entries;
QFileSystemWatcher SampleCache::s_watcher;

/**
 * An auto evictor which automatically evicts sample buffers from the cache
 * a specified time after the last reference to the buffer is dropped
*/
class AutoEvicter
{
public:
	AutoEvicter(std::chrono::seconds keepAlive = std::chrono::seconds{60})
		: m_keepAliveDuration{keepAlive}
	{
	}

	void operator()(SampleBuffer* p) const
	{
		if (!p) { return; }

		if (m_keepAliveDuration.count() == 0)
		{
			// Evict from cache immediately
			SampleCache::remove(*p);
			delete p;
			return;
		}

		auto buffer = std::shared_ptr<const SampleBuffer>{p};
		SampleCache::replace(*buffer);

		// Postpone buffer deletion TODO: Will this work?
		QTimer::singleShot(m_keepAliveDuration,
			new KeepAlive{Engine::inst()->audioEngine(), std::move(buffer)},
			&KeepAlive::destroy);
	}

private:
	std::chrono::milliseconds m_keepAliveDuration;
};

SampleCache::SampleCache()
{
	connect(&s_watcher, &QFileSystemWatcher::fileChanged, this, &SampleCache::removeFile);
}

auto SampleCache::createBufferFromFile(const QString& filePath,
	std::chrono::seconds keepAlive) -> std::shared_ptr<const SampleBuffer>
{
	return std::shared_ptr<SampleBuffer> {
		new SampleBuffer{SampleBuffer::Access{}, filePath},
		AutoEvicter{keepAlive}
	};
}

auto SampleCache::createBufferFromBase64(const QString& base64,
	int sampleRate, std::chrono::seconds keepAlive) -> std::shared_ptr<const SampleBuffer>
{
	return std::shared_ptr<SampleBuffer> {
		new SampleBuffer{SampleBuffer::Access{}, base64, sampleRate},
		AutoEvicter{keepAlive}
	};
}

void SampleCache::add(const SampleBuffer& buffer)
{
	if (buffer.sourceType() == SampleBuffer::Source::Unknown)
	{
		throw std::runtime_error{"Unsupported sample buffer source"};
	}

	const bool added = s_entries.try_emplace(std::pair{buffer.source(), buffer.sourceType()}, buffer.get()).second;

	if (added && buffer.sourceType() == SampleBuffer::Source::AudioFile)
	{
		s_watcher.addPath(buffer.audioFile());
	}
}

auto SampleCache::remove(const SampleBuffer& buffer) -> bool
{
	const bool removed = s_entries.erase(std::pair{buffer.source(), buffer.sourceType()}) > 0;

	if (removed && buffer.sourceType() == SampleBuffer::Source::AudioFile)
	{
		s_watcher.removePath(buffer.audioFile());
	}

	return removed;
}

auto SampleCache::get(const QString& source, SampleBuffer::Source sourceType)
	-> std::shared_ptr<const SampleBuffer>
{
	if (sourceType == SampleBuffer::Source::Unknown)
	{
		throw std::runtime_error{"Unsupported sample buffer source"};
	}

	if (auto it = s_entries.find(std::pair{source, sourceType}); it != s_entries.end())
	{
		return it->second.lock();
	}

	return nullptr;
}

auto SampleCache::replace(const SampleBuffer& buffer) -> bool
{
	if (buffer.sourceType() == SampleBuffer::Source::Unknown)
	{
		throw std::runtime_error{"Unsupported sample buffer source"};
	}

	if (auto it = s_entries.find(std::pair{buffer.source(), buffer.sourceType()}); it != s_entries.end())
	{
		it->second = buffer.get();
		return true;
	}

	return false;
}

void SampleCache::removeFile(const QString& path)
{
	s_entries.erase(std::pair{path, SampleBuffer::Source::AudioFile});
}

} // namespace lmms
