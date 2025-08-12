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
	enum class Zone
	{
		Lower,
		Upper
	};

	//! Returns the manager channel of the current zone, for sending global events (such as master pitch wheel)
	int managerChannel()
	{
		return m_zone == Zone::Lower
			? 0
			: 16;
	}

	//! Sets the MPE configuration before sending it as MIDI events
	void config(int numChannelsLowerZone = 16, int numChannelsUpperZone = 0, int pitchBendRange = 48, Zone zone = Zone::Lower)
	{
		// Ensure the zones do not overlap
		assert(numChannelsLowerZone + numChannelsUpperZone <= 16);
		m_numChannelsLowerZone = numChannelsLowerZone;
		m_numChannelsUpperZone = numChannelsUpperZone;
		m_pitchBendRange = pitchBendRange;
		m_zone = zone;
	}

	//! Returns the channel in the current zone with the fewest notes, or with the oldest NoteOff. Returns -1 if the zone has no channels.
	int findAvailableChannel();

	//! Calls processOutEvent on the given MidiEventProcessor (such as an InstrumentTrack) to send all the MPE configuration and pitch bend range events and reset the pitch bends for all channels.
	void sendMPEConfigSignals(MidiEventProcessor* proc);

	//! Increments the internal counter of active notes on the given channel
	void noteOn(int channel)
	{
		m_channelNoteCounts[channel]++;
	}
	//! Decrements the internal counter of active notes on the given channel, and records the total count of the NoteOff signals up to this point to compare which channel has the oldest NoteOff
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
	Zone m_zone = Zone::Lower;
} ;

} // namespace lmms

#endif // LMMS_MPE_MANAGER_H
