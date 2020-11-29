/*
 * Lv2UridCache.cpp - Lv2UridCache implementation
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

#include "Lv2UridCache.h"

#ifdef LMMS_HAVE_LV2

#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <QtGlobal>

#include "Lv2UridMap.h"

uint32_t Lv2UridCache::operator[](Lv2UridCache::Id id) const
{
	Q_ASSERT(id != Id::size);
	return m_cache[static_cast<std::size_t>(id)];
}

Lv2UridCache::Lv2UridCache(UridMap &mapper)
{
	const uint32_t noIdYet = 0;
	std::fill_n(m_cache, static_cast<std::size_t>(Id::size), noIdYet);

	auto init = [this, &mapper](Id id, const char* uridStr)
	{
		m_cache[static_cast<std::size_t>(id)] = mapper.map(uridStr);
	};

	init(Id::midi_MidiEvent, LV2_MIDI__MidiEvent);

	for(uint32_t urid : m_cache) { Q_ASSERT(urid != noIdYet); }
}

#endif // LMMS_HAVE_LV2


