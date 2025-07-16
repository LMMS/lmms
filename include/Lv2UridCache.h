/*
 * Lv2UridCache.h - Lv2UridCache definition
 *
 * Copyright (c) 2020-2024 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_LV2_URID_CACHE_H
#define LMMS_LV2_URID_CACHE_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <cstdint>
#include <string_view>
#include <lv2/urid/urid.h>


namespace lmms
{


//! Cached URIDs for fast access (for use in real-time code)
class Lv2UridCache
{
public:
	enum class Id //!< ID for m_uridCache array
	{
		// keep it alphabetically (except "size" at the end)
		atom_Float,
		atom_Int,
		bufsz_maxBlockLength,
		bufsz_minBlockLength,
		bufsz_nominalBlockLength,
		bufsz_sequenceSize,
		midi_MidiEvent,
		param_sampleRate,
		ui_backgroundColor,
		ui_foregroundColor,
		ui_scaleFactor,
		ui_updateRate,
		// exception to alphabetic ordering - keep at the end:
		size
	};

	template<typename T>
	struct IdForType;

	static LV2_URID noUrid() { return 0; }

	//! Return URID for a cache ID
	LV2_URID operator[](Id id) const;
	//! Return name of an ID
	static const char* nameOfId(Id id)
	{
		return s_idNames[static_cast<std::size_t>(id)].data();
	}

	Lv2UridCache(class UridMap& mapper);

private:
	LV2_URID m_cache[static_cast<int>(Id::size)];

	// must match Id enum!
	static constexpr std::string_view s_idNames[static_cast<std::size_t>(Id::size)] =
	{
		"atom_Float",
		"atom_Int",
		"bufsz_maxBlockLength",
		"bufsz_minBlockLength",
		"bufsz_nominalBlockLength",
		"bufsz_sequenceSize",
		"midi_MidiEvent",
		"param_sampleRate",
		"ui_backgroundColor",
		"ui_foregroundColor",
		"ui_scaleFactor",
		"ui_updateRate"
	};

	static void checkIdNamesConsistency();
};

template<> struct Lv2UridCache::IdForType<float> { static constexpr auto value = Id::atom_Float; };
template<> struct Lv2UridCache::IdForType<std::int32_t> { static constexpr auto value = Id::atom_Int; };


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2_URID_CACHE_H
