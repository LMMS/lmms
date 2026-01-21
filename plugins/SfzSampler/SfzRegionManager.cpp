

#include "SfzRegionManager.h"
#include <QDebug>


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
}




} // namespace lmms