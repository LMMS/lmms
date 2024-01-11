/*
 * SampleLoader.cpp - Sample loader with an optional cache
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

#include <QMessageBox>
#include <cassert>

#include "Engine.h"
#include "GuiApplication.h"
#include "PathUtil.h"
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
 * immediately after the last reference to the buffer is dropped
*/
class SampleLoader::AutoEvictor
{
public:
	void operator()(SampleBuffer* p) const noexcept
	{
		if (!p) { return; }

		SampleLoader::inst().remove(*p);
		delete p;
	}
};

SampleLoader::SampleLoader()
{
	connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &SampleLoader::removeFile);
}

auto SampleLoader::inst() -> SampleLoader&
{
	static SampleLoader singleton;
	return singleton;
}

auto SampleLoader::fromFile(const QString& filePath, Cache cache)
	-> std::shared_ptr<const SampleBuffer>
{
	if (filePath.isEmpty()) { return nullptr; }

	switch (cache)
	{
		case Cache::Read: [[fallthrough]];
		case Cache::ReadWrite:
		{
			if (auto buffer = SampleLoader::inst().getFile(filePath))
			{
				return buffer;
			}
		}
		[[fallthrough]];
		case Cache::None:
		{
			try
			{
				if (cache == Cache::ReadWrite)
				{
					auto buffer = std::shared_ptr<SampleBuffer> {
						new SampleBuffer{SampleBuffer::Access{}, filePath},
						AutoEvictor{}
					};
					SampleLoader::inst().add(buffer);
					return buffer;
				}

				return std::make_shared<SampleBuffer>(SampleBuffer::Access{}, filePath);
			}
			catch (const std::runtime_error& error)
			{
				displayError(QString::fromStdString(error.what()));
				return nullptr;
			}
		}
		default:
			return nullptr; // invalid
	}
}

auto SampleLoader::fromBase64(const QString& base64, int sampleRate, Cache cache)
	-> std::shared_ptr<const SampleBuffer>
{
	if (base64.isEmpty()) { return nullptr; }

	switch (cache)
	{
		case Cache::Read: [[fallthrough]];
		case Cache::ReadWrite:
		{
			if (auto buffer = SampleLoader::inst().getBase64(base64))
			{
				return buffer;
			}
		}
		[[fallthrough]];
		case Cache::None:
		{
			try
			{
				if (cache == Cache::ReadWrite)
				{
					auto buffer = std::shared_ptr<SampleBuffer> {
						new SampleBuffer{SampleBuffer::Access{}, base64, sampleRate},
						AutoEvictor{}
					};
					SampleLoader::inst().add(buffer);
					return buffer;
				}

				return std::make_shared<SampleBuffer>(SampleBuffer::Access{}, base64, sampleRate);
			}
			catch (const std::runtime_error& error)
			{
				displayError(QString::fromStdString(error.what()));
				return nullptr;
			}
		}
		default:
			return nullptr; // invalid
	}
}

auto SampleLoader::fromBase64(const QString& base64, Cache cache)
	-> std::shared_ptr<const SampleBuffer>
{
	return fromBase64(base64, Engine::audioEngine()->processingSampleRate(), cache);
}

void SampleLoader::add(const std::shared_ptr<const SampleBuffer>& buffer)
{
	assert(buffer != nullptr);
	if (buffer->sourceType() == SampleBuffer::Source::Unknown)
	{
		throw std::runtime_error{"Unsupported sample buffer source"};
	}

	const bool added = m_entries.try_emplace(Key{buffer->source(), buffer->sourceType()}, buffer).second;

	if (added && buffer->sourceType() == SampleBuffer::Source::AudioFile)
	{
		m_watcher.addPath(buffer->audioFileAbsolute());
	}
}

auto SampleLoader::remove(const SampleBuffer& buffer) -> bool
{
	const bool removed = m_entries.erase(Key{buffer.source(), buffer.sourceType()}) > 0;

	if (removed && buffer.sourceType() == SampleBuffer::Source::AudioFile)
	{
		m_watcher.removePath(buffer.audioFileAbsolute());
	}

	return removed;
}

auto SampleLoader::getFile(const QString& filePath) -> std::shared_ptr<const SampleBuffer>
{
	if (auto it = m_entries.find(Key{PathUtil::toAbsolute(filePath), Source::AudioFile}); it != m_entries.end())
	{
		return it->second.lock();
	}
	return nullptr;
}

auto SampleLoader::getBase64(const QString& base64) -> std::shared_ptr<const SampleBuffer>
{
	// TODO: Also include sample rate in base64 key?
	if (auto it = m_entries.find(Key{base64, Source::Base64}); it != m_entries.end())
	{
		return it->second.lock();
	}
	return nullptr;
}

void SampleLoader::removeFile(const QString& path)
{
	m_entries.erase(Key{path, SampleBuffer::Source::AudioFile});
}

} // namespace lmms
