/*
 * SampleCache.h
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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

#include <filesystem>

#include "FileResourceCache.h"
#include "PathUtil.h"
#include "SampleBuffer.h"
#include "SampleThumbnailCache.h"

namespace lmms {

class SampleCache
{
public:
	auto fetchBuffer(const std::filesystem::path& path) -> std::shared_ptr<const SampleBuffer>
	{
		return m_buffers.fetch(path, [&path] {
			auto buffer = SampleBuffer::fromFile(PathUtil::pathToQString(path));
			return buffer ? std::make_shared<SampleBuffer>(std::move(*buffer)) : nullptr;
		});
	}

	auto fetchThumbnailCache(const std::filesystem::path& path) -> std::shared_ptr<const SampleThumbnailCache>
	{
		return m_thumbnailCaches.fetch(path, [&path, this] {
			auto buffer = fetchBuffer(path);
			return buffer ? std::make_shared<SampleThumbnailCache>(*buffer) : nullptr;
		});
	}

	static auto instance() -> SampleCache&
	{
		static auto s_inst = SampleCache{};
		return s_inst;
	}

private:
	FileResourceCache<SampleBuffer> m_buffers;
	FileResourceCache<SampleThumbnailCache> m_thumbnailCaches;
};
} // namespace lmms

#endif // LMMS_SAMPLE_CACHE_H
