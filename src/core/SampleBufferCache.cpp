/*
 * SampleBufferCache.cpp - Used to cache sample buffers
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#include "SampleBufferCache.h"
#include "SampleBufferV2.h"

namespace lmms
{
	std::shared_ptr<const SampleBufferV2> SampleBufferCache::get(const QString& id)
	{
		if (m_map.find(id) == m_map.end()) { return nullptr; }
		return m_map.at(id).lock();
	}

	std::shared_ptr<const SampleBufferV2> SampleBufferCache::add(const QString& id, bool isBase64)
	{
		if (m_map.find(id) != m_map.end()) { return m_map.at(id).lock(); }

		auto sharedBuffer = std::shared_ptr<const SampleBufferV2>(new SampleBufferV2{id, isBase64}, 
			[this, id](const SampleBufferV2* ptr)
			{
				delete ptr;
				m_map.erase(id);
			}
		);

		m_map.emplace(id, std::weak_ptr<const SampleBufferV2>(sharedBuffer));
		return sharedBuffer;
	}

	size_t SampleBufferCache::Hash::operator()(const QString& str) const noexcept 
	{
		return qHash(str);
	}
} // namespace lmms
