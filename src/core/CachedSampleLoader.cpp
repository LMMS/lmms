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

#include <QDebug>
#include <QTimer>
#include <QObject>
#include <cassert>
#include <unordered_map>

#include "PathUtil.h"
#include "lmms_basics.h"

namespace lmms {

/**
 * An auto evictor which automatically evicts sample buffers from the cache
 * a specified time after the last reference to the buffer is dropped
*/
class CachedSampleLoader::AutoEvicter
{
public:
	AutoEvicter(std::chrono::milliseconds keepAlive = std::chrono::seconds{60})
		: m_keepAlive{keepAlive}
	{
	}

	void operator()(SampleBuffer* p) const noexcept
	{
		if (!p) { return; }

		qDebug() << "AutoEvicter - deleter";

		if (m_postponedEviction)
		{
			qDebug() << "AutoEvicter - postponed eviction period has passed, time to delete";
			//CachedSampleLoader::inst().remove(*p);
			delete p;
			qDebug() << "AutoEvicter - done";
			return;
		}

		if (m_keepAlive.count() == 0)
		{
			// Evict from cache immediately
			qDebug() << "AutoEvicter - evict immediately";
			CachedSampleLoader::inst().remove(*p);
			delete p;
			qDebug() << "AutoEvicter - done";
			return;
		}

		qDebug() << "AutoEvicter - postpone evict";

		auto buffer = std::shared_ptr<const SampleBuffer>{p, AutoEvicter{true, m_keepAlive}};

		CachedSampleLoader::inst().replace(buffer);

		// Postpone buffer deletion TODO: Will this work?
		QTimer::singleShot(m_keepAlive,
			new detail::CacheKeepAlive{std::move(buffer)},
			&detail::CacheKeepAlive::destroy);

		qDebug() << "AutoEvicter - deleter return";
	}

	//! Keep alive duration
	auto keepAlive() -> auto& { return m_keepAlive; }

	void resetPostponedEviction() { m_postponedEviction = false; qDebug() << "resetPostponedEviction()"; }

private:
	AutoEvicter(bool postponedEviction, std::chrono::milliseconds keepAlive = std::chrono::seconds{60})
		: m_keepAlive{keepAlive}
		, m_postponedEviction{postponedEviction}
	{
	}

	std::chrono::milliseconds m_keepAlive;
	bool m_postponedEviction = false;
};

detail::CacheKeepAlive::CacheKeepAlive(std::shared_ptr<const SampleBuffer>&& p) noexcept
	: QObject{}
	, m_data{std::move(p)}
{
}

void detail::CacheKeepAlive::destroy() noexcept
{
	qDebug() << "CacheKeepAlive - destroy; use count before:" << m_data.use_count();
	m_data.reset();
	qDebug() << "CacheKeepAlive - use count should be 0:" << m_data.use_count();
	delete this; // ???
}

CachedSampleLoader::CachedSampleLoader()
{
	connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &CachedSampleLoader::removeFile);
}

auto CachedSampleLoader::inst() -> CachedSampleLoader&
{
	static auto singleton = std::unique_ptr<CachedSampleLoader>(new CachedSampleLoader{});
	return *singleton;
}

auto CachedSampleLoader::createBufferFromFile(const QString& filePath,
	std::chrono::seconds keepAlive) -> std::shared_ptr<const SampleBuffer>
{
	if (filePath.isEmpty()) { return nullptr; }

	qDebug() << "CachedSampleLoader::createBufferFromFile";

	if (auto buffer = CachedSampleLoader::inst().get(filePath, SampleBuffer::Source::AudioFile))
	{
		//std::get_deleter<AutoEvicter>(buffer)->keepAlive() = keepAlive; // TODO?
		return buffer;
	}

	qDebug() << "CachedSampleLoader: Cache returned null";

	try
	{
		auto buffer = std::shared_ptr<SampleBuffer> {
			new SampleBuffer{SampleBuffer::Access{}, filePath},
			AutoEvicter{ std::chrono::seconds{5} } // TODO
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

auto CachedSampleLoader::createBufferFromBase64(const QString& base64,
	int sampleRate, std::chrono::seconds keepAlive) -> std::shared_ptr<const SampleBuffer>
{
	if (base64.isEmpty()) { return nullptr; }

	qDebug() << "CachedSampleLoader::createBufferFromBase64";

	if (auto buffer = CachedSampleLoader::inst().get(base64, SampleBuffer::Source::Base64))
	{
		//qDebug() << "CachedSampleLoader: AutoEvictor keepAlive ms:" << std::get_deleter<AutoEvicter>(buffer)->keepAlive().count();
		//std::get_deleter<AutoEvicter>(buffer)->keepAlive() = keepAlive; // TODO?
		return buffer;
	}

	qDebug() << "CachedSampleLoader: Cache returned null";

	try
	{
		auto buffer = std::shared_ptr<SampleBuffer> {
			new SampleBuffer{SampleBuffer::Access{}, base64, sampleRate},
			AutoEvicter{keepAlive}
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

	qDebug() << QString{"add: Emplacing {%1, %2}"}.arg(buffer->source(), QString::number((int)buffer->sourceType()));

	const bool added = m_entries.try_emplace(std::pair{buffer->source(), buffer->sourceType()}, buffer).second;

	if (added && buffer->sourceType() == SampleBuffer::Source::AudioFile)
	{
		m_watcher.addPath(buffer->audioFileAbsolute());
	}
}

auto CachedSampleLoader::remove(const SampleBuffer& buffer) -> bool
{
	qDebug() << "CachedSampleLoader::remove";
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
	std::shared_ptr<const SampleBuffer> buffer;
	switch (sourceType)
	{
		case SampleBuffer::Source::AudioFile:
		{
			qDebug() << QString{"get: Finding {%1, %2}"}.arg(PathUtil::toAbsolute(source), QString::number((int)sourceType));
			if (auto it = m_entries.find(std::pair{PathUtil::toAbsolute(source), sourceType}); it != m_entries.end())
			{
				qDebug() << "Found in cache:" << PathUtil::toAbsolute(source);
				buffer = it->second.lock();
			}
			break;
		}
		case SampleBuffer::Source::Base64:
		{
			qDebug() << QString{"get: Finding {%1, %2}"}.arg(source, QString::number((int)sourceType));
			if (auto it = m_entries.find(std::pair{source, sourceType}); it != m_entries.end())
			{
				qDebug() << "Found in cache:" << source;
				buffer = it->second.lock();
			}
			break;
		}
		default:
			throw std::runtime_error{"Unsupported sample buffer source"};
	}

	if (!buffer)
	{
		qDebug() << "Not found in cache";
		return nullptr;
	}

	std::get_deleter<AutoEvicter>(buffer)->resetPostponedEviction();
	return buffer;
}

auto CachedSampleLoader::replace(const std::shared_ptr<const SampleBuffer>& buffer) -> bool
{
	// NOTE: This method cannot take `const SampleBuffer&` and use `buffer.get()` because
	// using `shared_from_this()` from the shared pointer deleter is UB.

	assert(buffer != nullptr);
	if (buffer->sourceType() == SampleBuffer::Source::Unknown)
	{
		throw std::runtime_error{"Unsupported sample buffer source"};
	}

	if (auto it = m_entries.find(std::pair{buffer->source(), buffer->sourceType()}); it != m_entries.end())
	{
		qDebug() << "CachedSampleLoader::replace buffer.get():" << (buffer.get() ? "non-null" : "null");
		it->second = buffer;
		qDebug() << "CachedSampleLoader::replace success";
		return true;
	}

	qDebug() << "CachedSampleLoader::replace FAIL";
	return false;
}

void CachedSampleLoader::removeFile(const QString& path)
{
	m_entries.erase(std::pair{path, SampleBuffer::Source::AudioFile});
}

} // namespace lmms
