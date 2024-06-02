/*
 * SampleLoader.cpp - Sample loader with support for caching
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
 *               2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "SampleLoader.h"

#include <cassert>

#include "Engine.h"
#include "Song.h"
#include "lmms_basics.h"

namespace lmms {

namespace {

void displayError(const QString& message)
{
	Engine::getSong()->collectError(QString("%1: %2").arg(QObject::tr("Error loading sample"), message));
}

} // namespace

/**
 * An evictor which automatically evicts sample buffers from the cache
 * immediately after the last reference to the buffer is dropped.
 * Used as std::shared_ptr<const SampleBuffer>'s deleter.
*/
struct SampleLoader::AutoEvictor
{
	void operator()(SampleBuffer* p) const noexcept
	{
		if (!p) { return; }

		instance().remove(*p);
		delete p;
	}
};

SampleLoader::SampleLoader()
{
	connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [&](const QString& path) {
		auto lock = std::scoped_lock{m_access};
		m_entries.erase(path);
	});
}

auto SampleLoader::instance() -> SampleLoader&
{
	static SampleLoader singleton;
	return singleton;
}

auto SampleLoader::fromFile(const QString& filePath, bool cache)
	-> std::shared_ptr<const SampleBuffer>
{
	try
	{
		if (auto buffer = instance().get(filePath))
		{
			return buffer;
		}

		if (cache)
		{
			auto buffer = std::shared_ptr<SampleBuffer> {
				new SampleBuffer{SampleBuffer::Access{}, filePath},
				AutoEvictor{}
			};

			instance().add(buffer);
			return buffer;
		}
		else
		{
			return std::make_shared<SampleBuffer>(SampleBuffer::Access{}, filePath);
		}
	}
	catch (const std::runtime_error& error)
	{
		displayError(QString::fromStdString(error.what()));
		return nullptr;
	}
}

auto SampleLoader::fromBase64(const QString& base64, sample_rate_t sampleRate, bool cache)
	-> std::shared_ptr<const SampleBuffer>
{
	try
	{
		if (auto buffer = instance().get({base64, sampleRate}))
		{
			return buffer;
		}

		if (cache)
		{
			auto buffer = std::shared_ptr<SampleBuffer> {
				new SampleBuffer{SampleBuffer::Access{}, base64, sampleRate},
				AutoEvictor{}
			};

			instance().add(buffer);
			return buffer;
		}
		else
		{
			return std::make_shared<SampleBuffer>(SampleBuffer::Access{}, base64, sampleRate);
		}
	}
	catch (const std::runtime_error& error)
	{
		displayError(QString::fromStdString(error.what()));
		return nullptr;
	}
}

auto SampleLoader::fromBase64(const QString& base64, bool cache)
	-> std::shared_ptr<const SampleBuffer>
{
	return fromBase64(base64, Engine::audioEngine()->outputSampleRate(), cache);
}

auto SampleLoader::get(const SampleBuffer::Source& source) -> std::shared_ptr<const SampleBuffer>
{
	auto lock = std::scoped_lock{m_access};
	if (auto it = m_entries.find(source); it != m_entries.end())
	{
		return it->second.lock();
	}
	return nullptr;
}

void SampleLoader::add(const std::shared_ptr<const SampleBuffer>& buffer)
{
	assert(buffer != nullptr);
	if (buffer->source().type() == SampleBuffer::Source::Type::Unknown)
	{
		throw std::runtime_error{"Unsupported sample buffer source"};
	}

	auto lock = std::scoped_lock{m_access};
	const bool added = m_entries.try_emplace(buffer->source(), buffer).second;

	if (added && buffer->source().type() == SampleBuffer::Source::Type::AudioFile)
	{
		m_watcher.addPath(buffer->source().audioFileAbsolute());
	}
}

auto SampleLoader::remove(const SampleBuffer& buffer) -> bool
{
	auto lock = std::scoped_lock{m_access};
	const bool removed = m_entries.erase(buffer.source()) > 0;

	if (removed && buffer.source().type() == SampleBuffer::Source::Type::AudioFile)
	{
		m_watcher.removePath(buffer.source().audioFileAbsolute());
	}

	return removed;
}

} // namespace lmms
