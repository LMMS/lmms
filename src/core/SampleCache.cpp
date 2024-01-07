/*
 * SampleCache.cpp
 *
 * Copyright (c) 2024 saker <sakertooth@gmail.com>
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

#include "SampleBuffer.h"

namespace lmms {

SampleCache::SampleCache()
{
	QObject::connect(&m_fsWatcher, &QFileSystemWatcher::fileChanged, [this](const QString& path) {
		m_samples.remove(path);
		if (m_fsWatcher.files().contains(path)) { m_fsWatcher.removePath(path); }
	});
}

auto SampleCache::get(const QString& path) -> std::shared_ptr<const SampleBuffer>
{
	if (const auto it = m_samples.find(path); it != m_samples.end())
	{
		if (const auto buffer = it.value().lock()) { return buffer; }
	}

	// TODO: Do not signify failure by returning an empty buffer.
	try
	{
		const auto buffer = std::make_shared<const SampleBuffer>(path);
		m_samples.insert(path, buffer);
		m_fsWatcher.addPath(path);
		return buffer;
	}
	catch (const std::runtime_error& error) { return SampleBuffer::emptyBuffer(); }
}

auto SampleCache::instance() -> SampleCache&
{
	static auto instance = SampleCache{};
	return instance;
}

} // namespace lmms
