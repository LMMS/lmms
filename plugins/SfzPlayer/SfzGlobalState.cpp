/*
 * SfzGlobalState.cpp - Class which holds information about the current state of the
 * SFZ player, such as currently/previously pressed keys, and current midi CC values
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


#include "SfzGlobalState.h"
#include "lmms_math.h"

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
		if (m_lastPlayedSwitchKeys.contains(trigger.key().value()))
		{
			m_switchKeyPressCounter++;
			m_lastPlayedSwitchKeys.at(trigger.key().value()) = m_switchKeyPressCounter;
			m_pressedSwitchKeys.at(trigger.key().value()) = true;
			m_lastPlayedSwitchKey = trigger.key().value();
		}
	}
	else if (trigger.type() == SfzTrigger::Type::NoteOff)
	{
		if (m_lastPlayedSwitchKeys.contains(trigger.key().value()))
		{
			m_pressedSwitchKeys.at(trigger.key().value()) = false;
		}
	}
	// Update the current random value
	m_rand = fastRand(1.0f);
}

std::optional<int> SfzGlobalState::lastSwitchKeyPressedInRange(int lowKey, int highKey, const std::optional<int> defaultKey) const
{
	// The range might only include some of the switch keys
	// If the last played switch key happens to fall in the range, that's awesome!
	if (m_lastPlayedSwitchKey >= lowKey && m_lastPlayedSwitchKey <= highKey) { return m_lastPlayedSwitchKey; }
	// Otherwise, we need to loop over the list of when each switch key was played and figure out which one was last played
	std::optional<int> lastPlayedKey = std::nullopt;
	std::optional<int> bestScore = std::nullopt;
	for (const auto& [key, counter] : m_lastPlayedSwitchKeys)
	{
		if (counter == std::nullopt) { continue; } // This key has not been played yet

		if (bestScore == std::nullopt || counter > bestScore)
		{
			lastPlayedKey = key;
			bestScore = counter;
		}
	}
	// If no keys in the region have been played yet, return the default key
	if (lastPlayedKey == std::nullopt) { return defaultKey; }
	else { return lastPlayedKey; }
}


void SfzGlobalState::initializeMidiCCValues(const SfzControlsConfig& controlsConfig)
{
	for (int i = 0; i < NumMidiCCs; ++i)
	{
		m_ccValues.at(i) = controlsConfig.m_set_cc.at(i);
	}
}

void SfzGlobalState::initializeSwitchKeysMap(const SfzControlsConfig& controlsConfig)
{
	for (const auto& [key, info] : controlsConfig.m_switchKeyInfo)
	{
		m_lastPlayedSwitchKeys[key] = std::nullopt;
		m_pressedSwitchKeys[key] = false;
	}
}


} // namespace lmms
