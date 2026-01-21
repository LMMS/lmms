
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
	SfzRegionManager() = default;
	SfzRegionManager(std::vector<SfzRegion>& regions);

	//! Based on the trigger type, key, etc, returns a list of regions which might match the trigger.
	//! This is meant to intelligently narrow down the number of regions where we manually have to loop over and check
	const std::vector<SfzRegion*>& findPotentialMatchingRegions(const SfzTrigger& trigger) const;

	const std::vector<SfzRegion*>& allRegions() const { return m_allRegions; }

private:
	std::vector<SfzRegion> m_regions;

	std::vector<SfzRegion*> m_allRegions;
	std::array<std::vector<SfzRegion*>, 128> m_noteOnRegions;
	std::array<std::vector<SfzRegion*>, 128> m_noteOffRegions;
};

} // namespace lmms

#endif // LMMS_SFZ_REGION_MANAGER_H
