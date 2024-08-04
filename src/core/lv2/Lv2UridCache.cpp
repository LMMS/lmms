/*
 * Lv2UridCache.cpp - Lv2UridCache implementation
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

#include "Lv2UridCache.h"

#ifdef LMMS_HAVE_LV2

#include <cassert>
#include <lv2/atom/atom.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/midi/midi.h>
#include <lv2/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <QtGlobal>

#include "Lv2UridMap.h"

// support newer URIs on old systems
#ifndef LV2_BUF_SIZE__nominalBlockLength
#define LV2_BUF_SIZE__nominalBlockLength LV2_BUF_SIZE_PREFIX "nominalBlockLength"
#endif

namespace lmms
{


LV2_URID Lv2UridCache::operator[](Lv2UridCache::Id id) const
{
	Q_ASSERT(id != Id::size);
	return m_cache[static_cast<std::size_t>(id)];
}




Lv2UridCache::Lv2UridCache(UridMap &mapper)
{
	checkIdNamesConsistency();

	const LV2_URID noUridYet = std::numeric_limits<LV2_URID>::max();
	std::fill_n(m_cache, static_cast<std::size_t>(Id::size), noUridYet);

	auto init = [this, &mapper](Id id, const char* uridStr)
	{
		m_cache[static_cast<std::size_t>(id)] = mapper.map(uridStr);
	};
	auto initNoUrid = [this](Id id)
	{
		m_cache[static_cast<std::size_t>(id)] = noUrid();
	};
	(void)initNoUrid;

	init(Id::atom_Float, LV2_ATOM__Float);
	init(Id::atom_Int, LV2_ATOM__Int);
	init(Id::bufsz_minBlockLength, LV2_BUF_SIZE__minBlockLength);
	init(Id::bufsz_maxBlockLength, LV2_BUF_SIZE__maxBlockLength);
	init(Id::bufsz_nominalBlockLength, LV2_BUF_SIZE__nominalBlockLength);
	init(Id::bufsz_sequenceSize, LV2_BUF_SIZE__sequenceSize);
	init(Id::midi_MidiEvent, LV2_MIDI__MidiEvent);
	init(Id::param_sampleRate, LV2_PARAMETERS__sampleRate);
#ifdef LV2_UI__backgroundColor
	init(Id::ui_backgroundColor, LV2_UI__backgroundColor);
#else
	initNoUrid(Id::ui_backgroundColor);
#endif
#ifdef LV2_UI__foregroundColor
	init(Id::ui_foregroundColor, LV2_UI__foregroundColor);
#else
	initNoUrid(Id::ui_foregroundColor);
#endif
	init(Id::ui_updateRate, LV2_UI__updateRate);
#ifdef LV2_UI__scaleFactor
	init(Id::ui_scaleFactor, LV2_UI__scaleFactor);
#else
	initNoUrid(Id::ui_scaleFactor);
#endif

	for(LV2_URID urid : m_cache)
	{
		// If you hit this assert, then you added an ID for which you did not call "init"
		Q_ASSERT(urid != noUridYet);
	}
}




void Lv2UridCache::checkIdNamesConsistency()
{
	// make sure sizes match
	static_assert(sizeof(s_idNames)/sizeof(std::string_view) == static_cast<std::size_t>(Id::size));
	// all array elements are (non-default-)initialized
	assert(s_idNames[static_cast<std::size_t>(Id::size)][0]);
	// alphabetical order
	for(std::size_t i = 1; i < static_cast<std::size_t>(Id::size); ++i)
	{
		assert(s_idNames[i-1]<s_idNames[i]);
	}
}


} // namespace lmms

#endif // LMMS_HAVE_LV2


