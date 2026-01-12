/*
 * AudioBusTest.cpp
 *
 * Copyright (c) 2025 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "AudioBus.h"

#include <QObject>
#include <QtTest>
#include <algorithm>
#include <iostream>
#include <numeric>

#include "MixHelpers.h"

using lmms::AudioBus;

class AudioBusTest : public QObject
{
	Q_OBJECT

private:
	std::vector<lmms::SampleFrame> m_buffers;
	std::vector<lmms::SampleFrame*> m_trackChannels;

	// Creates new AudioBus with quiet buffers
	auto createAudioBus(lmms::f_cnt_t frames) -> AudioBus
	{
		auto ab = AudioBus{frames};
		ab.enableSilenceTracking(true);
		return ab;
	}

private slots:
	//! Verifies correct default construction
	void DefaultConstructor()
	{
		auto ab = AudioBus{};
		QCOMPARE(ab.busCount(), 0);
		QCOMPARE(ab.totalChannels(), 0);
		QCOMPARE(ab.frames(), 0);
		QCOMPARE(ab.silenceFlags().none(), true);
	}

	//! Verifies correct construction
	void Constructor()
	{
		auto ab = createAudioBus(10);
		QCOMPARE(ab.busCount(), 1);
		QCOMPARE(ab.totalChannels(), 2);
		QCOMPARE(ab.frames(), 10);
		QCOMPARE(ab.silenceFlags().all(), true);
	}

	//! Verifies that the `addBus` method works as intended
	void AddBus()
	{
		// A bus with 2 channels (the main bus) is added by default
		auto ab = createAudioBus(10);
		QCOMPARE(ab.busCount(), 1);
		QCOMPARE(ab.totalChannels(), 2);

		// Add a bus with 5 channels
		auto bus2 = ab.addBus(5);
		QVERIFY(bus2 != nullptr);
		QCOMPARE(bus2->channels(), 5);
		QCOMPARE(bus2->startingChannel(), 2);
		QCOMPARE(ab.busCount(), 2);
		QCOMPARE(ab.totalChannels(), 7);

		// Add a bus with 1 channel
		auto bus3 = ab.addBus(1);
		QVERIFY(bus3 != nullptr);
		QCOMPARE(bus3->channels(), 1);
		QCOMPARE(bus3->startingChannel(), 2 + 5);
		QCOMPARE(ab.busCount(), 3);
		QCOMPARE(ab.totalChannels(), 8);

		// Add a bus with as many channels as possible
		auto bus4 = ab.addBus(lmms::MaxChannelsPerBus);
		QVERIFY(bus4 != nullptr);
		QCOMPARE(bus4->channels(), lmms::MaxChannelsPerBus);
		QCOMPARE(bus4->startingChannel(), 2 + 5 + 1);
		QCOMPARE(ab.busCount(), 4);
		QCOMPARE(ab.totalChannels(), 8 + lmms::MaxChannelsPerBus);

		// Now, try with too many channels
		auto bus5 = ab.addBus(lmms::MaxChannelsPerBus + 1);
		QCOMPARE(bus5, nullptr);
		QCOMPARE(ab.busCount(), 4);
		QCOMPARE(ab.totalChannels(), 8 + lmms::MaxChannelsPerBus);

		// And again with too few channels
		auto bus6 = ab.addBus(0);
		QCOMPARE(bus6, nullptr);
		QCOMPARE(ab.busCount(), 4);
		QCOMPARE(ab.totalChannels(), 8 + lmms::MaxChannelsPerBus);

		// Add more busses until no more can be added
		auto bussesLeft = static_cast<int>(lmms::MaxBussesPerTrack - ab.busCount());
		QVERIFY(bussesLeft >= 0);
		while (bussesLeft > 0)
		{
			auto temp = ab.addBus(1);
			QVERIFY(temp != nullptr);
			--bussesLeft;
		}
		QCOMPARE(bussesLeft, 0);

		// Cannot add more busses
		auto bus7 = ab.addBus(1);
		QCOMPARE(bus7, nullptr);

		// Create new AudioBus
		auto ab2 = createAudioBus(10);
		auto channelsLeft = lmms::MaxTrackChannels - ab2.totalChannels();
		constexpr auto channelsPerBus = static_cast<int>(lmms::MaxChannelsPerBus - 1);

		while (channelsLeft - channelsPerBus > 0)
		{
			auto bus = ab2.addBus(channelsPerBus);
			QVERIFY(bus != nullptr);
			channelsLeft -= channelsPerBus;
		}
		QVERIFY(channelsLeft > 0);
		QVERIFY(ab2.busCount() < lmms::MaxBussesPerTrack);

		auto bus8 = ab2.addBus(channelsLeft);
		QVERIFY(bus8 != nullptr);
		QVERIFY(ab2.busCount() < lmms::MaxBussesPerTrack);
		QCOMPARE(ab2.totalChannels(), lmms::MaxTrackChannels);

		// Verify that more busses cannot be added when the maximum track channels
		// has been reached
		auto bus9 = ab2.addBus(1);
		QCOMPARE(bus9, nullptr);
		QCOMPARE(ab2.totalChannels(), lmms::MaxTrackChannels);
	}

	//! Verifies that the `updateSilenceFlags` method works as intended
	void UpdateSilenceFlags()
	{
		auto ab = createAudioBus(10);

		// Add a 2nd stereo bus
		QVERIFY(ab.addBus(2) != nullptr);

		// Both channels should be silent
		QCOMPARE(ab.updateSilenceFlags(0b1111), true);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Now introduce a non-zero sample to the right channel of 2nd track channel bus
		// but update all channels except that one
		ab.buffers(1).buffer(1)[5] = 1.f;
		QCOMPARE(ab.updateSilenceFlags(0b0111), true);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Now update that channel
		QCOMPARE(ab.updateSilenceFlags(0b1000), false);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], true);
		QCOMPARE(ab.silenceFlags()[3], false);

		// Should return true if no channels are selected for update
		QCOMPARE(ab.updateSilenceFlags(0), true);

		// Make the left channel of the 1st track channel bus non-zero too
		ab.buffers(0).buffer(0)[5] = 1.f;

		// Update the left channel of the 1st track channel bus
		QCOMPARE(ab.updateSilenceFlags(0b0001), false);
		QCOMPARE(ab.silenceFlags()[0], false);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], true);
		QCOMPARE(ab.silenceFlags()[3], false);

		// Make the left channel of the 2nd track channel bus non-zero,
		// and set the left channel of 1st track channel bus back to zero
		ab.buffers(1).buffer(0)[5] = 1.f;
		ab.buffers(0).buffer(0)[5] = 0.f;

		// Update only the 2nd track channel bus
		QCOMPARE(ab.updateSilenceFlags(0b1100), false);
		QCOMPARE(ab.silenceFlags()[0], false);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], false);
		QCOMPARE(ab.silenceFlags()[3], false);

		// Now update the 1st track channel bus
		QCOMPARE(ab.updateSilenceFlags(0b0011), true);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], false);
		QCOMPARE(ab.silenceFlags()[3], false);

		// Zero out all channels again
		ab.buffers(1).buffer(0)[5] = 0.f;
		ab.buffers(1).buffer(1)[5] = 0.f;
		QCOMPARE(ab.updateSilenceFlags(0b0000), true);
		QCOMPARE(ab.silenceFlags().all(), false);
		QCOMPARE(ab.updateSilenceFlags(0b1111), true);
		QCOMPARE(ab.silenceFlags().all(), true);
	}

	//! Verifies that the `updateAllSilenceFlags` method works as intended
	void UpdateAllSilenceFlags()
	{
		auto ab = createAudioBus(10);

		// Add a 2nd stereo bus
		QVERIFY(ab.addBus(2) != nullptr);

		// Both channels should be silent
		QCOMPARE(ab.updateAllSilenceFlags(), true);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Now introduce a non-zero sample to the right channel of 2nd track channel bus
		ab.buffers(1).buffer(1)[5] = 1.f;
		QCOMPARE(ab.updateAllSilenceFlags(), false);
		QCOMPARE(ab.silenceFlags().all(), false);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], true);
		QCOMPARE(ab.silenceFlags()[3], false);

		// Make the left channel of the 1st track channel bus non-zero too
		ab.buffers(0).buffer(0)[5] = 1.f;
		QCOMPARE(ab.updateAllSilenceFlags(), false);
		QCOMPARE(ab.silenceFlags().all(), false);
		QCOMPARE(ab.silenceFlags()[0], false);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], true);
		QCOMPARE(ab.silenceFlags()[3], false);

		// Zero out all channels again
		ab.buffers(0).buffer(0)[5] = 0.f;
		ab.buffers(1).buffer(1)[5] = 0.f;
		QCOMPARE(ab.updateAllSilenceFlags(), true);
		QCOMPARE(ab.silenceFlags().all(), true);
	}

	//! Verifies that the `hasInputNoise` method works as intended
	void HasInputNoise()
	{
		auto ab = createAudioBus(10);

		// Add a 2nd stereo bus
		QVERIFY(ab.addBus(2) != nullptr);

		// No input noise since all track channels are silent
		QCOMPARE(ab.hasInputNoise(0b1111), false);

		// Make the left channels in both busses non-zero and manually update the silence status
		ab.buffers(0).buffer(0)[5] = 1.f;
		ab.buffers(1).buffer(0)[5] = 1.f;
		ab.silenceFlags()[0] = false;
		ab.silenceFlags()[2] = false;

		// Check if any channels are non-zero
		QCOMPARE(ab.hasInputNoise(0b1111), true);

		// Check if either of the left channels are non-zero
		QCOMPARE(ab.hasInputNoise(0b0101), true);

		// Check if either of the right channels are non-zero
		QCOMPARE(ab.hasInputNoise(0b1010), false);

		// Check if either channel in the 1st bus are non-zero
		QCOMPARE(ab.hasInputNoise(0b0011), true);
	}

	//! Verifies that the `sanitize` method works as intended
	void Sanitize()
	{
		lmms::MixHelpers::setNaNHandler(true);

		auto ab = createAudioBus(10);

		// Add a 2nd stereo bus
		QVERIFY(ab.addBus(2) != nullptr);

		// Should have no effect when all buffers are zeroed
		QCOMPARE(ab.silenceFlags().all(), true);
		ab.sanitize(0b1111);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Make left channel of 1st track channel bus contain an Inf, and force the channel to non-quiet
		ab.buffers(0).buffer(0)[5] = std::numeric_limits<float>::infinity();
		ab.silenceFlags()[0] = false;

		// Make right channel of 1st track channel bus non-zero too, but using a valid value
		ab.buffers(0).buffer(1)[5] = 1.f;
		ab.silenceFlags()[1] = false;

		// Sanitize only the left channel
		ab.sanitize(0b0001);

		// The left channel's buffer should be zeroed, while the right channel should be unaffected
		QCOMPARE(ab.buffers(0).buffer(0)[5], 0.f);
		QCOMPARE(ab.buffers(0).buffer(1)[5], 1.f);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);

		// Try again
		ab.buffers(0).buffer(0)[5] = std::numeric_limits<float>::infinity();
		ab.silenceFlags()[0] = false;

		// This time, sanitize both channels of the 1st track channel bus
		ab.sanitize(0b0011);

		// Again, the left channel's buffer should be zeroed, while the right channel should be unaffected
		QCOMPARE(ab.buffers(0).buffer(0)[5], 0.f);
		QCOMPARE(ab.buffers(0).buffer(1)[5], 1.f);
		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);
	}

	//! Verifies that the `silenceChannels` method works as intended
	void SilenceChannels()
	{
		auto ab = createAudioBus(10);

		// Add a 2nd stereo bus
		QVERIFY(ab.addBus(2) != nullptr);

		// Should have no effect when all buffers are zeroed
		QCOMPARE(ab.silenceFlags().all(), true);
		ab.silenceChannels(0b1111);
		QCOMPARE(ab.silenceFlags().all(), true);

		// Make left channel of 2nd track channel bus contain a non-zero value, and force the channel to non-quiet
		ab.buffers(1).buffer(0)[5] = 1.f;
		ab.silenceFlags()[2] = false;

		// Silence only the left channel
		ab.silenceChannels(0b0100);

		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], true);
		QCOMPARE(ab.silenceFlags()[3], true);

		// Make right channel of 2nd track channel bus contain a non-zero value, and force the channel to non-quiet
		ab.buffers(1).buffer(1)[5] = 1.f;
		ab.silenceFlags()[3] = false;

		// Silence only the right channel
		ab.silenceChannels(0b1000);

		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], true);
		QCOMPARE(ab.silenceFlags()[2], true);
		QCOMPARE(ab.silenceFlags()[3], true);

		// Make right channel of 1st track channel bus and both channels of 2nd track channel bus contain
		// a non-zero value, and force those channels to non-quiet
		ab.buffers(1).buffer(1)[5] = 1.f;
		ab.silenceFlags()[1] = false;
		ab.buffers(1).buffer(0)[5] = 1.f;
		ab.silenceFlags()[2] = false;
		ab.buffers(1).buffer(1)[5] = 1.f;
		ab.silenceFlags()[3] = false;

		// Silence both channels of the 2nd track channel bus, plus the already-quiet left channel of the 1st bus
		ab.silenceChannels(0b1101);

		QCOMPARE(ab.silenceFlags()[0], true);
		QCOMPARE(ab.silenceFlags()[1], false);
		QCOMPARE(ab.silenceFlags()[2], true);
		QCOMPARE(ab.silenceFlags()[3], true);
	}
};

QTEST_GUILESS_MAIN(AudioBusTest)
#include "AudioBusTest.moc"
