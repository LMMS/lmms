/*
* MPETest.cpp
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

#include <QtTest>
#include "MPEManager.h"
#include "MidiEventProcessor.h"


class DummyMidiEventProcessor : public lmms::MidiEventProcessor
{
public:
	void processInEvent(const lmms::MidiEvent& event, const lmms::TimePos& time, lmms::f_cnt_t offset) override {};
	void processOutEvent(const lmms::MidiEvent& event, const lmms::TimePos& time, lmms::f_cnt_t offset) override
	{
		m_eventHistory.push_back(event);
	}
	std::vector<lmms::MidiEvent> m_eventHistory;
};





class MPETest : public QObject
{
	Q_OBJECT
private slots:
	/** @brief Checks that notes are routed to the channels with the least number of active notes, or the channel with the oldest NoteOff in the case of a tie.
	 */
	void mpeChannelRoutingTest()
	{
		using namespace lmms;

		MPEManager mpeManager;

		//
		// LOWER ZONE
		//

		// Setup lower zone to use first 5 channels (0 is the manager channel, 1-4 are member channels), 0 channels for the upper zone, 48 semitone pitch bend range, and use the lower zone.
		mpeManager.config(5, 0, 48, MPEManager::Zone::Lower);

		// Currently, the active note counts should be 0 on all channels (bracketed are the member channels)
		// 0 [0 0 0 0] 0 0 0 0 0 0 0 0 0 0 0

		// "Send" noteOn events to some of the channels
		mpeManager.noteOn(1);
		mpeManager.noteOn(1);
		mpeManager.noteOn(1);
		mpeManager.noteOn(2);
		mpeManager.noteOn(2);
		mpeManager.noteOn(2);
		mpeManager.noteOn(2);
		mpeManager.noteOn(3);
		mpeManager.noteOn(3);
		// Now, the active note counts should look like:
		// 0 [3 4 2 0] 0 0 0 0 0 0 0 0 0 0 0
		// Ignoring the first channel (manager channel), the next active note should be spawned in channel 4, since it has the least notes.
		QCOMPARE(mpeManager.findAvailableChannel(), 4);


		// Add some notes to channel 4
		mpeManager.noteOn(4);
		mpeManager.noteOn(4);
		mpeManager.noteOn(4);

		// The note counts should look like this:
		// 0 [3 4 2 3] 0 0 0 0 0 0 0 0 0 0 0
		// Now channel 3 has the fewest active notes
		QCOMPARE(mpeManager.findAvailableChannel(), 3);

		// Release a few of the notes
		mpeManager.noteOff(2);
		mpeManager.noteOff(2);
		mpeManager.noteOff(2);
		mpeManager.noteOff(1);
		mpeManager.noteOff(1);
		mpeManager.noteOff(3);
		mpeManager.noteOff(4);
		mpeManager.noteOff(4);
		// The note counts should look like this:
		// 0 [1 1 1 1] 0 0 0 0 0 0 0 0 0 0 0

		// According to the MPE specification best practice (page 17, A.3), if channels are tied for the number of active notes, the note should be routed to the channel with the oldest noteOff signal.
		// In this case, channel 2 released its notes first, so the new note should be routed there
		QCOMPARE(mpeManager.findAvailableChannel(), 2);



		//
		// UPPER ZONE
		//

		mpeManager.config(0, 5, 48, MPEManager::Zone::Upper);

		// Currently, the active note counts should be 0 on all channels
		// 0 0 0 0 0 0 0 0 0 0 0 [0 0 0 0] 0

		// "Send" noteOn events to some of the channels
		mpeManager.noteOn(11);
		mpeManager.noteOn(11);
		mpeManager.noteOn(11);
		mpeManager.noteOn(12);
		mpeManager.noteOn(12);
		mpeManager.noteOn(12);
		mpeManager.noteOn(13);
		mpeManager.noteOn(13);
		// Now, the active note counts should look like:
		// 0 0 0 0 0 0 0 0 0 0 0 [3 3 2 0] 0
		// Ignoring the last channel (manager channel), the next active note should be spawned in channel 14, since it has the least notes.
		QCOMPARE(mpeManager.findAvailableChannel(), 14);


		// Add some notes to channel 14
		mpeManager.noteOn(14);
		mpeManager.noteOn(14);
		mpeManager.noteOn(14);

		// The note counts should look like this:
		// 0 0 0 0 0 0 0 0 0 0 0 [3 3 2 3] 0
		// Now channel 13 has the fewest active notes
		QCOMPARE(mpeManager.findAvailableChannel(), 13);

		// Release a few of the notes
		mpeManager.noteOff(12);
		mpeManager.noteOff(12);
		mpeManager.noteOff(13);
		mpeManager.noteOff(14);
		mpeManager.noteOff(14);
		mpeManager.noteOff(11);
		mpeManager.noteOff(11);
		// The note counts should look like this:
		// 0 0 0 0 0 0 0 0 0 0 0 [1 1 1 1] 0

		// Channel 12 released its notes first, so the new note should be routed there
		QCOMPARE(mpeManager.findAvailableChannel(), 12);
	}

	/** @brief Checks edge cases with zone channel counts to ensure it handles them properly
	 */
	void mpeZoneChannelCountTest()
	{
		using namespace lmms;
		MPEManager mpeManager;

		// With two channels in total, the first is manager, and the second is the only member channel. Therefore, all notes should be routed through the member channel.
		mpeManager.config(2, 0, 48, MPEManager::Zone::Lower);
		// Channel 0 is manager, channel 1 is member
		QCOMPARE(mpeManager.findAvailableChannel(), 1);

		// Same thing for upper zone
		mpeManager.config(0, 2, 48, MPEManager::Zone::Upper);
		// Channel 15 is manager, channel 14 is member
		QCOMPARE(mpeManager.findAvailableChannel(), 14);

		// With no channels on a zone, the output channel should be std::nullopt, signifying that it should be routed to some default channel.
		mpeManager.config(0, 16, 48, MPEManager::Zone::Lower);
		QCOMPARE(mpeManager.findAvailableChannel(), std::nullopt);
		mpeManager.config(16, 0, 48, MPEManager::Zone::Upper);
		QCOMPARE(mpeManager.findAvailableChannel(), std::nullopt);

		// Likewise, if there is only one channel (the master channel), there are no member channels, so the mpe spec says the zone should deactivate (Page 23, B.6).
		mpeManager.config(1, 15, 48, MPEManager::Zone::Lower);
		QCOMPARE(mpeManager.findAvailableChannel(), std::nullopt);
		mpeManager.config(15, 1, 48, MPEManager::Zone::Upper);
		QCOMPARE(mpeManager.findAvailableChannel(), std::nullopt);
	}

	/** @brief Ensures MPE configuration messages are sent correctly
	 */
	void mpeConfigurationTest()
	{
		using namespace lmms;
		MPEManager mpeManager;
		DummyMidiEventProcessor proc;

		// Set some random channel amounts and pitch bend range
		int lowerZoneChannels = 7;
		int upperZoneChannels = 5;
		int pitchBendRange = 54;
		mpeManager.config(lowerZoneChannels, upperZoneChannels, pitchBendRange);
		mpeManager.sendMPEConfigSignals(&proc);

		//
		// Loop through the events, pretending to be a plugin receiving them and updating its internal state, then ensure the end state is correct.
		//
		int receivedLowerZoneChannels = -1, receivedUpperZoneChannels = -1;
		// Technically the pitch bend range can be different on different zones, but for now we set the same range for both.
		std::array<int, 16> receivedPitchBendRanges = {};
		receivedPitchBendRanges.fill(-1);
		std::array<int, 16> receivedPitchBends = {};
		receivedPitchBends.fill(-1);
		// The config/range is sent via controller RPNs (Registered Parameter Numbers)
		// The id of the RPN is sent in two steps:
		// -- The first 7 bits are sent (Most Significant Bits / MSB) with a MidiControlChange event, where param0 = 101 and param1 = the first 7 bits
		// -- The last 7 bits are sent (Least Significant Bits / LSB) with a MidiControlChange event, where param0 = 100 and param1 = the last 7 bits
		// (Those numbers, 101 and 100, are standardized values in the specification for this purpose)
		// Now that the plugin knows what RPN the sender is talking about, it waits for the next MidiControlChange events to send to actual value.
		// Depending on the parameter, two messages may be sent to specify both fine and coarse values, or just one. In our case, we will only be setting the coarse value.
		// -- The value is sent via another MidiControlChange event, with param0 = 6, and param1 = the value
		// (And once again, 6 is a standardized value for setting the coarse value)
		// I also heard it was good practice to send 0x7F as the MSB and LSB after finishing the config, to prevent an accidental controller event from changing something
		std::array<int, 16> LSBValues = {};
		std::array<int, 16> MSBValues = {};
		// Keep track of which channesl we have sent RPN signals to, so we can make sure we reset the MSB/LSB's at the end.
		std::array<bool, 16> touchedChannels = {};

		for (auto it = proc.m_eventHistory.begin(); it != proc.m_eventHistory.end(); ++it)
		{
			MidiEvent& event = *it;
			uint8_t channel = event.channel();
			QVERIFY(channel >= 0 && channel < 16);

			switch (event.type())
			{
				case MidiControlChange:
					switch (event.param(0))
					{
						case MidiControllerRegisteredParameterNumberMSB:
							MSBValues[channel] = event.param(1);
							touchedChannels[channel] = true;
							break;
						case MidiControllerRegisteredParameterNumberLSB:
							LSBValues[channel] = event.param(1);
							touchedChannels[channel] = true;
							break;
						case MidiControllerDataEntry:
							if ((MSBValues[channel] << 8) + LSBValues[channel] == MidiMPEConfigurationRPN)
							{
								// The MPE config message should only be sent on either the first or last channel
								QVERIFY(channel == 0x0 || channel == 0xF);
								if (channel == 0x0) { receivedLowerZoneChannels = event.param(1); }
								if (channel == 0xF) { receivedUpperZoneChannels = event.param(1); }
							}
							else if ((MSBValues[channel] << 8) + LSBValues[channel] == MidiPitchBendSensitivityRPN)
							{
								receivedPitchBendRanges[channel] = event.param(1);
							}
							break;
						default:
							QFAIL("MidiControlChange sent which was not MSB, LSB, or Coarse Data Entry");
							break;
					}
					break;
				case MidiPitchBend:
					receivedPitchBends[channel] = event.pitchBend();
					break;
				default:
					QFAIL("MIDI Event sent during configuration which was not MidiControlChange or MidiPitchBend");
					break;
			}
		}

		//
		// Confirm the current state is correct
		//
		QCOMPARE(receivedLowerZoneChannels, lowerZoneChannels);
		QCOMPARE(receivedUpperZoneChannels, upperZoneChannels);
		// We currently only set the pitch bend range on member channels, not the manager channels, since that's supposed to be handled by the pitch bend range knob.
		for (int channel = 1; channel <= receivedLowerZoneChannels; ++channel)
		{
			QCOMPARE(receivedPitchBendRanges[channel], pitchBendRange);
		}
		for (int channel = 15 - receivedUpperZoneChannels; channel <= 14; ++channel)
		{
			QCOMPARE(receivedPitchBendRanges[channel], pitchBendRange);
		}
		// Currently we also reset the pitch bends when changing the MPE config. This is not required for the sender in the specification, but it can prevent some bugs with plugins which don't automatically reset it themselves.
		for (int bend: receivedPitchBends)
		{
			// Pitch bend values go from 0 to 16383, so the zero point is 8192.
			QCOMPARE(bend, 8192);
		}
		// And just for good practice, make sure the MSB/LSB's are set to null at the end for all the channels we touched
		for (int channel = 0; channel < 16; ++channel)
		{
			if (touchedChannels[channel])
			{
				QCOMPARE(MSBValues[channel], (MidiNullFunctionNumberRPN >> 8) & 0x7F);
				QCOMPARE(LSBValues[channel], MidiNullFunctionNumberRPN & 0x7F);
			}
		}
	}
};

QTEST_GUILESS_MAIN(MPETest)
#include "MPETest.moc"
