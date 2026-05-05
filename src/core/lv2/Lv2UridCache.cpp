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

#include <lv2/atom/atom.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/midi/midi.h>
#include <lv2/parameters/parameters.h>
#include <QtGlobal>

#include "Lv2UridMap.h"

// support newer URIs on old systems
#ifndef LV2_BUF_SIZE__nominalBlockLength
#define LV2_BUF_SIZE__nominalBlockLength LV2_BUF_SIZE_PREFIX "nominalBlockLength"
#endif

namespace lmms
{


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

	init(Id::atom_Float, LV2_ATOM__Float);
	init(Id::atom_Int, LV2_ATOM__Int);
	init(Id::bufsz_minBlockLength, LV2_BUF_SIZE__minBlockLength);
	init(Id::bufsz_maxBlockLength, LV2_BUF_SIZE__maxBlockLength);
	init(Id::bufsz_nominalBlockLength, LV2_BUF_SIZE__nominalBlockLength);
	init(Id::bufsz_sequenceSize, LV2_BUF_SIZE__sequenceSize);
	init(Id::midi_MidiEvent, LV2_MIDI__MidiEvent);
	init(Id::param_sampleRate, LV2_PARAMETERS__sampleRate);

	for(uint32_t urid : m_cache) { Q_ASSERT(urid != noIdYet); }
}


} // namespace lmms

#endif // LMMS_HAVE_LV2


