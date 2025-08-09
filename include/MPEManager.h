/*
 * MPEManager.h - Helper class for dealing with Midi Polyphonic Expression routing and configuration
 *
 * Copyright (c) 2025 Keratin
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

#ifndef LMMS_MPE_MANAGER_H
#define LMMS_MPE_MANAGER_H

#include <array>
#include <cassert>

namespace lmms
{

class MidiEventProcessor;
class MidiPort;


class MPEManager
{
public:
	enum MPEZone
	{
		Lower,
		Upper
	};

	void config(int numChannelsLowerZone = 16, int numChannelsUpperZone = 0, int pitchBendRange = 48, MPEZone zone = MPEZone::Lower)
	{
		// Ensure the zones do not overlap
		assert(numChannelsLowerZone + numChannelsUpperZone <= 16);
		m_numChannelsLowerZone = numChannelsLowerZone;
		m_numChannelsUpperZone = numChannelsUpperZone;
		m_pitchBendRange = pitchBendRange;
		m_zone = zone;
	}

	int findAvailableChannel(int key = 0, bool willNotChange = false);
	void sendMPEConfigSignals(MidiEventProcessor* proc);

	void noteOn(int channel)
	{
		m_channelNoteCounts[channel]++;
	}
	void noteOff(int channel)
	{
		m_channelNoteCounts[channel]--;
		m_noteOffCount++;
		m_channelNoteOffTimes[channel] = m_noteOffCount;
	}

private:
	std::array<int, 16> m_channelNoteCounts = {};
	std::array<int, 16> m_channelNoteOffTimes = {};
	int m_noteOffCount = 0; // Used in place of the current time as a way to compare the age of NoteOff events
	int m_numChannelsLowerZone = 16;
	int m_numChannelsUpperZone = 0;
	int m_pitchBendRange = 48;
	MPEZone m_zone = MPEZone::Lower;
} ;

} // namespace lmms

#endif // LMMS_MPE_MANAGER_H
