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

#include <ctime>
#include <array>

namespace lmms
{

class InstrumentTrack;
class MidiPort;


class MPEManager
{
public:
	MPEManager(InstrumentTrack* instrumentTrack);

	int findAvailableChannel(int key, bool willNotChange = false);
	void sendMPEConfigSignals();

	void noteOn(int channel)
	{
		m_channelNoteCounts[channel]++;
	}
	void noteOff(int channel)
	{
		m_channelNoteCounts[channel]--;
		m_channelNoteOffTimes[channel] = std::time(nullptr);
	}
private:
	std::array<int, 16> m_channelNoteCounts = {};
	std::array<std::time_t, 16> m_channelNoteOffTimes = {};

	InstrumentTrack* m_instrumentTrack;
} ;

} // namespace lmms

#endif // LMMS_MPE_MANAGER_H
