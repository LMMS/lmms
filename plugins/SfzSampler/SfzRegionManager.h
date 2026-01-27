/*
 * SfzRegionManager.h - Helper class for optimizing region selection based on trigger
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

#ifndef LMMS_SFZ_REGION_MANAGER_H
#define LMMS_SFZ_REGION_MANAGER_H

#include "SfzRegion.h"


namespace lmms
{

class SfzRegionManager
{
public:
	//! The point of this class is to speed up processing triggers (NoteOn/NoteOff/etc events)
	//! Normally, if your SFZ file has tens of thousands of regions, you have to loop over every single one each time a key
	//! is pressed to check whether all the conditions are met and it should spawn a voice or not (is the key within lokey/hikey range,
	//! is the velocity within lovel/hivel range, has the right switch key been pressed, are the midi CC's in the right ranges, etc)
	//! This is not optimal.
	//! Instead, because there are only 128 keys, we can construct a mapping, where each key is paired with a list of regions where lokey/hikey matches it
	//! Doing this for both NoteOn and NoteOff events means having 256 lists of pointers to regions, which is not bad.
	//! That way, whenever a note is pressed, you can simply look up in the table a list of all the regions which might match.
	//! This is usually 10-100x fewer regions than the total, which is much faster to loop over.
	SfzRegionManager() = default;
	SfzRegionManager(std::vector<SfzRegion>& regions);

	//! Based on the trigger type, key, etc, returns a list of regions which might match the trigger.
	//! This is meant to intelligently narrow down the number of regions where we manually have to loop over and check
	const std::vector<SfzRegion*>& findPotentialMatchingRegions(const SfzTrigger& trigger) const;

	//! Returns a vector of pointers for all the regions, in case any code needs to loop over all of them.
	const std::vector<SfzRegion*>& allRegions() const { return m_allRegions; }

private:
	//! Stores the actual region objects
	std::vector<SfzRegion> m_regions;

	std::vector<SfzRegion*> m_allRegions;
	std::array<std::vector<SfzRegion*>, 128> m_noteOnRegions;
	std::array<std::vector<SfzRegion*>, 128> m_noteOffRegions;
};

} // namespace lmms

#endif // LMMS_SFZ_REGION_MANAGER_H
