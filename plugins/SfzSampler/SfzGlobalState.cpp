

#include "SfzGlobalState.h"

namespace lmms
{

void SfzGlobalState::processTrigger(const SfzTrigger& trigger)
{
	if (trigger.type() == SfzTrigger::Type::ControlChange)
	{
		m_ccValues.at(trigger.controlChangeNumber().value()) = trigger.controlChangeValue().value();
	}
	else if (trigger.type() == SfzTrigger::Type::NoteOn)
	{
		m_activeKeys.at(trigger.key().value()) = true;
		m_keyPressCounter++;
		m_lastPlayedKeys.at(trigger.key().value()) = m_keyPressCounter;
	}
	else if (trigger.type() == SfzTrigger::Type::NoteOff)
	{
		m_activeKeys.at(trigger.key().value()) = false;
	}
}

std::optional<int> SfzGlobalState::lastKeyPressedInRange(const int lowKey, const int highKey, const std::optional<int> defaultKey) const
{
	std::optional<int> lastPlayedKey = std::nullopt;
	std::optional<int> bestScore = std::nullopt;

	for (int key = lowKey; key <= highKey; ++key)
	{
		if (m_lastPlayedKeys.at(key) == std::nullopt) { continue; } // This key has not been played yet
	
		if (bestScore == std::nullopt || m_lastPlayedKeys.at(key) > bestScore)
		{
			lastPlayedKey = key;
			bestScore = m_lastPlayedKeys.at(key);
		}
	}
	// If no keys in the region have been played yet, return the default key
	if (lastPlayedKey == std::nullopt) { return defaultKey; }
	else { return lastPlayedKey; }
}


} // namespace lmms