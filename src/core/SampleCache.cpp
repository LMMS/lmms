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

#include <algorithm>
#include <memory>

namespace lmms {
void SampleCache::add(const std::string& key, std::shared_ptr<const SampleBuffer> buffer)
{
	m_entries.emplace(key, buffer);
}

void SampleCache::remove(const std::string& key)
{
	m_entries.erase(key);
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

auto SampleCache::addEvictor(std::unique_ptr<Evictor> evictor) -> const std::unique_ptr<Evictor>&
{
	m_evictors.push_back(std::move(evictor));

	auto& addedEvictor = m_evictors.back();
	addedEvictor->setup(*this);
	return addedEvictor;
}

void SampleCache::removeEvictor(const std::unique_ptr<Evictor>& evictor)
{
	const auto it = std::remove_if(
		m_evictors.begin(), m_evictors.end(), [&](const auto& other) { return evictor == other; });
	m_evictors.erase(it, m_evictors.end());
}

} // namespace lmms
