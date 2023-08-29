/*
 * Lv2UridCache.h - Lv2UridCache definition
 *
 * Copyright (c) 2020-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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
		bufsz_minBlockLength,
		bufsz_maxBlockLength,
		bufsz_nominalBlockLength,
		bufsz_sequenceSize,
		midi_MidiEvent,
		param_sampleRate,
		// exception to alphabetic ordering - keep at the end:
		size
	};

	template<typename T>
	struct IdForType;

	//! Return URID for a cache ID
	uint32_t operator[](Id id) const;

	Lv2UridCache(class UridMap& mapper);

private:
	uint32_t m_cache[static_cast<int>(Id::size)];
};

template<> struct Lv2UridCache::IdForType<float> { static constexpr auto value = Id::atom_Float; };
template<> struct Lv2UridCache::IdForType<std::int32_t> { static constexpr auto value = Id::atom_Int; };


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2_URID_CACHE_H
