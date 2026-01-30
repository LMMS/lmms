/*
 * TrackChannelContainerTest.cpp
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

// TODO: Add tests for when silence tracking is disabled

#include "TrackChannelContainer.h"

#include <QObject>
#include <QtTest>
#include <algorithm>
#include <iostream>
#include <numeric>

#include "MixHelpers.h"

using lmms::TrackChannelContainer;

class TrackChannelContainerTest : public QObject
{
	Q_OBJECT

private:
	std::vector<lmms::SampleFrame> m_buffers;
	std::vector<lmms::SampleFrame*> m_trackChannels;

	// Creates new TrackChannelContainer with quiet buffers
	auto createTrackChannelContainer(lmms::f_cnt_t frames) -> TrackChannelContainer
	{
		auto tc = TrackChannelContainer{frames};
		tc.enableSilenceTracking(true);
		return tc;
	}

private slots:
	//! Verifies correct default construction
	void DefaultConstructor()
	{
		auto tc = TrackChannelContainer{};
		QCOMPARE(tc.groupCount(), 0);
		QCOMPARE(tc.totalChannels(), 0);
		QCOMPARE(tc.frames(), 0);
		QCOMPARE(tc.silenceFlags().none(), true);
	}

	//! Verifies correct construction
	void Constructor()
	{
		auto tc = createTrackChannelContainer(10);
		QCOMPARE(tc.groupCount(), 1);
		QCOMPARE(tc.totalChannels(), 2);
		QCOMPARE(tc.frames(), 10);
		QCOMPARE(tc.silenceFlags().all(), true);
	}

	//! Verifies that the `addGroup` method works as intended
	void AddGroup()
	{
		// A group with 2 channels (the main group) is added by default
		auto tc = createTrackChannelContainer(10);
		QCOMPARE(tc.groupCount(), 1);
		QCOMPARE(tc.totalChannels(), 2);

		// Add a group with 5 channels
		auto group2 = tc.addGroup(5);
		QVERIFY(group2 != nullptr);
		QCOMPARE(group2->channels(), 5);
		QCOMPARE(tc.groupCount(), 2);
		QCOMPARE(tc.totalChannels(), 7);

		// Add a group with 1 channel
		auto group3 = tc.addGroup(1);
		QVERIFY(group3 != nullptr);
		QCOMPARE(group3->channels(), 1);
		QCOMPARE(tc.groupCount(), 3);
		QCOMPARE(tc.totalChannels(), 8);

		// Add a group with as many channels as possible
		auto group4 = tc.addGroup(lmms::MaxChannelsPerGroup);
		QVERIFY(group4 != nullptr);
		QCOMPARE(group4->channels(), lmms::MaxChannelsPerGroup);
		QCOMPARE(tc.groupCount(), 4);
		QCOMPARE(tc.totalChannels(), 8 + lmms::MaxChannelsPerGroup);

		// Now, try with too many channels
		auto group5 = tc.addGroup(lmms::MaxChannelsPerGroup + 1);
		QCOMPARE(group5, nullptr);
		QCOMPARE(tc.groupCount(), 4);
		QCOMPARE(tc.totalChannels(), 8 + lmms::MaxChannelsPerGroup);

		// And again with too few channels
		auto group6 = tc.addGroup(0);
		QCOMPARE(group6, nullptr);
		QCOMPARE(tc.groupCount(), 4);
		QCOMPARE(tc.totalChannels(), 8 + lmms::MaxChannelsPerGroup);

		// Add more groups until no more can be added
		auto groupsLeft = static_cast<int>(lmms::MaxGroupsPerTrack - tc.groupCount());
		QVERIFY(groupsLeft >= 0);
		while (groupsLeft > 0)
		{
			auto temp = tc.addGroup(1);
			QVERIFY(temp != nullptr);
			--groupsLeft;
		}
		QCOMPARE(groupsLeft, 0);

		// Cannot add more groups
		auto group7 = tc.addGroup(1);
		QCOMPARE(group7, nullptr);

		// Create new TrackChannelContainer
		auto tc2 = createTrackChannelContainer(10);
		auto channelsLeft = lmms::MaxTrackChannels - tc2.totalChannels();
		constexpr auto channelsPerGroup = static_cast<int>(lmms::MaxChannelsPerGroup - 1);

		while (channelsLeft - channelsPerGroup > 0)
		{
			auto group = tc2.addGroup(channelsPerGroup);
			QVERIFY(group != nullptr);
			channelsLeft -= channelsPerGroup;
		}
		QVERIFY(channelsLeft > 0);
		QVERIFY(tc2.groupCount() < lmms::MaxGroupsPerTrack);

		auto group8 = tc2.addGroup(channelsLeft);
		QVERIFY(group8 != nullptr);
		QVERIFY(tc2.groupCount() < lmms::MaxGroupsPerTrack);
		QCOMPARE(tc2.totalChannels(), lmms::MaxTrackChannels);

		// Verify that more groups cannot be added when the maximum track channels
		// has been reached
		auto group9 = tc2.addGroup(1);
		QCOMPARE(group9, nullptr);
		QCOMPARE(tc2.totalChannels(), lmms::MaxTrackChannels);
	}

	//! Verifies that the `updateSilenceFlags` method works as intended
	void UpdateSilenceFlags()
	{
		auto tc = createTrackChannelContainer(10);

		// Add a 2nd stereo group
		QVERIFY(tc.addGroup(2) != nullptr);

		// Both channels should be silent
		QCOMPARE(tc.updateSilenceFlags(0b1111), true);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Now introduce a non-zero sample to the right channel of 2nd track channel group
		// but update all channels except that one
		tc.buffers(1).buffer(1)[5] = 1.f;
		QCOMPARE(tc.updateSilenceFlags(0b0111), true);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Now update that channel
		QCOMPARE(tc.updateSilenceFlags(0b1000), false);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], true);
		QCOMPARE(tc.silenceFlags()[3], false);

		// Should return true if no channels are selected for update
		QCOMPARE(tc.updateSilenceFlags(0), true);

		// Make the left channel of the 1st track channel group non-zero too
		tc.buffers(0).buffer(0)[5] = 1.f;

		// Update the left channel of the 1st track channel group
		QCOMPARE(tc.updateSilenceFlags(0b0001), false);
		QCOMPARE(tc.silenceFlags()[0], false);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], true);
		QCOMPARE(tc.silenceFlags()[3], false);

		// Make the left channel of the 2nd track channel group non-zero,
		// and set the left channel of 1st track channel group back to zero
		tc.buffers(1).buffer(0)[5] = 1.f;
		tc.buffers(0).buffer(0)[5] = 0.f;

		// Update only the 2nd track channel group
		QCOMPARE(tc.updateSilenceFlags(0b1100), false);
		QCOMPARE(tc.silenceFlags()[0], false);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], false);
		QCOMPARE(tc.silenceFlags()[3], false);

		// Now update the 1st track channel group
		QCOMPARE(tc.updateSilenceFlags(0b0011), true);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], false);
		QCOMPARE(tc.silenceFlags()[3], false);

		// Zero out all channels again
		tc.buffers(1).buffer(0)[5] = 0.f;
		tc.buffers(1).buffer(1)[5] = 0.f;
		QCOMPARE(tc.updateSilenceFlags(0b0000), true);
		QCOMPARE(tc.silenceFlags().all(), false);
		QCOMPARE(tc.updateSilenceFlags(0b1111), true);
		QCOMPARE(tc.silenceFlags().all(), true);
	}

	//! Verifies that the `updateAllSilenceFlags` method works as intended
	void UpdateAllSilenceFlags()
	{
		auto tc = createTrackChannelContainer(10);

		// Add a 2nd stereo group
		QVERIFY(tc.addGroup(2) != nullptr);

		// Both channels should be silent
		QCOMPARE(tc.updateAllSilenceFlags(), true);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Now introduce a non-zero sample to the right channel of 2nd track channel group
		tc.buffers(1).buffer(1)[5] = 1.f;
		QCOMPARE(tc.updateAllSilenceFlags(), false);
		QCOMPARE(tc.silenceFlags().all(), false);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], true);
		QCOMPARE(tc.silenceFlags()[3], false);

		// Make the left channel of the 1st track channel group non-zero too
		tc.buffers(0).buffer(0)[5] = 1.f;
		QCOMPARE(tc.updateAllSilenceFlags(), false);
		QCOMPARE(tc.silenceFlags().all(), false);
		QCOMPARE(tc.silenceFlags()[0], false);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], true);
		QCOMPARE(tc.silenceFlags()[3], false);

		// Zero out all channels again
		tc.buffers(0).buffer(0)[5] = 0.f;
		tc.buffers(1).buffer(1)[5] = 0.f;
		QCOMPARE(tc.updateAllSilenceFlags(), true);
		QCOMPARE(tc.silenceFlags().all(), true);
	}

	//! Verifies that the `hasInputNoise` method works as intended
	void HasInputNoise()
	{
		auto tc = createTrackChannelContainer(10);

		// Add a 2nd stereo group
		QVERIFY(tc.addGroup(2) != nullptr);

		// No input noise since all track channels are silent
		QCOMPARE(tc.hasInputNoise(0b1111), false);

		// Make the left channels in both groups non-zero and manually update the silence status
		tc.buffers(0).buffer(0)[5] = 1.f;
		tc.buffers(1).buffer(0)[5] = 1.f;
		tc.assumeNonSilent(0);
		tc.assumeNonSilent(2);

		// Check if any channels are non-zero
		QCOMPARE(tc.hasInputNoise(0b1111), true);

		// Check if either of the left channels are non-zero
		QCOMPARE(tc.hasInputNoise(0b0101), true);

		// Check if either of the right channels are non-zero
		QCOMPARE(tc.hasInputNoise(0b1010), false);

		// Check if either channel in the 1st group are non-zero
		QCOMPARE(tc.hasInputNoise(0b0011), true);
	}

	//! Verifies that the `sanitize` method works as intended
	void Sanitize()
	{
		lmms::MixHelpers::setNaNHandler(true);

		auto tc = createTrackChannelContainer(10);

		// Add a 2nd stereo group
		QVERIFY(tc.addGroup(2) != nullptr);

		// Should have no effect when all buffers are zeroed
		QCOMPARE(tc.silenceFlags().all(), true);
		tc.sanitize(0b1111);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Make left channel of 1st track channel group contain an Inf, and force the channel to non-quiet
		tc.buffers(0).buffer(0)[5] = std::numeric_limits<float>::infinity();
		tc.assumeNonSilent(0);

		// Make right channel of 1st track channel group non-zero too, but using a valid value
		tc.buffers(0).buffer(1)[5] = 1.f;
		tc.assumeNonSilent(1);

		// Sanitize only the left channel
		tc.sanitize(0b0001);

		// The left channel's buffer should be zeroed, while the right channel should be unaffected
		QCOMPARE(tc.buffers(0).buffer(0)[5], 0.f);
		QCOMPARE(tc.buffers(0).buffer(1)[5], 1.f);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);

		// Try again
		tc.buffers(0).buffer(0)[5] = std::numeric_limits<float>::infinity();
		tc.assumeNonSilent(0);

		// This time, sanitize both channels of the 1st track channel group
		tc.sanitize(0b0011);

		// Again, the left channel's buffer should be zeroed, while the right channel should be unaffected
		QCOMPARE(tc.buffers(0).buffer(0)[5], 0.f);
		QCOMPARE(tc.buffers(0).buffer(1)[5], 1.f);
		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);
	}

	//! Verifies that the `silenceChannels` method works as intended
	void SilenceChannels()
	{
		auto tc = createTrackChannelContainer(10);

		// Add a 2nd stereo group
		QVERIFY(tc.addGroup(2) != nullptr);

		// Should have no effect when all buffers are zeroed
		QCOMPARE(tc.silenceFlags().all(), true);
		tc.silenceChannels(0b1111);
		QCOMPARE(tc.silenceFlags().all(), true);

		// Make left channel of 2nd track channel group contain a non-zero value, and force the channel to non-quiet
		tc.buffers(1).buffer(0)[5] = 1.f;
		tc.assumeNonSilent(2);

		// Silence only the left channel
		tc.silenceChannels(0b0100);

		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], true);
		QCOMPARE(tc.silenceFlags()[3], true);

		// Make right channel of 2nd track channel group contain a non-zero value, and force the channel to non-quiet
		tc.buffers(1).buffer(1)[5] = 1.f;
		tc.assumeNonSilent(3);

		// Silence only the right channel
		tc.silenceChannels(0b1000);

		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], true);
		QCOMPARE(tc.silenceFlags()[2], true);
		QCOMPARE(tc.silenceFlags()[3], true);

		// Make right channel of 1st track channel group and both channels of 2nd track channel group contain
		// a non-zero value, and force those channels to non-quiet
		tc.buffers(1).buffer(1)[5] = 1.f;
		tc.assumeNonSilent(1);
		tc.buffers(1).buffer(0)[5] = 1.f;
		tc.assumeNonSilent(2);
		tc.buffers(1).buffer(1)[5] = 1.f;
		tc.assumeNonSilent(3);

		// Silence both channels of the 2nd track channel group, plus the already-quiet left channel of the 1st group
		tc.silenceChannels(0b1101);

		QCOMPARE(tc.silenceFlags()[0], true);
		QCOMPARE(tc.silenceFlags()[1], false);
		QCOMPARE(tc.silenceFlags()[2], true);
		QCOMPARE(tc.silenceFlags()[3], true);
	}
};

QTEST_GUILESS_MAIN(TrackChannelContainerTest)
#include "TrackChannelContainerTest.moc"
