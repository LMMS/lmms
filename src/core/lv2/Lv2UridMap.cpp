/*
 * Lv2UridMap.cpp - Lv2UridMap implementation
 *
 * Copyright (c) 2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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


#include "Lv2UridMap.h"

#ifdef LMMS_HAVE_LV2

namespace lmms
{


static LV2_URID staticMap(LV2_URID_Map_Handle handle, const char* uri)
{
	auto map = static_cast<UridMap*>(handle);
	return map->map(uri);
}

static const char* staticUnmap(LV2_URID_Unmap_Handle handle, LV2_URID urid)
{
	auto map = static_cast<UridMap*>(handle);
	return map->unmap(urid);
}

UridMap::UridMap()
{
	m_mapFeature.handle = static_cast<LV2_URID_Map_Handle>(this);
	m_mapFeature.map = staticMap;
	m_unmapFeature.handle = static_cast<LV2_URID_Unmap_Handle>(this);
	m_unmapFeature.unmap = staticUnmap;
}

LV2_URID UridMap::map(const char *uri)
{
	LV2_URID result = 0u;

	// the Lv2 docs say that 0 should be returned in any case
	// where creating an ID for the given URI fails
	try
	{
		// TODO:
		// when using C++14, we can get around any string allocation
		// in the case the URI is already inside the map:
		// * use `m_map.find(uri)` instead of `m_map.find(uriStr)`
		// * to avoid temporary string construction in the `find` call, create
		//   m_map like this:
		//   std::unordered_map<std::string, LV2_URID,
		//     std::hash<std::string>, std::equal<>> m_map;
		// * move the try block inside the case where the URI is not in the map
		const std::string uriStr = uri;

		std::lock_guard<std::mutex> guard (m_MapMutex);

		auto itr = m_map.find(uriStr);
		if (itr == m_map.end())
		{
			// 1 is the first free URID
			std::size_t index = 1u + m_unMap.size();
			auto pr = m_map.emplace(std::move(uriStr), index);
			if (pr.second)
			{
				m_unMap.emplace_back(pr.first->first.c_str());
				result = static_cast<LV2_URID>(index);
			}
		}
		else { result = itr->second; }
	}
	catch(...) { /* result variable is already 0 */ }

	return result;
}

const char *UridMap::unmap(LV2_URID urid)
{
	std::size_t idx = static_cast<std::size_t>(urid) - 1;

	std::lock_guard<std::mutex> guard (m_MapMutex);
	return (idx < m_unMap.size()) ? m_unMap[idx] : nullptr;
}


} // namespace lmms

#endif // LMMS_HAVE_LV2

