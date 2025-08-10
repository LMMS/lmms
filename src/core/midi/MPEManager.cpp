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

#include "MidiEventProcessor.h"
#include "MidiPort.h"

namespace lmms
{

int MPEManager::findAvailableChannel()
{
	// For the lower zone, the first channel, channel 0, is the Manager channel. The channels after that are Member channels.
	// For the upper zone, the last channel, channel 15, is the Manager channel. The channels below 15 are Member channels.

	const int numChannels = m_zone == MPEZone::Lower
		? m_numChannelsLowerZone
		: m_numChannelsUpperZone;
	const int numMemberChannels = numChannels - 1;
	// Page 23, B.6, if there are no member channels the zone should deactivate.
	if (numMemberChannels <= 0) { return -1; }

	const int firstMemberChannel = m_zone == MPEZone::Lower
		? 1
		: 15 - numMemberChannels;
	const int lastMemberChannel = m_zone == MPEZone::Lower
		? numMemberChannels
		: 14;

	// The MPE specification, page 27 Appendix E, does not prohibit NoteOn/NoteOff signals on the Manager channels.
	// However, notes should ideally not be routed to Manager channels, since then you can't control their pitch individually
	// without affecting the pitch of the whole instrument.

	// Page 17, Appendix A.3 gives some ideas on how to implement a good note routing system which aims to minimize the
	// chance for notes being incorrectly pitch bent due to being on the same channel as another note being pitch bent.

	// Their proposed method routes notes to the channels which have the fewest active notes.
	// If two channels have the same number of notes, the ones with the oldest NoteOff signal is preferred.

	// Find member channel with fewest notes/oldest NoteOff signal
	int bestChannel = firstMemberChannel;
	int bestNoteCount = m_channelNoteCounts[bestChannel];
	// Technically we are not using time, but instead the count of NoteOff events up to that point. This way it does not depend on the current time.
	int bestNoteOffTime = m_channelNoteOffTimes[bestChannel];
	for (int channel = firstMemberChannel; channel <= lastMemberChannel; ++channel)
	{
		if (m_channelNoteCounts[channel] < bestNoteCount
			|| (m_channelNoteCounts[channel] == bestNoteCount && m_channelNoteOffTimes[channel] < bestNoteOffTime))
		{
			bestChannel = channel;
			bestNoteCount = m_channelNoteCounts[channel];
			bestNoteOffTime = m_channelNoteOffTimes[channel];
		}
	}
	return bestChannel;
}


void MPEManager::sendMPEConfigSignals(MidiEventProcessor* proc)
{
	// The MPE config signal is sent just like the pitch bend range signal. The first 7 bits of the message id is sent (Most Significant Bits / MSB), then the last 7 bits (Least Significant Bits / LSB),
	// The plugin now knows that the next MidiControllerDataEntry message is meant to set the MPE config
	// The channel of the message in this case determines whether it is for the Lower or Upper zone (0 = lower, 15 = upper)
	proc->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerRegisteredParameterNumberMSB, (MidiMPEConfigurationRPN >> 8) & 0x7F));
	proc->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerRegisteredParameterNumberLSB, MidiMPEConfigurationRPN & 0x7F));
	proc->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerDataEntry, m_numChannelsLowerZone));
	// Send null RPN signals to end the control change. This is not explicitly required by the MPE spec, but I heard it was good practice to prevent accidental signals from editing the last sent config
	proc->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerRegisteredParameterNumberMSB, (MidiNullFunctionNumberRPN >> 8) & 0x7F));
	proc->processOutEvent(MidiEvent(MidiControlChange, 0, MidiControllerRegisteredParameterNumberLSB, MidiNullFunctionNumberRPN & 0x7F));
	// Send the same signals, but for the upper zone.
	// TODO section 2.2.1, if a sender intends to only use one zone, it should only send the config message for that one zone. This is not mandatory, but perhaps in the future we should implement that.
	proc->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerRegisteredParameterNumberMSB, (MidiMPEConfigurationRPN >> 8) & 0x7F));
	proc->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerRegisteredParameterNumberLSB, MidiMPEConfigurationRPN & 0x7F));
	proc->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerDataEntry, m_numChannelsUpperZone));

	proc->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerRegisteredParameterNumberMSB, (MidiNullFunctionNumberRPN >> 8) & 0x7F));
	proc->processOutEvent(MidiEvent(MidiControlChange, 0xF, MidiControllerRegisteredParameterNumberLSB, MidiNullFunctionNumberRPN & 0x7F));

	// Set pitch bend range on all Member channels.
	// The manager channels are untouched, since those are controlled by the instrument track's pitch knob.
	// Lower zone
	for (int channel = 1; channel <= m_numChannelsLowerZone; ++channel)
	{
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberMSB, (MidiPitchBendSensitivityRPN >> 8) & 0x7F));
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberLSB, MidiPitchBendSensitivityRPN & 0x7F));
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerDataEntry, m_pitchBendRange));
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberMSB, (MidiNullFunctionNumberRPN >> 8) & 0x7F));
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberLSB, MidiNullFunctionNumberRPN & 0x7F));
	}
	// Upper zone
	for (int channel = 15 - m_numChannelsUpperZone; channel <= 14; ++channel)
	{
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberMSB, (MidiPitchBendSensitivityRPN >> 8) & 0x7F));
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberLSB, MidiPitchBendSensitivityRPN & 0x7F));
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerDataEntry, m_pitchBendRange));
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberMSB, (MidiNullFunctionNumberRPN >> 8) & 0x7F));
		proc->processOutEvent(MidiEvent(MidiControlChange, channel, MidiControllerRegisteredParameterNumberLSB, MidiNullFunctionNumberRPN & 0x7F));
	}

	// And reset the pitch bend values so that they don't get stuck after disabling MPE.
	// Sending on all channels 0-15 rather than 1-14, since the lower zone can sometimes extend to channel 15 if the upper zone is inactive, and vice versa.
	// Technically, this should not be necessary, since according to 2.2.3, the reciever should handle resetting the values when a channel leaves/enters an MPE zone. But some vst's (vital) don't appear to do that, so we still do it for compatability.
	for (int channel = 0; channel < 16; ++channel)
	{
		// Pitch bend values go from 0 to 16383, so the zero point is 8192.
		proc->processOutEvent(MidiEvent(MidiPitchBend, channel, 8192));
	}
}


} // namespace lmms
