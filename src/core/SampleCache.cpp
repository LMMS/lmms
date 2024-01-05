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

#include <memory>

namespace lmms {
void SampleCache::add(const std::string& key, std::shared_ptr<const SampleBuffer> buffer)
{
	m_entries.emplace(key, buffer);
}

auto SampleCache::get(const std::string& key) -> std::optional<std::shared_ptr<const SampleBuffer>>
{
	const auto it = m_entries.find(key);
	if (it == m_entries.end()) { return std::nullopt; }
	return it->second;
}

auto SampleCache::contains(const std::string& key) -> bool
{
	return m_entries.find(key) != m_entries.end();
}

} // namespace lmms
