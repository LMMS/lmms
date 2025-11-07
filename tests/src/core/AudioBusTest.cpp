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
	auto createBus(lmms::track_ch_t channels, lmms::f_cnt_t frames) -> AudioBus
	{
		assert(channels % 2 == 0);
		const auto channelPairs = static_cast<lmms::track_ch_t>(channels / 2);

		m_trackChannels.resize(channelPairs);

		m_buffers.resize(channelPairs * frames);
		std::ranges::fill(m_buffers, lmms::SampleFrame{});

		for (lmms::track_ch_t channelPair = 0; channelPair < channelPairs; ++channelPair)
		{
			m_trackChannels[channelPair] = &m_buffers[channelPair * frames];
		}

		auto ab = AudioBus{m_trackChannels.data(), channelPairs, frames};
		ab.enableSilenceTracking(true);

		return ab;
	}

private slots:
	//! Verifies correct default construction
	void DefaultConstructor()
	{
		auto ab = AudioBus{};
		QCOMPARE(ab.bus(), nullptr);
		QCOMPARE(ab.channels(), 0);
		QCOMPARE(ab.frames(), 0);
		QCOMPARE(ab.quietChannels().none(), true);
	}

	//! Verifies correct construction
	void Constructor()
	{
		auto ab = createBus(2, 10);
		QCOMPARE(ab.bus() == nullptr, false);
		QCOMPARE(ab.channels(), 2);
		QCOMPARE(ab.frames(), 10);
		QCOMPARE(ab.quietChannels().all(), true);
	}

	//! Verifies that the `update` method works as intended
	void Update()
	{
		auto ab = createBus(4, 10);

		// Both channels should be silent
		QCOMPARE(ab.update(0b1111), true);
		QCOMPARE(ab.quietChannels().all(), true);

		// Now introduce a non-zero sample to the right channel of 2nd track channel bus
		// but update all channels except that one
		ab.trackChannelPair(1).frame(5)[1] = 1.f;
		QCOMPARE(ab.update(0b0111), true);
		QCOMPARE(ab.quietChannels().all(), true);

		// Now update that channel
		QCOMPARE(ab.update(0b1000), false);
		QCOMPARE(ab.quietChannels()[0], true);
		QCOMPARE(ab.quietChannels()[1], true);
		QCOMPARE(ab.quietChannels()[2], true);
		QCOMPARE(ab.quietChannels()[3], false);

		// Should return true if no channels are selected for update
		QCOMPARE(ab.update(0), true);

		// Make the left channel of the 1st track channel bus non-zero too
		ab.trackChannelPair(0).frame(5)[0] = 1.f;

		// Update the left channel of the 1st track channel bus
		QCOMPARE(ab.update(0b0001), false);
		QCOMPARE(ab.quietChannels()[0], false);
		QCOMPARE(ab.quietChannels()[1], true);
		QCOMPARE(ab.quietChannels()[2], true);
		QCOMPARE(ab.quietChannels()[3], false);

		// Make the left channel of the 2nd track channel bus non-zero,
		// and set the left channel of 1st track channel bus back to zero
		ab.trackChannelPair(1).frame(5)[0] = 1.f;
		ab.trackChannelPair(0).frame(5)[0] = 0.f;

		// Update only the 2nd track channel bus
		QCOMPARE(ab.update(0b1100), false);
		QCOMPARE(ab.quietChannels()[0], false);
		QCOMPARE(ab.quietChannels()[1], true);
		QCOMPARE(ab.quietChannels()[2], false);
		QCOMPARE(ab.quietChannels()[3], false);

		// Now update the 1st track channel bus
		QCOMPARE(ab.update(0b0011), true);
		QCOMPARE(ab.quietChannels()[0], true);
		QCOMPARE(ab.quietChannels()[1], true);
		QCOMPARE(ab.quietChannels()[2], false);
		QCOMPARE(ab.quietChannels()[3], false);

		// Zero out all channels again
		ab.trackChannelPair(1).frame(5)[0] = 0.f;
		ab.trackChannelPair(1).frame(5)[1] = 0.f;
		QCOMPARE(ab.update(0b0000), true);
		QCOMPARE(ab.quietChannels().all(), false);
		QCOMPARE(ab.update(0b1111), true);
		QCOMPARE(ab.quietChannels().all(), true);
	}

	//! Verifies that the `updateAll` method works as intended
	void UpdateAll()
	{
		auto ab = createBus(4, 10);

		// Both channels should be silent
		QCOMPARE(ab.updateAll(), true);
		QCOMPARE(ab.quietChannels().all(), true);

		// Now introduce a non-zero sample to the right channel of 2nd track channel bus
		ab.trackChannelPair(1).frame(5)[1] = 1.f;
		QCOMPARE(ab.updateAll(), false);
		QCOMPARE(ab.quietChannels().all(), false);
		QCOMPARE(ab.quietChannels()[0], true);
		QCOMPARE(ab.quietChannels()[1], true);
		QCOMPARE(ab.quietChannels()[2], true);
		QCOMPARE(ab.quietChannels()[3], false);

		// Make the left channel of the 1st track channel bus non-zero too
		ab.trackChannelPair(0).frame(5)[0] = 1.f;
		QCOMPARE(ab.updateAll(), false);
		QCOMPARE(ab.quietChannels().all(), false);
		QCOMPARE(ab.quietChannels()[0], false);
		QCOMPARE(ab.quietChannels()[1], true);
		QCOMPARE(ab.quietChannels()[2], true);
		QCOMPARE(ab.quietChannels()[3], false);

		// Zero out all channels again
		ab.trackChannelPair(0).frame(5)[0] = 0.f;
		ab.trackChannelPair(1).frame(5)[1] = 0.f;
		QCOMPARE(ab.updateAll(), true);
		QCOMPARE(ab.quietChannels().all(), true);
	}

	//! Verifies that the `hasInputNoise` method works as intended
	void HasInputNoise()
	{
		auto ab = createBus(4, 10);

		// No input noise since all track channels are silent
		QCOMPARE(ab.hasInputNoise(0b1111), false);

		// Make the left channels in both busses non-zero and manually update the silence status
		ab.trackChannelPair(0).frame(5)[0] = 1.f;
		ab.trackChannelPair(1).frame(5)[0] = 1.f;
		ab.quietChannels()[0] = false;
		ab.quietChannels()[2] = false;

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

		auto ab = createBus(4, 10);

		// Should have no effect when all buffers are zeroed
		QCOMPARE(ab.quietChannels().all(), true);
		ab.sanitize(0b1111);
		QCOMPARE(ab.quietChannels().all(), true);

		// Make left channel of 1st track channel bus contain an Inf, and force the channel to non-quiet
		ab.trackChannelPair(0).frame(5)[0] = std::numeric_limits<float>::infinity();
		ab.quietChannels()[0] = false;

		// Make right channel of 1st track channel bus non-zero too, but using a valid value
		ab.trackChannelPair(0).frame(5)[1] = 1.f;
		ab.quietChannels()[1] = false;

		// Sanitize only the left channel
		ab.sanitize(0b0001);

		// The left channel's buffer should be zeroed, while the right channel should be unaffected
		QCOMPARE(ab.trackChannelPair(0).frame(5)[0], 0.f);
		QCOMPARE(ab.trackChannelPair(0).frame(5)[1], 1.f);
		QCOMPARE(ab.quietChannels()[0], true);
		QCOMPARE(ab.quietChannels()[1], false);

		// Try again
		ab.trackChannelPair(0).frame(5)[0] = std::numeric_limits<float>::infinity();
		ab.quietChannels()[0] = false;

		// This time, sanitize both channels of the 1st track channel bus
		ab.sanitize(0b0011);

		// When the left/right channels are both sanitized, it is allowed to zero both buffers if
		// an Inf/NaN is detected in either channel
		QCOMPARE(ab.trackChannelPair(0).frame(5)[0], 0.f);
		QCOMPARE(ab.trackChannelPair(0).frame(5)[1], 0.f);
		QCOMPARE(ab.quietChannels()[0], true);
		QCOMPARE(ab.quietChannels()[1], true);
	}

	//! Verifies that the `silenceChannels` method works as intended
	void SilenceChannels()
	{
		auto ab = createBus(4, 10);

		// Should have no effect when all buffers are zeroed
		QCOMPARE(ab.quietChannels().all(), true);
		ab.silenceChannels(0b1111);
		QCOMPARE(ab.quietChannels().all(), true);

		// Make left channel of 2nd track channel bus contain a non-zero value, and force the channel to non-quiet
		ab.trackChannelPair(1).frame(5)[0] = 1.f;
		ab.quietChannels()[2] = false;

		// Silence only the left channel
		ab.silenceChannels(0b0100);

		QCOMPARE(ab.quietChannels()[0], true);
		QCOMPARE(ab.quietChannels()[1], true);
		QCOMPARE(ab.quietChannels()[2], true);
		QCOMPARE(ab.quietChannels()[3], true);

		// Make right channel of 2nd track channel bus contain a non-zero value, and force the channel to non-quiet
		ab.trackChannelPair(1).frame(5)[1] = 1.f;
		ab.quietChannels()[3] = false;

		// Silence only the right channel
		ab.silenceChannels(0b1000);

		QCOMPARE(ab.quietChannels()[0], true);
		QCOMPARE(ab.quietChannels()[1], true);
		QCOMPARE(ab.quietChannels()[2], true);
		QCOMPARE(ab.quietChannels()[3], true);

		// Make right channel of 1st track channel bus and both channels of 2nd track channel bus contain
		// a non-zero value, and force those channels to non-quiet
		ab.trackChannelPair(1).frame(5)[1] = 1.f;
		ab.quietChannels()[1] = false;
		ab.trackChannelPair(1).frame(5)[0] = 1.f;
		ab.quietChannels()[2] = false;
		ab.trackChannelPair(1).frame(5)[1] = 1.f;
		ab.quietChannels()[3] = false;

		// Silence both channels of the 2nd track channel bus, plus the already-quiet left channel of the 1st bus
		ab.silenceChannels(0b1101);

		QCOMPARE(ab.quietChannels()[0], true);
		QCOMPARE(ab.quietChannels()[1], false);
		QCOMPARE(ab.quietChannels()[2], true);
		QCOMPARE(ab.quietChannels()[3], true);
	}
};

QTEST_GUILESS_MAIN(AudioBusTest)
#include "AudioBusTest.moc"
