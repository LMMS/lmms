/*
 * MPEManager.cpp - Helper class for dealing with Midi Polyphonic Expression
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

#include "MPEManager.h"

#include "InstrumentTrack.h"
#include "MidiPort.h"

#include <QDebug>

namespace lmms
{

MPEManager::MPEManager(InstrumentTrack* instrumentTrack):
	m_instrumentTrack(instrumentTrack)
{
}

// TODO add support for upper zone?
// TODO currently no calls use `willNotChange`
int MPEManager::findAvailableChannel(int key, bool willNotChange)
{
	// For the lower zone, the first channel, channel 0, is the Manager channel. The channels after that are Member channels.
	// For the upper zone, the last channel, channel 15, is the Manager channel. The channels below 15 are Member channels.

	const int masterChannel = 0; // TODO add support for upper zone

	const int numChannels = m_instrumentTrack->midiPort()->MPELowerZoneChannels();
	const int numMemberChannels = numChannels - 1;

	// The MPE specification, page 27 Appendix E, does not prohibit NoteOn/NoteOff signals on the Manager channels.
	// However, notes should ideally not be routed to Manager channels, since then you can't control their pitch individually
	// without affecting the pitch of the whole instrument.

	// Unless there are no member channels, in which case we default to just sending everything on one channel.
	if (numMemberChannels <= 0) { return masterChannel; }

	// The MPE specification (page 17, Appendix A.3) gives some ideas on how to implement a good note routing system which aims to minimize the
	// chance for notes being incorrectly pitch bent due to being on the same channel as another note being pitch bent.

	// Their proposed method routes notes to the channels which have the fewest active notes. If two channels have the same number of notes, the ones with the oldest NoteOff signal is preferred.

	// However, this method can be improved in the case where LMMS knows for certain that a note will not be pitch bent, as
	// then it can route them all to a single channel (perhaps the manager channel?) without having to worry about one of them suddenly bending everything at once.
	// This is only really possible for MidiClip-based notes where we know what the detuning curve looks like in advance. Notes
	// coming in from input midi events cannot be guaranteed not to bend at some later point in time.

	// If LMMS cannot guarantee that the incoming note will not bend, then it will be routed as normal to the channel with the fewest and/or
	// oldest note off signal


	// Route static note to Manager channel as described above.
	if (willNotChange) { return masterChannel; }

	// Find channel with fewest notes/oldest NoteOff signal
	// Starting at channel 1 to avoid Manager Channel
	int bestChannel = 1;
	int bestNoteCount = m_channelNoteCounts[bestChannel];
	std::time_t bestNoteOffTime = m_channelNoteOffTimes[bestChannel];
	for (int channel = 1; channel < numMemberChannels + 1; ++channel)
	{
		if (m_channelNoteCounts[channel] < bestNoteCount
			|| (m_channelNoteCounts[channel] == bestNoteCount && m_channelNoteOffTimes[channel] < bestNoteOffTime))
		{
			bestChannel = channel;
			bestNoteCount = m_channelNoteCounts[channel];
			bestNoteOffTime = m_channelNoteOffTimes[channel];
		}
	}


	qDebug() << "Channel Counts:"
		<< m_channelNoteCounts[0]
		<< m_channelNoteCounts[1]
		<< m_channelNoteCounts[2]
		<< m_channelNoteCounts[3]
		<< m_channelNoteCounts[4]
		<< m_channelNoteCounts[5]
		<< m_channelNoteCounts[6]
		<< m_channelNoteCounts[7]
		<< m_channelNoteCounts[8]
		<< m_channelNoteCounts[9]
		<< m_channelNoteCounts[10]
		<< m_channelNoteCounts[11]
		<< m_channelNoteCounts[12]
		<< m_channelNoteCounts[13]
		<< m_channelNoteCounts[14]
		<< m_channelNoteCounts[15];
	qDebug() << "note off times relative:"
		<< m_channelNoteOffTimes[0] - std::time(nullptr)
		<< m_channelNoteOffTimes[1] - std::time(nullptr)
		<< m_channelNoteOffTimes[2] - std::time(nullptr)
		<< m_channelNoteOffTimes[3] - std::time(nullptr)
		<< m_channelNoteOffTimes[4] - std::time(nullptr)
		<< m_channelNoteOffTimes[5] - std::time(nullptr)
		<< m_channelNoteOffTimes[6] - std::time(nullptr)
		<< m_channelNoteOffTimes[7] - std::time(nullptr)
		<< m_channelNoteOffTimes[8] - std::time(nullptr)
		<< m_channelNoteOffTimes[9] - std::time(nullptr)
		<< m_channelNoteOffTimes[10] - std::time(nullptr)
		<< m_channelNoteOffTimes[11] - std::time(nullptr)
		<< m_channelNoteOffTimes[12] - std::time(nullptr)
		<< m_channelNoteOffTimes[13] - std::time(nullptr)
		<< m_channelNoteOffTimes[14] - std::time(nullptr)
		<< m_channelNoteOffTimes[15] - std::time(nullptr);

	return bestChannel;
}


// TODO add comments
void MPEManager::sendMPEConfigSignals()
{
	// Setup MPE Zone 1
	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerRegisteredParameterNumberMSB, (MidiMPEConfigurationRPN >> 8) & 0x7F));
	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerRegisteredParameterNumberLSB, MidiMPEConfigurationRPN & 0x7F));
	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerDataEntry, m_instrumentTrack->midiPort()->MPELowerZoneChannels()));

	// Send null RPN signals to end the control change. I heard this was good practice.
	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerRegisteredParameterNumberMSB, (MidiNullFunctionNumberRPN >> 8) & 0x7F));
	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerRegisteredParameterNumberLSB, MidiNullFunctionNumberRPN & 0x7F));
	// Setup MPE Zone 2
	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerRegisteredParameterNumberMSB, (MidiMPEConfigurationRPN >> 8) & 0x7F));
	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerRegisteredParameterNumberLSB, MidiMPEConfigurationRPN & 0x7F));
	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerDataEntry, m_instrumentTrack->midiPort()->MPEUpperZoneChannels()));

	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerRegisteredParameterNumberMSB, (MidiNullFunctionNumberRPN >> 8) & 0x7F));
	m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerRegisteredParameterNumberLSB, MidiNullFunctionNumberRPN & 0x7F));

	// Set pitch bend range to on all Member channels
	// TODO this doesn't always work on all VSTs. I have the default at 48 because that seems to be the default in the MPE spec, but lmms likes to use 60 so...
	// Also currently this doesn't affect the manager channels, 0 and 15. But I think that's fine, since the pitch wheel is supposed to handle that.
	for (int channel = 1; channel < 15; ++channel)
	{
		m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberMSB, (MidiPitchBendSensitivityRPN >> 8) & 0x7F));
		m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberLSB, MidiPitchBendSensitivityRPN & 0x7F));
		m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerDataEntry, m_instrumentTrack->midiPort()->MPEPitchRange()));
		m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberMSB, (MidiNullFunctionNumberRPN >> 8) & 0x7F));
		m_instrumentTrack->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberLSB, MidiNullFunctionNumberRPN & 0x7F));
	}

}


} // namespace lmms
