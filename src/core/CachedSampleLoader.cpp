/*
 * CachedSampleLoader.cpp - Sample loader with a cache
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

#include "CachedSampleLoader.h"

#include <cassert>

#include "PathUtil.h"
#include "lmms_basics.h"

namespace lmms {

/**
 * An auto evictor which automatically evicts sample buffers from the cache
 * immediately after the last reference to the buffer is dropped
*/
class CachedSampleLoader::AutoEvictor
{
public:
	void operator()(SampleBuffer* p) const noexcept
	{
		if (!p) { return; }

		CachedSampleLoader::inst().remove(*p);
		delete p;
	}
};

CachedSampleLoader::CachedSampleLoader()
{
	connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &CachedSampleLoader::removeFile);
}

auto CachedSampleLoader::inst() -> CachedSampleLoader&
{
	static auto singleton = std::unique_ptr<CachedSampleLoader>(new CachedSampleLoader{});
	return *singleton;
}

auto CachedSampleLoader::createBufferFromFile(const QString& filePath)
	-> std::shared_ptr<const SampleBuffer>
{
	if (filePath.isEmpty()) { return nullptr; }

	if (auto buffer = CachedSampleLoader::inst().get(filePath, SampleBuffer::Source::AudioFile))
	{
		return buffer;
	}

	try
	{
		auto buffer = std::shared_ptr<SampleBuffer> {
			new SampleBuffer{SampleBuffer::Access{}, filePath},
			AutoEvictor{}
		};
		CachedSampleLoader::inst().add(buffer);
		return buffer;
	}
	catch (const std::runtime_error& error)
	{
		displayError(QString::fromStdString(error.what()));
		return nullptr;
	}
}

auto CachedSampleLoader::createBufferFromBase64(const QString& base64, int sampleRate)
	-> std::shared_ptr<const SampleBuffer>
{
	if (base64.isEmpty()) { return nullptr; }

	if (auto buffer = CachedSampleLoader::inst().get(base64, SampleBuffer::Source::Base64))
	{
		return buffer;
	}

	try
	{
		auto buffer = std::shared_ptr<SampleBuffer> {
			new SampleBuffer{SampleBuffer::Access{}, base64, sampleRate},
			AutoEvictor{}
		};
		CachedSampleLoader::inst().add(buffer);
		return buffer;
	}
	catch (const std::runtime_error& error)
	{
		displayError(QString::fromStdString(error.what()));
		return nullptr;
	}
}

void CachedSampleLoader::add(const std::shared_ptr<const SampleBuffer>& buffer)
{
	assert(buffer != nullptr);
	if (buffer->sourceType() == SampleBuffer::Source::Unknown)
	{
		throw std::runtime_error{"Unsupported sample buffer source"};
	}

	const bool added = m_entries.try_emplace(std::pair{buffer->source(), buffer->sourceType()}, buffer).second;

	if (added && buffer->sourceType() == SampleBuffer::Source::AudioFile)
	{
		m_watcher.addPath(buffer->audioFileAbsolute());
	}
}

auto CachedSampleLoader::remove(const SampleBuffer& buffer) -> bool
{
	const bool removed = m_entries.erase(std::pair{buffer.source(), buffer.sourceType()}) > 0;

	if (removed && buffer.sourceType() == SampleBuffer::Source::AudioFile)
	{
		m_watcher.removePath(buffer.audioFileAbsolute());
	}

	return removed;
}

auto CachedSampleLoader::get(const QString& source, SampleBuffer::Source sourceType)
	-> std::shared_ptr<const SampleBuffer>
{
	switch (sourceType)
	{
		case SampleBuffer::Source::AudioFile:
		{
			if (auto it = m_entries.find(std::pair{PathUtil::toAbsolute(source), sourceType}); it != m_entries.end())
			{
				return it->second.lock();
			}
			break;
		}
		case SampleBuffer::Source::Base64:
		{
			if (auto it = m_entries.find(std::pair{source, sourceType}); it != m_entries.end())
			{
				return it->second.lock();
			}
			break;
		}
		default:
			throw std::runtime_error{"Unsupported sample buffer source"};
	}

	return nullptr;
}

void CachedSampleLoader::removeFile(const QString& path)
{
	m_entries.erase(std::pair{path, SampleBuffer::Source::AudioFile});
}

} // namespace lmms
