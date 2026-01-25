/*
 * SfzRegionManager.cpp - Helper class for optimizing region selection based on trigger
 *
 * Copyright (c) 2026 Keratin
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

#include "SfzRegionManager.h"


namespace lmms
{


SfzRegionManager::SfzRegionManager(std::vector<SfzRegion>& regions)
	: m_regions(regions)
{
	// For each key, figure out what list of regions could include it, to make a kind of look-up table
	for (int key = 0; key < 128; ++key)
	{
		for (auto& region : m_regions)
		{
			if (key >= region.m_lokey && key <= region.m_hikey)
			{
				// Additionally sort by trigger type
				switch (region.m_trigger)
				{
				case SfzOpcodeState::TriggerType::Attack:
					m_noteOnRegions.at(key).push_back(&region);
					break;
				case SfzOpcodeState::TriggerType::Release:
					m_noteOffRegions.at(key).push_back(&region);
					break;
				}
			}
		}
	}

	// Unfortunately, it's more difficult to make lookup tables for midi CC events. Instead, just use a big vector with all the regions by default
	// TODO or is it? there's probably a way. But currently we actually don't support cc events triggering regions, so technically this will never be used at the moment.
	for (auto& region : m_regions)
	{
		m_allRegions.push_back(&region);
	}
}



const std::vector<SfzRegion*>& SfzRegionManager::findPotentialMatchingRegions(const SfzTrigger& trigger) const
{
	switch (trigger.type())
	{
	case SfzTrigger::Type::NoteOn:
		return m_noteOnRegions.at(trigger.key().value());
	case SfzTrigger::Type::NoteOff:
		return m_noteOffRegions.at(trigger.key().value());
	case SfzTrigger::Type::ControlChange:
		return m_allRegions; // TODO is it possible to somehow make a lookup table for CC events?
	}
	return m_allRegions; // This was added to prevent a warning, but I don't think it's necessary, since the switch covers all cases.
}




} // namespace lmms