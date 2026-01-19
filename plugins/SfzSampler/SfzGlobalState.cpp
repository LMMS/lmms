

#include "SfzGlobalState.h"
#include <QDebug>
#include <iostream>

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

void SfzGlobalState::switchKeyPressed(const int key)
{
	// Assuming the m_keyPressCounter was already updated in processTrigger
	m_lastPlayedSwitchKeys.at(key) = m_keyPressCounter;
}

std::optional<int> SfzGlobalState::lastKeyPressedInRange(int lowKey, int highKey, const std::optional<int> defaultKey, bool switchKeysOnly) const
{
	auto& lastKeyPressArray = switchKeysOnly ? m_lastPlayedSwitchKeys : m_lastPlayedKeys;
	// Some SFZs pass -1 as the lokey, so make sure to clamp it into the range 0-127 before accessing the array to prevent out of range errors
	lowKey = std::max(lowKey, 0);
	highKey = std::min(highKey, 127);
	// If the range is invalid, return the default key.
	if (lowKey > highKey) { return defaultKey; }

	std::optional<int> lastPlayedKey = std::nullopt;
	std::optional<int> bestScore = std::nullopt;

	for (int key = lowKey; key <= highKey; ++key)
	{
		if (lastKeyPressArray.at(key) == std::nullopt) { continue; } // This key has not been played yet
	
		if (bestScore == std::nullopt || lastKeyPressArray.at(key) > bestScore)
		{
			lastPlayedKey = key;
			bestScore = lastKeyPressArray.at(key);
		}
	}
	// If no keys in the region have been played yet, return the default key
	if (lastPlayedKey == std::nullopt) { return defaultKey; }
	else { return lastPlayedKey; }
}


void SfzGlobalState::initializeMidiCCValues(const SfzOpcodeState& controlsConfig)
{
	for (int i = 0; i < SfzOpcodeState::NumMidiCCs; ++i)
	{
		m_ccValues.at(i) = controlsConfig.m_set_cc.at(i);
	}
}


} // namespace lmms