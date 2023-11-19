/*
 * Lv2UridMap.cpp - Lv2UridMap class
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

#ifndef LMMS_LV2_URID_MAP_H
#define LMMS_LV2_URID_MAP_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <mutex> // TODO: use semaphore, even though this is not realtime critical
#include <string>
#include <unordered_map>
#include <vector>


namespace lmms
{


/**
 * Complete implementation of the Lv2 Urid Map extension
 */
class UridMap
{
	std::unordered_map<std::string, LV2_URID> m_map;
	std::vector<const char*> m_unMap;

	//! mutex for both m_map and m_unMap
	//! the URID map is global, which is why a mutex is required here
	std::mutex m_MapMutex;

	LV2_URID_Map m_mapFeature;
	LV2_URID_Unmap m_unmapFeature;

public:
	//! constructor; will set up the features
	UridMap();

	//! map feature function
	LV2_URID map(const char* uri);
	//! unmap feature function
	const char* unmap(LV2_URID urid);

	// access the features
	LV2_URID_Map* mapFeature() { return &m_mapFeature; }
	LV2_URID_Unmap* unmapFeature() { return &m_unmapFeature; }
};


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2_URID_MAP_H
