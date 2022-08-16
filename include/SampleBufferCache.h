/*
 * SampleBufferCache.h - Used to cache sample buffers
 *
 * Copyright (c) 2022 sakertooth <sakertooth@gmail.com>
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

#ifndef SAMPLE_BUFFER_CACHE_H
#define SAMPLE_BUFFER_CACHE_H

#include "SampleBufferV2.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace lmms 
{
	class SampleBufferCache
	{
	public:
		using CacheID = std::experimental::filesystem::path::string_type;
		
		std::shared_ptr<const SampleBufferV2> get(const CacheID& id);
		std::shared_ptr<const SampleBufferV2> add(const CacheID& id);
	private:
		std::unordered_map<CacheID, std::weak_ptr<const SampleBufferV2>> m_map;
	};
}

#endif
