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
		auto ab = AudioBuffer{10};
		QCOMPARE(ab.groupCount(), 1);
		QCOMPARE(ab.group(0).channels(), 2);
		QCOMPARE(ab.totalChannels(), 2);
		QCOMPARE(ab.frames(), 10);
	}

	//! Verifies constructor with no channels does not create a first group
	void Constructor_NoChannels()
	{
		auto ab = AudioBuffer{10, 0};
		QCOMPARE(ab.groupCount(), 0);
		QCOMPARE(ab.totalChannels(), 0);
		QCOMPARE(ab.frames(), 10);
	}

	//! Verifies that the `addGroup` method can add the first group correctly
	void AddGroup_FirstGroup()
	{
		// Begin with zero groups
		auto ab = AudioBuffer{10, 0};

		// Add a first group with 5 channels
		auto group = ab.addGroup(5);
		QVERIFY(group != nullptr);
		QCOMPARE(&ab.group(0), group);
		QCOMPARE(group->channels(), 5);
		QCOMPARE(ab.groupCount(), 1);
		QCOMPARE(ab.totalChannels(), 5);
	}

	//! Verifies that a 2nd group can be appended after the 1st group
	void AddGroup_SecondGroup()
	{
		// Begin with 1 group
		auto ab = AudioBuffer{10, 3};

		// Add a 2nd group with 4 channels
		auto group = ab.addGroup(4);
		QVERIFY(group != nullptr);
		QCOMPARE(&ab.group(1), group);
		QCOMPARE(group->channels(), 4);
		QCOMPARE(ab.groupCount(), 2);
		QCOMPARE(ab.totalChannels(), 7);
	}

	//! Verifies that a group with 0 channels cannot be added and doing so has no effect
	void AddGroup_ZeroChannelsFails()
	{
		auto ab = AudioBuffer{10};
		QCOMPARE(ab.groupCount(), 1);
		QCOMPARE(ab.totalChannels(), 2);

		auto group = ab.addGroup(0);
		QCOMPARE(group, nullptr);

		// Nothing should have changed
		QCOMPARE(ab.groupCount(), 1);
		QCOMPARE(ab.totalChannels(), 2);
	}

	//! Verifies that groups cannot be added past the maximum group count
	void AddGroup_MaximumGroups()
	{
		auto ab = AudioBuffer{10, 0};

		// Add groups until no more can be added
		auto groupsLeft = static_cast<int>(lmms::MaxGroupsPerTrack);
		QVERIFY(groupsLeft >= 0);
		while (groupsLeft > 0)
		{
			auto temp = ab.addGroup(1);
			QVERIFY(temp != nullptr);
			--groupsLeft;
		}
		QCOMPARE(groupsLeft, 0);
		QCOMPARE(ab.groupCount(), lmms::MaxGroupsPerTrack);
		QCOMPARE(ab.totalChannels(), lmms::MaxGroupsPerTrack);

		// Next group should fail
		auto group = ab.addGroup(1);
		QCOMPARE(group, nullptr);
		QCOMPARE(ab.groupCount(), lmms::MaxGroupsPerTrack);
		QCOMPARE(ab.totalChannels(), lmms::MaxGroupsPerTrack);
	}

	//! Verifies that groups cannot be added past the maximum total channel count for the track
	void AddGroup_MaximumTotalChannels()
	{
		auto ab = AudioBuffer{10, lmms::MaxChannelsPerTrack - 1};

		// Try adding a group with enough channels
		// to push the total channels past the maximum for the track (should fail)
		auto group = ab.addGroup(2);
		QCOMPARE(group, nullptr);
		QCOMPARE(ab.totalChannels(), lmms::MaxChannelsPerTrack - 1);

		// Ok, how about just enough to hit the maximum
		// total channels for the track (should succeed)
		group = ab.addGroup(1);
		QVERIFY(group != nullptr);
		QCOMPARE(ab.totalChannels(), lmms::MaxChannelsPerTrack);
	}

	//! Verifies that groups can be specified using `setGroups`
	void SetGroups()
	{
		// Start with 6 channels, all in one group
		auto ab = AudioBuffer{10, 6};
		float* const* allBuffers = ab.allBuffers().data();

		QCOMPARE(ab.groupCount(), 1);
		QCOMPARE(ab.group(0).channels(), 6);

		// Split into group of 2 channels and group of 4 channels
		ab.setGroups(2, [](lmms::group_cnt_t idx, lmms::AudioBuffer::ChannelGroup&) {
			switch (idx)
			{
				case 0: return 2;
				case 1: return 4;
				default: return 0;
			}
		});

		QCOMPARE(ab.groupCount(), 2);
		QCOMPARE(ab.group(0).channels(), 2);
		QCOMPARE(ab.group(1).channels(), 4);

		// Check that no reallocation occurred
		QCOMPARE(ab.allBuffers().data(), allBuffers);
	}

	//! Verifies all silence flag bits are set when there are no channels
	void SilenceFlags_AllSilentWhenNoChannels()
	{
		auto ab = AudioBuffer{10, 0};
		QCOMPARE(ab.silenceFlags().all(), true);
	}

	//! Verifies all silence flags bits are set even after adding new groups/channels
	void SilenceFlags_AllSilentWhenNewGroupsAdded()
	{
		auto ab = AudioBuffer{10};
		QCOMPARE(ab.silenceFlags().all(), true);

		ab.addGroup(4);
		QCOMPARE(ab.silenceFlags().all(), true);
	}

	//! Verifies that `assumeNonSilent` clears a specific bit in the silence flags
	void AssumeNonSilent()
	{
		auto ab = AudioBuffer{10, 2};
		QCOMPARE(ab.silenceFlags().all(), true);

		// Assume 2nd channel is non-silent
		ab.assumeNonSilent(1);

		QCOMPARE(ab.silenceFlags().all(), false);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);
	}

	//! Verifies `enableSilenceTracking` enables and disables silence tracking
	void EnableSilenceTracking_GetterSetter()
	{
		auto ab = AudioBuffer{10};
		ab.enableSilenceTracking(true);
		QCOMPARE(ab.silenceTrackingEnabled(), true);

		ab.enableSilenceTracking(false);
		QCOMPARE(ab.silenceTrackingEnabled(), false);
	}

	//! Verifies that `enableSilenceTracking(true)` also updates silence flags
	void EnableSilenceTracking_UpdatesSilenceFlags()
	{
		auto ab = AudioBuffer{10, 2};
		ab.enableSilenceTracking(false);

		// Assume 2nd channel is non-silent
		ab.assumeNonSilent(1);

		QCOMPARE(ab.silenceFlags().all(), false);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);

		ab.enableSilenceTracking(true);

		// Silence flags should be updated
		QCOMPARE(ab.silenceFlags().all(), true);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], true);
	}

	//! Verifies that the `updateSilenceFlags` method does nothing to silence flags
	//! when all channels are already silent, regardless of which channels are selected
	//! for an update.
	void UpdateSilenceFlags_DoesNothingWhenSilent()
	{
		auto ab = AudioBuffer{10};
		ab.enableSilenceTracking(true);

		QCOMPARE(ab.silenceFlags().all(), true);

		// Right channel only
		QCOMPARE(ab.updateSilenceFlags(0b01), true);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Left channel only
		QCOMPARE(ab.updateSilenceFlags(0b10), true);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Both channels
		QCOMPARE(ab.updateSilenceFlags(0b11), true);
		QCOMPARE(ab.silenceFlags().all(), true);
	}

	//! Verifies that the `updateSilenceFlags` method updates a single non-silent channel,
	//! but only when that channel is selected for an update.
	void UpdateSilenceFlags_UpdatesChannelWhenSelected()
	{
		auto ab = AudioBuffer{10};
		ab.enableSilenceTracking(true);

		// Both channels should be silent
		QCOMPARE(ab.silenceFlags().all(), true);

		// Introduce noise to a frame in the right channel
		ab.group(0).buffer(1)[5] = 1.f;

		// Update the left channel - returns true because the updated channel is silent
		QCOMPARE(ab.updateSilenceFlags(0b01), true);

		// Silence flags remain the same since the non-silent channel was not updated
		QCOMPARE(ab.silenceFlags().all(), true);

		// Now update the right channel - returns false since the updated channel is not silent
		QCOMPARE(ab.updateSilenceFlags(0b10), false);

		// The silence flag for the right channel should now be cleared
		QCOMPARE(ab.silenceFlags()[0], true);  // left channel
		QCOMPARE(ab.silenceFlags()[1], false); // right channel
		QCOMPARE(ab.silenceFlags()[2], true);  // unused 3rd channel
		// eab.

		// Updating both channels returns false since one of them is non-silent
		QCOMPARE(ab.updateSilenceFlags(0b11), false);
	}

	//! Verifies that the `updateSilenceFlags` method works across channel groups
	void UpdateSilenceFlags_WorksWithGroups()
	{
		auto ab = AudioBuffer{10, 0};
		ab.enableSilenceTracking(true);

		ab.addGroup(3);
		ab.addGroup(1);

		// All channels should be silent
		QCOMPARE(ab.silenceFlags().all(), true);

		// Introduce noise to a frame in the 2nd channel of the 1st group
		ab.group(0).buffer(1)[5] = 1.f;

		// Introduce noise to a frame in the 1st channel of the 2nd group
		ab.group(1).buffer(0)[5] = 1.f;

		// Update the two silent channels - returns true because both are silent
		QCOMPARE(ab.updateSilenceFlags(0b0101), true);

		// Silence flags remain the same since the non-silent channels were not updated
		QCOMPARE(ab.silenceFlags().all(), true);

		// Now update the 3rd channel of the 1st group and the 1st channel of the 2nd group
		// Returns false since one of the updated channels is not silent
		QCOMPARE(ab.updateSilenceFlags(0b1100), false);

		// The silence flag for the 1st channel of the 2nd group should now be cleared,
		// but the 2nd channel of the 1st group should still be marked silent since
		// it has not been updated yet.
		QCOMPARE(ab.silenceFlags()[0], true);  // group 1, channel 1
		QCOMPARE(ab.silenceFlags()[1], true);  // group 1, channel 2
		QCOMPARE(ab.silenceFlags()[2], true);  // group 1, channel 3
		QCOMPARE(ab.silenceFlags()[3], false); // group 2, channel 1
		QCOMPARE(ab.silenceFlags()[4], true);  // unused 5th channel
		// eab.

		// Now update group 1, channel 2
		QCOMPARE(ab.updateSilenceFlags(0b0010), false);

		QCOMPARE(ab.silenceFlags()[0], true);  // group 1, channel 1
		QCOMPARE(ab.silenceFlags()[1], false); // group 1, channel 2
		QCOMPARE(ab.silenceFlags()[2], true);  // group 1, channel 3
		QCOMPARE(ab.silenceFlags()[3], false); // group 2, channel 1
		QCOMPARE(ab.silenceFlags()[4], true);  // unused 5th channel
		// eab.
	}

	//! Verifies that the `updateSilenceFlags` method updates a silent channel's flags
	//! from non-silent to silent when selected for update.
	void UpdateSilenceFlags_UpdatesFromNonSilenceToSilence()
	{
		auto ab = AudioBuffer{10, 2};
		ab.enableSilenceTracking(true);

		QCOMPARE(ab.silenceFlags().all(), true);

		// Assume 2nd channel is non-silent
		ab.assumeNonSilent(1);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);

		// Update 1st channel - does nothing
		QCOMPARE(ab.updateSilenceFlags(0b01), true);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);
		QCOMPARE(ab.silenceFlags()[2], true); // unused 3rd channel

		// Update 2nd channel (non-silent to silent)
		// Returns true because the channel's audio data is silent
		QCOMPARE(ab.updateSilenceFlags(0b10), true);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], true); // unused 3rd channel

		// Again, assume 2nd channel is non-silent
		ab.assumeNonSilent(1);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);
		QCOMPARE(ab.silenceFlags()[2], true); // unused 3rd channel

		// Update both channels
		// Returns true because both channels' audio data is silent
		QCOMPARE(ab.updateSilenceFlags(0b11), true);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], true); // unused 3rd channel
	}

	//! Verifies that `updateSilenceFlags` marks selected channels as non-silent when
	//! silence tracking is disabled.
	void UpdateSilenceFlags_NonSilentWhenSilenceTrackingDisabled()
	{
		auto ab = AudioBuffer{10, 2};
		ab.enableSilenceTracking(false);

		QCOMPARE(ab.silenceFlags().all(), true);

		// Now update the 2nd channel. The audio data is actually silent, but silence tracking
		// is disabled so it must assume the updated channel is non-silent just to be safe.
		QCOMPARE(ab.updateSilenceFlags(0b10), false);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);
		QCOMPARE(ab.silenceFlags()[2], true); // unused 3rd channel

		// Again with both channels
		QCOMPARE(ab.updateSilenceFlags(0b11), false);
		QCOMPARE(ab.silenceFlags()[0], false);
		QCOMPARE(ab.silenceFlags()[1], false);
		QCOMPARE(ab.silenceFlags()[2], true); // unused 3rd channel
	}

	//! Verifies that when no selected channels are passed to `updateSilenceFlags`,
	//! it returns true indicating that all selected channels are silent.
	void UpdateSilenceFlags_NoSelectionIsSilent()
	{
		auto ab = AudioBuffer{10, 2};

		// First, with silence tracking
		ab.enableSilenceTracking(true);
		QCOMPARE(ab.updateSilenceFlags(0), true);

		// Should produce the same result without silence tracking
		ab.enableSilenceTracking(false);
		QCOMPARE(ab.updateSilenceFlags(0), true);
	}

	//! Verifies that `updateAllSilenceFlags` updates all silence flags
	//! when silence tracking is enabled.
	void UpdateAllSilenceFlags_SilenceTrackingEnabled()
	{
		auto ab = AudioBuffer{10, 2};
		ab.addGroup(2);
		ab.enableSilenceTracking(true);

		QCOMPARE(ab.updateAllSilenceFlags(), true);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Introduce noise to a frame in the 1st channel of the 1st group
		ab.group(0).buffer(0)[5] = 1.f;

		// Introduce noise to a frame in the 2nd channel of the 2nd group
		ab.group(1).buffer(1)[5] = 1.f;

		// Those 2 channels should be marked silent after updating all channels
		QCOMPARE(ab.updateAllSilenceFlags(), false);
		QCOMPARE(ab.silenceFlags()[0], false); // channel 1
		QCOMPARE(ab.silenceFlags()[1], true);  // channel 2
		QCOMPARE(ab.silenceFlags()[2], true);  // channel 3
		QCOMPARE(ab.silenceFlags()[3], false); // channel 4
		QCOMPARE(ab.silenceFlags()[4], true);  // unused 5th channel

		// Silence the frame in the 2nd channel of the 2nd group
		ab.group(1).buffer(1)[5] = 0.f;

		// Now only the 1st channel should be marked silent after updating all channels
		QCOMPARE(ab.updateAllSilenceFlags(), false);
		QCOMPARE(ab.silenceFlags()[0], false); // channel 1
		QCOMPARE(ab.silenceFlags()[1], true);  // channel 2
		QCOMPARE(ab.silenceFlags()[2], true);  // channel 3
		QCOMPARE(ab.silenceFlags()[3], true);  // channel 4
		QCOMPARE(ab.silenceFlags()[4], true);  // unused 5th channel
	}

	//! Verifies that `updateAllSilenceFlags` marks all silence flags
	//! for used channels as non-silent when silence tracking is disabled.
	void UpdateAllSilenceFlags_SilenceTrackingDisabled()
	{
		auto ab = AudioBuffer{10, 2};
		ab.addGroup(2);
		ab.enableSilenceTracking(false);

		QCOMPARE(ab.updateAllSilenceFlags(), false);

		QCOMPARE(ab.silenceFlags()[0], false); // channel 1
		QCOMPARE(ab.silenceFlags()[1], false); // channel 2
		QCOMPARE(ab.silenceFlags()[2], false); // channel 3
		QCOMPARE(ab.silenceFlags()[3], false); // channel 4
		QCOMPARE(ab.silenceFlags()[4], true);  // unused 5th channel
	}

	//! Verifies that when there are no channels, `updateAllSilenceFlags`
	//! returns true indicating that all channels are silent.
	void UpdateAllSilenceFlags_NoChannelsIsSilent()
	{
		auto ab = AudioBuffer{10, 0};

		// First, with silence tracking
		ab.enableSilenceTracking(true);
		QCOMPARE(ab.updateAllSilenceFlags(), true);

		// Should produce the same result without silence tracking
		ab.enableSilenceTracking(false);
		QCOMPARE(ab.updateAllSilenceFlags(), true);
	}

	//! Verifies that `hasSignal` returns true if any of the selected
	//! channels are non-silent.
	void HasSignal()
	{
		auto ab = AudioBuffer{10, 2};
		ab.enableSilenceTracking(true);

		// Add a 2nd stereo group
		QVERIFY(ab.addGroup(2) != nullptr);

		// No signal since all channels are silent
		QCOMPARE(ab.hasSignal(0b1111), false);

		// Assume both left channels are non-silent
		ab.assumeNonSilent(0);
		ab.assumeNonSilent(2);

		// Check if any channels are non-silent
		QCOMPARE(ab.hasSignal(0b1111), true);

		// Check if either of the left channels are non-silent
		QCOMPARE(ab.hasSignal(0b0101), true);

		// Check if either of the right channels are non-silent
		QCOMPARE(ab.hasSignal(0b1010), false);

		// Check if either channel in the 1st group are non-silent
		QCOMPARE(ab.hasSignal(0b0011), true);

		// Check if the 5th channel (an unused channel) is non-silent
		QCOMPARE(ab.hasSignal(0b10000), false);
	}

	//! Verifies that the `sanitize` method only silences channels containing Inf or NaN
	void Sanitize_SilencesOnlyInfAndNaN()
	{
		lmms::MixHelpers::setNaNHandler(true);

		auto ab = AudioBuffer{10, 2};
		ab.enableSilenceTracking(true);

		// Add a 2nd stereo group
		QVERIFY(ab.addGroup(2) != nullptr);

		// Should have no effect when all buffers are silenced
		QCOMPARE(ab.silenceFlags().all(), true);
		ab.sanitize(0b1111);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Make left channel of 1st channel group
		// contain an Inf, and force the channel to non-silent
		ab.group(0).buffer(0)[5] = std::numeric_limits<float>::infinity();
		ab.assumeNonSilent(0);

		// Make right channel of 1st channel group non-silent too,
		// but using a valid value
		ab.group(0).buffer(1)[5] = 1.f;
		ab.assumeNonSilent(1);

		// Sanitize only the left channel
		ab.sanitize(0b0001);

		// The left channel's buffer should be silenced,
		// while the right channel should be unaffected
		QCOMPARE(ab.group(0).buffer(0)[5], 0.f);
		QCOMPARE(ab.group(0).buffer(1)[5], 1.f);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);
		QCOMPARE(ab.silenceFlags()[2], true);
		QCOMPARE(ab.silenceFlags()[3], true);
		QCOMPARE(ab.silenceFlags()[4], true); // unused 5th channel

		// Try again
		ab.group(0).buffer(0)[5] = std::numeric_limits<float>::quiet_NaN();
		ab.assumeNonSilent(0);

		// This time, sanitize both channels of the 1st channel group
		ab.sanitize(0b0011);

		// Again, the left channel's buffer should be silence,
		// while the right channel should be unaffected
		QCOMPARE(ab.group(0).buffer(0)[5], 0.f);
		QCOMPARE(ab.group(0).buffer(1)[5], 1.f);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);
		QCOMPARE(ab.silenceFlags()[2], true);
		QCOMPARE(ab.silenceFlags()[3], true);
		QCOMPARE(ab.silenceFlags()[4], true); // unused 5th channel
	}

	//! Verifies that the `silenceChannels` method silences the selected channels
	//! and updates their silence flags
	void SilenceChannels_SilencesSelectedChannels()
	{
		auto ab = AudioBuffer{10, 2};
		ab.enableSilenceTracking(true);

		// Add a 2nd stereo group
		QVERIFY(ab.addGroup(2) != nullptr);

		// Should have no effect when all buffers are silent
		QCOMPARE(ab.silenceFlags().all(), true);
		ab.silenceChannels(0b1111);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Make left channel of 2nd channel group contain
		// a non-silent value, and force the channel to be non-silent
		ab.group(1).buffer(0)[5] = 1.f;
		ab.assumeNonSilent(2);

		// Silence only the left channel
		ab.silenceChannels(0b0100);

		QCOMPARE(ab.silenceFlags()[0], true); // not selected
		QCOMPARE(ab.silenceFlags()[1], true); // not selected
		QCOMPARE(ab.silenceFlags()[2], true); // updated!
		QCOMPARE(ab.silenceFlags()[3], true); // not selected

		// Make right channel of 2nd channel group contain
		// a non-silent value, and force the channel to be non-silent
		ab.group(1).buffer(1)[5] = 1.f;
		ab.assumeNonSilent(3);

		// Silence only the right channel
		ab.silenceChannels(0b1000);

		QCOMPARE(ab.silenceFlags()[0], true); // not selected
		QCOMPARE(ab.silenceFlags()[1], true); // not selected
		QCOMPARE(ab.silenceFlags()[2], true); // not selected
		QCOMPARE(ab.silenceFlags()[3], true); // updated!

		// Make right channel of 1st channel group and
		// both channels of 2nd channel group contain
		// a non-silent value, and force those channels to be non-silent
		ab.group(1).buffer(1)[5] = 1.f;
		ab.assumeNonSilent(1);
		ab.group(1).buffer(0)[5] = 1.f;
		ab.assumeNonSilent(2);
		ab.group(1).buffer(1)[5] = 1.f;
		ab.assumeNonSilent(3);

		// Silence both channels of the 2nd channel group,
		// plus the already-silent left channel of the 1st group
		ab.silenceChannels(0b1101);

		QCOMPARE(ab.silenceFlags()[0], true);  // selected, but already silent
		QCOMPARE(ab.silenceFlags()[1], false); // not selected, remains non-silent
		QCOMPARE(ab.silenceFlags()[2], true);  // updated!
		QCOMPARE(ab.silenceFlags()[3], true);  // updated!
	}
};

QTEST_GUILESS_MAIN(AudioBufferTest)
#include "AudioBufferTest.moc"
