/*
 * AudioBufferTest.cpp
 *
 * Copyright (c) 2026 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "AudioBuffer.h"

#include <QObject>
#include <QtTest>
#include <algorithm>
#include <iostream>
#include <numeric>

#include "MixHelpers.h"

using lmms::AudioBuffer;

class AudioBufferTest : public QObject
{
	Q_OBJECT

private slots:
	//! Verifies constructor with default channels adds single stereo group
	void Constructor_DefaultChannels()
	{
		auto tc = AudioBuffer{10};
		QCOMPARE(tc.groupCount(), 1);
		QCOMPARE(tc.group(0).channels(), 2);
		QCOMPARE(tc.totalChannels(), 2);
		QCOMPARE(tc.frames(), 10);
	}

	//! Verifies constructor with no channels does not create a first group
	void Constructor_NoChannels()
	{
		auto tc = AudioBuffer{10, 0};
		QCOMPARE(tc.groupCount(), 0);
		QCOMPARE(tc.totalChannels(), 0);
		QCOMPARE(tc.frames(), 10);
	}

	//! Verifies that the `addGroup` method can add the first group correctly
	void AddGroup_FirstGroup()
	{
		// Begin with zero groups
		auto tc = AudioBuffer{10, 0};

		// Add a first group with 5 channels
		auto group = tc.addGroup(5);
		QVERIFY(group != nullptr);
		QCOMPARE(&tc.group(0), group);
		QCOMPARE(group->channels(), 5);
		QCOMPARE(tc.groupCount(), 1);
		QCOMPARE(tc.totalChannels(), 5);
	}

	//! Verifies that a 2nd group can be appended after the 1st group
	void AddGroup_SecondGroup()
	{
		// Begin with 1 group
		auto tc = AudioBuffer{10, 3};

		// Add a 2nd group with 4 channels
		auto group = tc.addGroup(4);
		QVERIFY(group != nullptr);
		QCOMPARE(&tc.group(1), group);
		QCOMPARE(group->channels(), 4);
		QCOMPARE(tc.groupCount(), 2);
		QCOMPARE(tc.totalChannels(), 7);
	}

	//! Verifies that a group with 0 channels cannot be added and doing so has no effect
	void AddGroup_ZeroChannelsFails()
	{
		auto tc = AudioBuffer{10};
		QCOMPARE(tc.groupCount(), 1);
		QCOMPARE(tc.totalChannels(), 2);

		auto group = tc.addGroup(0);
		QCOMPARE(group, nullptr);

		// Nothing should have changed
		QCOMPARE(tc.groupCount(), 1);
		QCOMPARE(tc.totalChannels(), 2);
	}

	//! Verifies that a group with the max channel count can be added
	void AddGroup_MaximumChannelsInGroup()
	{
		auto tc = AudioBuffer{10};

		auto group = tc.addGroup(lmms::MaxChannelsPerGroup);
		QVERIFY(group != nullptr);
		QCOMPARE(group->channels(), lmms::MaxChannelsPerGroup);
		QCOMPARE(tc.groupCount(), 2);
		QCOMPARE(tc.totalChannels(), 2 + lmms::MaxChannelsPerGroup);
	}

	//! Verifies that a group with over the max channel count cannot
	//! be added and doing so has no effect
	void AddGroup_TooManyChannels()
	{
		auto tc = AudioBuffer{10};
		QCOMPARE(tc.groupCount(), 1);
		QCOMPARE(tc.totalChannels(), 2);

		auto group = tc.addGroup(lmms::MaxChannelsPerGroup + 1);
		QCOMPARE(group, nullptr);
		QCOMPARE(tc.groupCount(), 1);
		QCOMPARE(tc.totalChannels(), 2);
	}

	//! Verifies that groups cannot be added past the maximum group count
	void AddGroup_MaximumGroups()
	{
		auto tc = AudioBuffer{10, 0};

		// Add groups until no more can be added
		auto groupsLeft = static_cast<int>(lmms::MaxGroupsPerTrack);
		QVERIFY(groupsLeft >= 0);
		while (groupsLeft > 0)
		{
			auto temp = tc.addGroup(1);
			QVERIFY(temp != nullptr);
			--groupsLeft;
		}
		QCOMPARE(groupsLeft, 0);
		QCOMPARE(tc.groupCount(), lmms::MaxGroupsPerTrack);
		QCOMPARE(tc.totalChannels(), lmms::MaxGroupsPerTrack);

		// Next group should fail
		auto group = tc.addGroup(1);
		QCOMPARE(group, nullptr);
		QCOMPARE(tc.groupCount(), lmms::MaxGroupsPerTrack);
		QCOMPARE(tc.totalChannels(), lmms::MaxGroupsPerTrack);
	}

	//! Verifies that groups cannot be added past the maximum total channel count for the track
	void AddGroup_MaximumTotalChannels()
	{
		auto tc = AudioBuffer{10, 0};

		static_assert(lmms::MaxChannelsPerTrack % lmms::MaxChannelsPerGroup == 0, "Need to update test");
		auto groupsLeft = lmms::MaxChannelsPerTrack / lmms::MaxChannelsPerGroup;
		auto channelsLeft = lmms::MaxChannelsPerTrack;

		// Add all but the last group
		while (groupsLeft > 1)
		{
			auto group = tc.addGroup(lmms::MaxChannelsPerGroup);
			QVERIFY(group != nullptr);
			--groupsLeft;
			channelsLeft -= lmms::MaxChannelsPerGroup;
		}
		QCOMPARE(groupsLeft, 1);
		QCOMPARE(channelsLeft, lmms::MaxChannelsPerGroup);

		// Add one fewer than the max channels for the last group
		auto group = tc.addGroup(lmms::MaxChannelsPerGroup - 1);
		QVERIFY(group != nullptr);
		QCOMPARE(tc.totalChannels(), lmms::MaxChannelsPerTrack - 1);

		// Ok, now try adding a group with enough channels
		// to push the total channels past the maximum for the track (should fail)
		group = tc.addGroup(2);
		QCOMPARE(group, nullptr);
		QCOMPARE(tc.totalChannels(), lmms::MaxChannelsPerTrack - 1);

		// Ok, how about just enough to hit the maximum
		// total channels for the track (should succeed)
		group = tc.addGroup(1);
		QVERIFY(group != nullptr);
		QCOMPARE(tc.totalChannels(), lmms::MaxChannelsPerTrack);
	}

	//! Verifies all silence flag bits are set when there are no channels
	void SilenceFlags_AllSilentWhenNoChannels()
	{
		auto tc = AudioBuffer{10, 0};
		QCOMPARE(tc.silenceFlags().all(), true);
	}

	//! Verifies all silence flags bits are set even after adding new groups/channels
	void SilenceFlags_AllSilentWhenNewGroupsAdded()
	{
		auto tc = AudioBuffer{10};
		QCOMPARE(tc.silenceFlags().all(), true);

		tc.addGroup(4);
		QCOMPARE(tc.silenceFlags().all(), true);
	}

	//! Verifies that `assumeNonSilent` clears a specific bit in the silence flags
	void AssumeNonSilent()
	{
		auto tc = AudioBuffer{10, 2};
		QCOMPARE(tc.silenceFlags().all(), true);

		// Assume 2nd channel is non-silent
		tc.assumeNonSilent(1);

		QCOMPARE(tc.silenceFlags().all(), false);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);
	}

	//! Verifies `enableSilenceTracking` enables and disables silence tracking
	void EnableSilenceTracking_GetterSetter()
	{
		auto tc = AudioBuffer{10};
		tc.enableSilenceTracking(true);
		QCOMPARE(tc.silenceTrackingEnabled(), true);

		tc.enableSilenceTracking(false);
		QCOMPARE(tc.silenceTrackingEnabled(), false);
	}

	//! Verifies that `enableSilenceTracking(true)` also updates silence flags
	void EnableSilenceTracking_UpdatesSilenceFlags()
	{
		auto tc = AudioBuffer{10, 2};
		tc.enableSilenceTracking(false);

		// Assume 2nd channel is non-silent
		tc.assumeNonSilent(1);

		QCOMPARE(tc.silenceFlags().all(), false);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);

		tc.enableSilenceTracking(true);

		// Silence flags should be updated
		QCOMPARE(tc.silenceFlags().all(), true);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], true);
	}

	//! Verifies that the `updateSilenceFlags` method does nothing to silence flags
	//! when all channels are already silent, regardless of which channels are selected
	//! for an update.
	void UpdateSilenceFlags_DoesNothingWhenSilent()
	{
		auto tc = AudioBuffer{10};
		tc.enableSilenceTracking(true);

		QCOMPARE(tc.silenceFlags().all(), true);

		// Right channel only
		QCOMPARE(tc.updateSilenceFlags(0b01), true);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Left channel only
		QCOMPARE(tc.updateSilenceFlags(0b10), true);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Both channels
		QCOMPARE(tc.updateSilenceFlags(0b11), true);
		QCOMPARE(tc.silenceFlags().all(), true);
	}

	//! Verifies that the `updateSilenceFlags` method updates a single non-silent channel,
	//! but only when that channel is selected for an update.
	void UpdateSilenceFlags_UpdatesChannelWhenSelected()
	{
		auto tc = AudioBuffer{10};
		tc.enableSilenceTracking(true);

		// Both channels should be silent
		QCOMPARE(tc.silenceFlags().all(), true);

		// Introduce noise to a frame in the right channel
		tc.group(0).buffer(1)[5] = 1.f;

		// Update the left channel - returns true because the updated channel is silent
		QCOMPARE(tc.updateSilenceFlags(0b01), true);

		// Silence flags remain the same since the non-silent channel was not updated
		QCOMPARE(tc.silenceFlags().all(), true);

		// Now update the right channel - returns false since the updated channel is not silent
		QCOMPARE(tc.updateSilenceFlags(0b10), false);

		// The silence flag for the right channel should now be cleared
		QCOMPARE(tc.silenceFlags()[0], true);  // left channel
		QCOMPARE(tc.silenceFlags()[1], false); // right channel
		QCOMPARE(tc.silenceFlags()[2], true);  // unused 3rd channel
		// etc.

		// Updating both channels returns false since one of them is non-silent
		QCOMPARE(tc.updateSilenceFlags(0b11), false);
	}

	//! Verifies that the `updateSilenceFlags` method works across channel groups
	void UpdateSilenceFlags_WorksWithGroups()
	{
		auto tc = AudioBuffer{10, 0};
		tc.enableSilenceTracking(true);

		tc.addGroup(3);
		tc.addGroup(1);

		// All channels should be silent
		QCOMPARE(tc.silenceFlags().all(), true);

		// Introduce noise to a frame in the 2nd channel of the 1st group
		tc.group(0).buffer(1)[5] = 1.f;

		// Introduce noise to a frame in the 1st channel of the 2nd group
		tc.group(1).buffer(0)[5] = 1.f;

		// Update the two silent channels - returns true because both are silent
		QCOMPARE(tc.updateSilenceFlags(0b0101), true);

		// Silence flags remain the same since the non-silent channels were not updated
		QCOMPARE(tc.silenceFlags().all(), true);

		// Now update the 3rd channel of the 1st group and the 1st channel of the 2nd group
		// Returns false since one of the updated channels is not silent
		QCOMPARE(tc.updateSilenceFlags(0b1100), false);

		// The silence flag for the 1st channel of the 2nd group should now be cleared,
		// but the 2nd channel of the 1st group should still be marked silent since
		// it has not been updated yet.
		QCOMPARE(tc.silenceFlags()[0], true);  // group 1, channel 1
		QCOMPARE(tc.silenceFlags()[1], true);  // group 1, channel 2
		QCOMPARE(tc.silenceFlags()[2], true);  // group 1, channel 3
		QCOMPARE(tc.silenceFlags()[3], false); // group 2, channel 1
		QCOMPARE(tc.silenceFlags()[4], true);  // unused 5th channel
		// etc.

		// Now update group 1, channel 2
		QCOMPARE(tc.updateSilenceFlags(0b0010), false);

		QCOMPARE(tc.silenceFlags()[0], true);  // group 1, channel 1
		QCOMPARE(tc.silenceFlags()[1], false); // group 1, channel 2
		QCOMPARE(tc.silenceFlags()[2], true);  // group 1, channel 3
		QCOMPARE(tc.silenceFlags()[3], false); // group 2, channel 1
		QCOMPARE(tc.silenceFlags()[4], true);  // unused 5th channel
		// etc.
	}

	//! Verifies that the `updateSilenceFlags` method updates a silent channel's flags
	//! from non-silent to silent when selected for update.
	void UpdateSilenceFlags_UpdatesFromNonSilenceToSilence()
	{
		auto tc = AudioBuffer{10, 2};
		tc.enableSilenceTracking(true);

		QCOMPARE(tc.silenceFlags().all(), true);

		// Assume 2nd channel is non-silent
		tc.assumeNonSilent(1);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);

		// Update 1st channel - does nothing
		QCOMPARE(tc.updateSilenceFlags(0b01), true);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);
		QCOMPARE(tc.silenceFlags()[2], true); // unused 3rd channel

		// Update 2nd channel (non-silent to silent)
		// Returns true because the channel's audio data is silent
		QCOMPARE(tc.updateSilenceFlags(0b10), true);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], true); // unused 3rd channel

		// Again, assume 2nd channel is non-silent
		tc.assumeNonSilent(1);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);
		QCOMPARE(tc.silenceFlags()[2], true); // unused 3rd channel

		// Update both channels
		// Returns true because both channels' audio data is silent
		QCOMPARE(tc.updateSilenceFlags(0b11), true);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], true); // unused 3rd channel
	}

	//! Verifies that `updateSilenceFlags` marks selected channels as non-silent when
	//! silence tracking is disabled.
	void UpdateSilenceFlags_NonSilentWhenSilenceTrackingDisabled()
	{
		auto tc = AudioBuffer{10, 2};
		tc.enableSilenceTracking(false);

		QCOMPARE(tc.silenceFlags().all(), true);

		// Now update the 2nd channel. The audio data is actually silent, but silence tracking
		// is disabled so it must assume the updated channel is non-silent just to be safe.
		QCOMPARE(tc.updateSilenceFlags(0b10), false);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);
		QCOMPARE(tc.silenceFlags()[2], true); // unused 3rd channel

		// Again with both channels
		QCOMPARE(tc.updateSilenceFlags(0b11), false);
		QCOMPARE(tc.silenceFlags()[0], false);
		QCOMPARE(tc.silenceFlags()[1], false);
		QCOMPARE(tc.silenceFlags()[2], true); // unused 3rd channel
	}

	//! Verifies that when no selected channels are passed to `updateSilenceFlags`,
	//! it returns true indicating that all selected channels are silent.
	void UpdateSilenceFlags_NoSelectionIsSilent()
	{
		auto tc = AudioBuffer{10, 2};

		// First, with silence tracking
		tc.enableSilenceTracking(true);
		QCOMPARE(tc.updateSilenceFlags(0), true);

		// Should produce the same result without silence tracking
		tc.enableSilenceTracking(false);
		QCOMPARE(tc.updateSilenceFlags(0), true);
	}

	//! Verifies that `updateAllSilenceFlags` updates all silence flags
	//! when silence tracking is enabled.
	void UpdateAllSilenceFlags_SilenceTrackingEnabled()
	{
		auto tc = AudioBuffer{10, 2};
		tc.addGroup(2);
		tc.enableSilenceTracking(true);

		QCOMPARE(tc.updateAllSilenceFlags(), true);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Introduce noise to a frame in the 1st channel of the 1st group
		tc.group(0).buffer(0)[5] = 1.f;

		// Introduce noise to a frame in the 2nd channel of the 2nd group
		tc.group(1).buffer(1)[5] = 1.f;

		// Those 2 channels should be marked silent after updating all channels
		QCOMPARE(tc.updateAllSilenceFlags(), false);
		QCOMPARE(tc.silenceFlags()[0], false); // channel 1
		QCOMPARE(tc.silenceFlags()[1], true);  // channel 2
		QCOMPARE(tc.silenceFlags()[2], true);  // channel 3
		QCOMPARE(tc.silenceFlags()[3], false); // channel 4
		QCOMPARE(tc.silenceFlags()[4], true);  // unused 5th channel

		// Silence the frame in the 2nd channel of the 2nd group
		tc.group(1).buffer(1)[5] = 0.f;

		// Now only the 1st channel should be marked silent after updating all channels
		QCOMPARE(tc.updateAllSilenceFlags(), false);
		QCOMPARE(tc.silenceFlags()[0], false); // channel 1
		QCOMPARE(tc.silenceFlags()[1], true);  // channel 2
		QCOMPARE(tc.silenceFlags()[2], true);  // channel 3
		QCOMPARE(tc.silenceFlags()[3], true);  // channel 4
		QCOMPARE(tc.silenceFlags()[4], true);  // unused 5th channel
	}

	//! Verifies that `updateAllSilenceFlags` marks all silence flags
	//! for used channels as non-silent when silence tracking is disabled.
	void UpdateAllSilenceFlags_SilenceTrackingDisabled()
	{
		auto tc = AudioBuffer{10, 2};
		tc.addGroup(2);
		tc.enableSilenceTracking(false);

		QCOMPARE(tc.updateAllSilenceFlags(), false);

		QCOMPARE(tc.silenceFlags()[0], false); // channel 1
		QCOMPARE(tc.silenceFlags()[1], false); // channel 2
		QCOMPARE(tc.silenceFlags()[2], false); // channel 3
		QCOMPARE(tc.silenceFlags()[3], false); // channel 4
		QCOMPARE(tc.silenceFlags()[4], true);  // unused 5th channel
	}

	//! Verifies that when there are no channels, `updateAllSilenceFlags`
	//! returns true indicating that all channels are silent.
	void UpdateAllSilenceFlags_NoChannelsIsSilent()
	{
		auto tc = AudioBuffer{10, 0};

		// First, with silence tracking
		tc.enableSilenceTracking(true);
		QCOMPARE(tc.updateAllSilenceFlags(), true);

		// Should produce the same result without silence tracking
		tc.enableSilenceTracking(false);
		QCOMPARE(tc.updateAllSilenceFlags(), true);
	}

	//! Verifies that `hasInputNoise` returns true if any of the selected
	//! channels are non-silent.
	void HasInputNoise()
	{
		auto tc = AudioBuffer{10, 2};
		tc.enableSilenceTracking(true);

		// Add a 2nd stereo group
		QVERIFY(tc.addGroup(2) != nullptr);

		// No input noise since all channels are silent
		QCOMPARE(tc.hasInputNoise(0b1111), false);

		// Assume both left channels are non-silent
		tc.assumeNonSilent(0);
		tc.assumeNonSilent(2);

		// Check if any channels are non-silent
		QCOMPARE(tc.hasInputNoise(0b1111), true);

		// Check if either of the left channels are non-silent
		QCOMPARE(tc.hasInputNoise(0b0101), true);

		// Check if either of the right channels are non-silent
		QCOMPARE(tc.hasInputNoise(0b1010), false);

		// Check if either channel in the 1st group are non-silent
		QCOMPARE(tc.hasInputNoise(0b0011), true);

		// Check if the 5th channel (an unused channel) is non-silent
		QCOMPARE(tc.hasInputNoise(0b10000), false);
	}

	//! Verifies that the `sanitize` method only silences channels containing Inf or NaN
	void Sanitize_SilencesOnlyInfAndNaN()
	{
		lmms::MixHelpers::setNaNHandler(true);

		auto tc = AudioBuffer{10, 2};
		tc.enableSilenceTracking(true);

		// Add a 2nd stereo group
		QVERIFY(tc.addGroup(2) != nullptr);

		// Should have no effect when all buffers are silenced
		QCOMPARE(tc.silenceFlags().all(), true);
		tc.sanitize(0b1111);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Make left channel of 1st channel group
		// contain an Inf, and force the channel to non-silent
		tc.group(0).buffer(0)[5] = std::numeric_limits<float>::infinity();
		tc.assumeNonSilent(0);

		// Make right channel of 1st channel group non-silent too,
		// but using a valid value
		tc.group(0).buffer(1)[5] = 1.f;
		tc.assumeNonSilent(1);

		// Sanitize only the left channel
		tc.sanitize(0b0001);

		// The left channel's buffer should be silenced,
		// while the right channel should be unaffected
		QCOMPARE(tc.group(0).buffer(0)[5], 0.f);
		QCOMPARE(tc.group(0).buffer(1)[5], 1.f);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);
		QCOMPARE(tc.silenceFlags()[2], true);
		QCOMPARE(tc.silenceFlags()[3], true);
		QCOMPARE(tc.silenceFlags()[4], true); // unused 5th channel

		// Try again
		tc.group(0).buffer(0)[5] = std::numeric_limits<float>::quiet_NaN();
		tc.assumeNonSilent(0);

		// This time, sanitize both channels of the 1st channel group
		tc.sanitize(0b0011);

		// Again, the left channel's buffer should be silence,
		// while the right channel should be unaffected
		QCOMPARE(tc.group(0).buffer(0)[5], 0.f);
		QCOMPARE(tc.group(0).buffer(1)[5], 1.f);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);
		QCOMPARE(tc.silenceFlags()[2], true);
		QCOMPARE(tc.silenceFlags()[3], true);
		QCOMPARE(tc.silenceFlags()[4], true); // unused 5th channel
	}

	//! Verifies that the `silenceChannels` method silences the selected channels
	//! and updates their silence flags
	void SilenceChannels_SilencesSelectedChannels()
	{
		auto tc = AudioBuffer{10, 2};
		tc.enableSilenceTracking(true);

		// Add a 2nd stereo group
		QVERIFY(tc.addGroup(2) != nullptr);

		// Should have no effect when all buffers are silent
		QCOMPARE(tc.silenceFlags().all(), true);
		tc.silenceChannels(0b1111);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Make left channel of 2nd channel group contain
		// a non-silent value, and force the channel to be non-silent
		tc.group(1).buffer(0)[5] = 1.f;
		tc.assumeNonSilent(2);

		// Silence only the left channel
		tc.silenceChannels(0b0100);

		QCOMPARE(tc.silenceFlags()[0], true); // not selected
		QCOMPARE(tc.silenceFlags()[1], true); // not selected
		QCOMPARE(tc.silenceFlags()[2], true); // updated!
		QCOMPARE(tc.silenceFlags()[3], true); // not selected

		// Make right channel of 2nd channel group contain
		// a non-silent value, and force the channel to be non-silent
		tc.group(1).buffer(1)[5] = 1.f;
		tc.assumeNonSilent(3);

		// Silence only the right channel
		tc.silenceChannels(0b1000);

		QCOMPARE(tc.silenceFlags()[0], true); // not selected
		QCOMPARE(tc.silenceFlags()[1], true); // not selected
		QCOMPARE(tc.silenceFlags()[2], true); // not selected
		QCOMPARE(tc.silenceFlags()[3], true); // updated!

		// Make right channel of 1st channel group and
		// both channels of 2nd channel group contain
		// a non-silent value, and force those channels to be non-silent
		tc.group(1).buffer(1)[5] = 1.f;
		tc.assumeNonSilent(1);
		tc.group(1).buffer(0)[5] = 1.f;
		tc.assumeNonSilent(2);
		tc.group(1).buffer(1)[5] = 1.f;
		tc.assumeNonSilent(3);

		// Silence both channels of the 2nd channel group,
		// plus the already-silent left channel of the 1st group
		tc.silenceChannels(0b1101);

		QCOMPARE(tc.silenceFlags()[0], true);  // selected, but already silent
		QCOMPARE(tc.silenceFlags()[1], false); // not selected, remains non-silent
		QCOMPARE(tc.silenceFlags()[2], true);  // updated!
		QCOMPARE(tc.silenceFlags()[3], true);  // updated!
	}
};

QTEST_GUILESS_MAIN(AudioBufferTest)
#include "AudioBufferTest.moc"
