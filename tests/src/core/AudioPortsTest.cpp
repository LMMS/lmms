/*
 * AudioPortsTest.cpp
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

#include <algorithm>
#include <iostream>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QObject>
#include <QtTest/QtTest>

#define LMMS_TESTING
#include "PluginAudioPorts.h"

namespace lmms {

namespace {

template<typename SampleT, proc_ch_t extent>
void zeroBuffer(PlanarBufferView<SampleT, extent> buffer)
{
	for (proc_ch_t idx = 0; idx < buffer.channels(); ++idx)
	{
		auto ptr = buffer.bufferPtr(idx);
		std::fill_n(ptr, buffer.frames(), 0);
	}
}

void zeroBuffer(AudioBus<float> bus)
{
	for (track_ch_t channelPair = 0; channelPair < bus.channelPairs(); ++channelPair)
	{
		auto buffer = bus.trackChannelPair(channelPair);
		std::ranges::fill(buffer.dataView(), 0.f);
	}
}

template<class F>
void transformBuffer(AudioBus<const float> in, AudioBus<float> out, const F& func)
{
	assert(in.channelPairs() == out.channelPairs());
	assert(in.frames() == out.frames());
	for (track_ch_t channelPair = 0; channelPair < in.channelPairs(); ++channelPair)
	{
		for (f_cnt_t sampleIdx = 0; sampleIdx < in.frames() * 2; sampleIdx += 2)
		{
			out[channelPair][sampleIdx]     = func(in[channelPair][sampleIdx]);
			out[channelPair][sampleIdx + 1] = func(in[channelPair][sampleIdx + 1]);
		}
	}
}

template<class F>
void transformBuffer(InterleavedBufferView<float, 2> inOut, const F& func)
{
	for (SampleFrame& sf : inOut.toSampleFrames())
	{
		sf = func(sf);
	}
}

template<typename SampleT, proc_ch_t extent, class F>
void transformBuffer(PlanarBufferView<SampleT, extent> in, PlanarBufferView<SampleT, extent> out, const F& func)
{
	assert(in.channels() == out.channels());
	assert(in.frames() == out.frames());
	for (proc_ch_t idx = 0; idx < in.channels(); ++idx)
	{
		auto inPtr = in.bufferPtr(idx);
		auto outPtr = out.bufferPtr(idx);
		std::transform(inPtr, inPtr + in.frames(), outPtr, func);
	}
}

void compareBuffers(AudioBus<const float> actual, AudioBus<const float> expected)
{
	QCOMPARE(actual.channelPairs(), expected.channelPairs());
	QCOMPARE(actual.frames(), expected.frames());
	for (track_ch_t channelPair = 0; channelPair < actual.channelPairs(); ++channelPair)
	{
		for (f_cnt_t sampleIdx = 0; sampleIdx < actual.frames() * 2; sampleIdx += 2)
		{
			QCOMPARE(actual[channelPair][sampleIdx],     expected[channelPair][sampleIdx]);
			QCOMPARE(actual[channelPair][sampleIdx + 1], expected[channelPair][sampleIdx + 1]);
		}
	}
}

template<typename SampleT, proc_ch_t extent>
void compareBuffers(PlanarBufferView<SampleT, extent> actual, PlanarBufferView<SampleT, extent> expected)
{
	QCOMPARE(actual.channels(), expected.channels());
	QCOMPARE(actual.frames(), expected.frames());
	for (proc_ch_t idx = 0; idx < actual.channels(); ++idx)
	{
		auto actualPtr = actual.bufferPtr(idx);
		auto expectedPtr = expected.bufferPtr(idx);
		for (f_cnt_t frame = 0; frame < actual.frames(); ++frame)
		{
			QCOMPARE(actualPtr[frame], expectedPtr[frame]);
		}
	}
}

class AudioPortsModelImpl : public AudioPortsModel
{
public:
	using AudioPortsModel::AudioPortsModel;

	// TODO: Test the bufferPropertiesChanging method
	auto popBufferPropertiesChangingCount() -> int { return std::exchange(m_bufferChangeCount, 0); }

	auto inputs() const -> proc_ch_t { return m_inputs; }
	auto outputs() const -> proc_ch_t { return m_outputs; }
	auto frames() const -> f_cnt_t { return m_frames; }

	using AudioPortsModel::trackChannelsUpperBound;
	using AudioPortsModel::usedTrackChannels;
	using AudioPortsModel::usedProcessorChannels;
	using AudioPortsModel::directRouting;

private:
	void bufferPropertiesChanging(proc_ch_t inChannels, proc_ch_t outChannels, f_cnt_t frames) override
	{
		++m_bufferChangeCount;
		m_inputs = inChannels;
		m_outputs = outChannels;
		m_frames = frames;
	}

	int m_bufferChangeCount = 0;
	proc_ch_t m_inputs = 0;
	proc_ch_t m_outputs = 0;
	f_cnt_t m_frames = 0;
};

} // namespace
} // namespace lmms


class AudioPortsTest : public QObject
{
	Q_OBJECT

public:
	static constexpr lmms::f_cnt_t MaxFrames = lmms::DEFAULT_BUFFER_SIZE;

private:
	std::vector<float> m_coreBuffer;
	float* m_coreBufferPtr = nullptr;

	auto getCoreBus() -> lmms::AudioBus<float>
	{
		m_coreBuffer.resize(MaxFrames * 2);
		m_coreBufferPtr = m_coreBuffer.data();

		std::fill_n(m_coreBuffer.data(), m_coreBuffer.size(), 0.f);

		return lmms::AudioBus<float>{&m_coreBufferPtr, 1, MaxFrames};
	}

private slots:
	void initTestCase()
	{
		using namespace lmms;
		Engine::init(true);
	}

	void cleanupTestCase()
	{
		using namespace lmms;
		Engine::destroy();
	}

	//! Verifies correct channel counts
	void ChannelCounts()
	{
		using namespace lmms;

		int propertiesChangedCount = 0;
		auto getPropertiesChangedCount = [&]() { return std::exchange(propertiesChangedCount, 0); };

		auto apmNxN = AudioPortsModelImpl{DynamicChannelCount, DynamicChannelCount, false};
		connect(&apmNxN, &AudioPortsModel::propertiesChanged, [&]() { ++propertiesChangedCount; });

		// Channel counts should stay zero until known
		QCOMPARE(apmNxN.in().channelCount(), 0);
		QCOMPARE(apmNxN.out().channelCount(), 0);
		QCOMPARE(getPropertiesChangedCount(), 0);

		apmNxN.setChannelCountIn(4);
		QCOMPARE(apmNxN.in().channelCount(), 4);
		QCOMPARE(apmNxN.out().channelCount(), 0);
		QCOMPARE(getPropertiesChangedCount(), 1);

		apmNxN.setChannelCountOut(8);
		QCOMPARE(apmNxN.in().channelCount(), 4);
		QCOMPARE(apmNxN.out().channelCount(), 8);
		QCOMPARE(getPropertiesChangedCount(), 1);

		// Change both the input and output counts at once
		apmNxN.setChannelCounts(5, 9);
		QCOMPARE(apmNxN.in().channelCount(), 5);
		QCOMPARE(apmNxN.out().channelCount(), 9);
		QCOMPARE(getPropertiesChangedCount(), 1);

		// The track channel count is 2 by default
		QCOMPARE(apmNxN.trackChannelCount(), 2);

		// Can change track channel count
		apmNxN.setTrackChannelCount(4);
		QCOMPARE(apmNxN.trackChannelCount(), 4);
		QCOMPARE(getPropertiesChangedCount(), 1);

		// Track channel count must be a even number (at least for now)
		QVERIFY_EXCEPTION_THROWN(apmNxN.setTrackChannelCount(5), std::exception);
		QCOMPARE(apmNxN.trackChannelCount(), 4);
		QCOMPARE(getPropertiesChangedCount(), 0);

		// Change all channel counts at once
		apmNxN.setAllChannelCounts(2, 3, 4);
		QCOMPARE(apmNxN.trackChannelCount(), 2);
		QCOMPARE(apmNxN.in().channelCount(), 3);
		QCOMPARE(apmNxN.out().channelCount(), 4);
		QCOMPARE(getPropertiesChangedCount(), 1);

		// stereo/stereo effect
		auto apm2x2 = AudioPortsModelImpl{2, 2, false};
		QCOMPARE(apm2x2.in().channelCount(), 2);
		QCOMPARE(apm2x2.out().channelCount(), 2);

		// stereo instrument
		auto apm0x2 = AudioPortsModelImpl{0, 2, true};
		QCOMPARE(apm0x2.in().channelCount(), 0);
		QCOMPARE(apm0x2.out().channelCount(), 2);
	}

	//! Verifies that the correct default connections are used for different channel counts
	void DefaultConnections()
	{
		using namespace lmms;

		// 2 inputs, 2 outputs (stereo/stereo effect)
		//
		// In    Out
		//  ___   ___
		// |X| | |X| |
		// | |X| | |X|
		//  ---   ---

		auto apm2x2 = AudioPortsModelImpl{2, 2, false};
		QCOMPARE(apm2x2.in().enabled(0, 0), true);
		QCOMPARE(apm2x2.in().enabled(0, 1), false);
		QCOMPARE(apm2x2.in().enabled(1, 0), false);
		QCOMPARE(apm2x2.in().enabled(1, 1), true);

		QCOMPARE(apm2x2.out().enabled(0, 0), true);
		QCOMPARE(apm2x2.out().enabled(0, 1), false);
		QCOMPARE(apm2x2.out().enabled(1, 0), false);
		QCOMPARE(apm2x2.out().enabled(1, 1), true);

		// 1 input, 1 output (mono/mono effect)
		//
		// In    Out
		//  _     _
		// |X|   |X|
		// | |   |X|
		//  -     -

		auto apm1x1 = AudioPortsModelImpl{1, 1, false};
		QCOMPARE(apm1x1.in().enabled(0, 0), true);
		QCOMPARE(apm1x1.in().enabled(1, 0), false);

		QCOMPARE(apm1x1.out().enabled(0, 0), true);
		QCOMPARE(apm1x1.out().enabled(1, 0), true);

		// 1 input, >2 outputs
		//
		// In    Out
		//  _     _______
		// |X|   |X| | | |
		// | |   | |X| | |
		//  -     -------

		auto apm1x4 = AudioPortsModelImpl{1, 4, false};
		QCOMPARE(apm1x4.in().enabled(0, 0), true);
		QCOMPARE(apm1x4.in().enabled(1, 0), false);

		QCOMPARE(apm1x4.out().enabled(0, 0), true);
		QCOMPARE(apm1x4.out().enabled(0, 1), false);
		QCOMPARE(apm1x4.out().enabled(0, 2), false);
		QCOMPARE(apm1x4.out().enabled(0, 3), false);
		QCOMPARE(apm1x4.out().enabled(1, 0), false);
		QCOMPARE(apm1x4.out().enabled(1, 1), true);
		QCOMPARE(apm1x4.out().enabled(1, 2), false);
		QCOMPARE(apm1x4.out().enabled(1, 3), false);

		// 2 inputs, 2 outputs (stereo instrument with stereo sidechain input)
		//
		// In    Out
		//  ___   ___
		// | | | |X| |
		// | | | | |X|
		//  ---   ---

		auto apm2x2Inst = AudioPortsModelImpl{2, 2, true};
		QCOMPARE(apm2x2Inst.in().enabled(0, 0), false);
		QCOMPARE(apm2x2Inst.in().enabled(0, 1), false);
		QCOMPARE(apm2x2Inst.in().enabled(1, 0), false);
		QCOMPARE(apm2x2Inst.in().enabled(1, 1), false);

		QCOMPARE(apm2x2Inst.out().enabled(0, 0), true);
		QCOMPARE(apm2x2Inst.out().enabled(0, 1), false);
		QCOMPARE(apm2x2Inst.out().enabled(1, 0), false);
		QCOMPARE(apm2x2Inst.out().enabled(1, 1), true);
	}

	//! Verifies that the used track channels cache works
	void UsedTrackChannelsCache()
	{
		using namespace lmms;

		// Setup
		auto apm = AudioPortsModelImpl{2, 2, false};

		// Out
		//  ___
		// |X| | 0
		// | |X| 1
		//  ---

		// Track channels 0 and 1 should both have a processor output channel routed to them
		QCOMPARE(apm.usedTrackChannels()[0], true);
		QCOMPARE(apm.usedTrackChannels()[1], true);

		// Out
		//  ___
		// | | | 0
		// | |X| 1
		//  ---

		apm.out().setPin(0, 0, false);

		// Now only track channel 1 should have a processor channel routed to it
		QCOMPARE(apm.usedTrackChannels()[0], false);
		QCOMPARE(apm.usedTrackChannels()[1], true);

		// Out
		//  ___
		// | |X| 0
		// | |X| 1
		//  ---

		apm.out().setPin(0, 1, true);

		QCOMPARE(apm.usedTrackChannels()[0], true);
		QCOMPARE(apm.usedTrackChannels()[1], true);

		// Out
		//  ___
		// | |X| 0
		// |X|X| 1
		//  ---

		apm.out().setPin(1, 0, true);

		QCOMPARE(apm.usedTrackChannels()[0], true);
		QCOMPARE(apm.usedTrackChannels()[1], true);
	}

	//! Verifies that the used processor channels cache works
	void UsedProcessorChannelsCache()
	{
		using namespace lmms;

		// Setup
		auto apm = AudioPortsModelImpl{2, 2, false};

		// Out
		//  ___
		// |X| | 0
		// | |X| 1
		//  ---

		// Processor channels 0 and 1 should both be routed to a track channel
		QCOMPARE(apm.usedProcessorChannels()[0], true);
		QCOMPARE(apm.usedProcessorChannels()[1], true);

		// Out
		//  ___
		// | | | 0
		// | |X| 1
		//  ---

		apm.out().setPin(0, 0, false);

		// Now only processor channel 1 should be routed to a track channel
		QCOMPARE(apm.usedProcessorChannels()[0], false);
		QCOMPARE(apm.usedProcessorChannels()[1], true);

		// Out
		//  ___
		// | |X| 0
		// | |X| 1
		//  ---

		apm.out().setPin(0, 1, true);

		QCOMPARE(apm.usedProcessorChannels()[0], false);
		QCOMPARE(apm.usedProcessorChannels()[1], true);

		// Out
		//  ___
		// | |X| 0
		// | |X| 1
		// | | | 2
		// | | | 3
		//  ---

		apm.setTrackChannelCount(4);

		// Should remain the same after increasing the number of track channels
		QCOMPARE(apm.usedProcessorChannels()[0], false);
		QCOMPARE(apm.usedProcessorChannels()[1], true);

		// Out
		//  ___
		// | |X| 0
		// | |X| 1
		// | | | 2
		// |X| | 3
		//  ---

		apm.out().setPin(3, 0, true);

		QCOMPARE(apm.usedProcessorChannels()[0], true);
		QCOMPARE(apm.usedProcessorChannels()[1], true);

		// Out
		//  ___
		// | |X| 0
		// | |X| 1
		//  ---

		apm.setTrackChannelCount(2);

		// The 1st processor channel should no longer be routed to any track channels
		QCOMPARE(apm.usedProcessorChannels()[0], false);
		QCOMPARE(apm.usedProcessorChannels()[1], true);

		// Out
		//  _____
		// | |X| | 0
		// | |X| | 1
		//  -----

		apm.setChannelCountOut(3);

		// The new processor channel should not be routed to anything, and the rest stays the same
		QCOMPARE(apm.usedProcessorChannels()[0], false);
		QCOMPARE(apm.usedProcessorChannels()[1], true);
		QCOMPARE(apm.usedProcessorChannels()[2], false);

		apm.setChannelCountIn(1);

		// Setting the input channel count has no effect
		QCOMPARE(apm.usedProcessorChannels()[0], false);
		QCOMPARE(apm.usedProcessorChannels()[1], true);
		QCOMPARE(apm.usedProcessorChannels()[2], false);
	}

	//! Verifies that the direct routing optimization works
	void DirectRoutingOptimization()
	{
		using namespace lmms;

		// Setup
		auto apm = AudioPortsModelImpl{2, 2, false};

		// In    Out
		//  ___   ___
		// |X| | |X| |
		// | |X| | |X|
		//  ---   ---

		// The default pin connections for a 2x2 audio processor should allow direct routing
		QCOMPARE(apm.directRouting().value_or(99), 0);

		// Should still work after increasing the track channel count
		apm.setTrackChannelCount(4);
		QCOMPARE(apm.trackChannelCount(), 4);
		QCOMPARE(apm.directRouting().value_or(99), 0);

		// In    Out
		//  ___   ___
		// | | | |X| |
		// | |X| | |X|
		// | | | | | |
		// | | | | | |
		//  ---   ---

		// Disabling a pin should prevent the optimization
		apm.in().setPin(0, 0, false);
		QCOMPARE(apm.directRouting().has_value(), false);

		// In    Out
		//  ___   ___
		// | | | |X| |
		// | | | | |X|
		// |X| | | | |
		// | |X| | | |
		//  ---   ---

		// The direct routing optimization requires the same track channel pairs on the input and output sides
		apm.in().setPin(1, 1, false);
		apm.in().setPin(2, 0, true);
		apm.in().setPin(3, 1, true);
		QCOMPARE(apm.directRouting().has_value(), false);

		// In    Out
		//  ___   ___
		// | | | | | |
		// | | | | | |
		// |X| | |X| |
		// | |X| | |X|
		//  ---   ---

		// But if the output side is also moved down, should be able to directly route the 2nd track channel pair
		apm.out().setPin(0, 0, false);
		apm.out().setPin(1, 1, false);
		apm.out().setPin(2, 0, true);
		apm.out().setPin(3, 1, true);
		QCOMPARE(apm.directRouting().value_or(99), 1);

		// Out
		//  ___
		// | | |
		// | | |
		// |X| |
		// | |X|
		//  ---

		// If processor input channels are removed, the optimization should still apply
		apm.setChannelCountIn(0);
		QCOMPARE(apm.directRouting().value_or(99), 1);

		// Out
		//  _____
		// | | | |
		// | | | |
		// |X| | |
		// | |X| |
		//  -----

		// Adding a 3rd output channel should disable the optimization (only 0 or 2 channels allowed)
		apm.setChannelCountOut(3);
		QCOMPARE(apm.directRouting().has_value(), false);
	}

	//! Verifies correct default routing for 1x1 non-interleaved audio processor
	void Routing_NonInterleaved1x1_Default()
	{
		using namespace lmms;

		// Setup
		constexpr auto settings = AudioPortsSettings{AudioDataKind::F32, false, 1, 1};
		auto ap = PluginAudioPorts<settings>{false};
		ap.init();
		auto coreBus = getCoreBus();

		// Use left channel as processor input, upmix mono processor output to stereo
		// In    Out
		//  _     _
		// |X|   |X|
		// | |   |X|
		//  -     -

		// NOTE: If both channels were connected to the mono input, the signals would
		//       be summed together and the amplitude would be doubled which is
		//       undesirable, so the pin connector uses only the left channel as the
		//       processor input, following what REAPER does by default.

		// Data on frames 0, 1, and 33
		auto trackChannels = coreBus.trackChannelPair(0); // channels 0/1
		trackChannels.sampleFrameAt(0).setLeft(123.f);
		trackChannels.sampleFrameAt(0).setRight(321.f);
		trackChannels.sampleFrameAt(1).setLeft(456.f);
		trackChannels.sampleFrameAt(1).setRight(654.f);
		trackChannels.sampleFrameAt(33).setLeft(789.f);
		trackChannels.sampleFrameAt(33).setRight(987.f);

		// Processor input and output buffers
		auto ins = ap.buffers()->input();
		auto outs = ap.buffers()->output();

		// Route to processor
		auto router = ap.getRouter();
		router.send(coreBus, ins);

		// Check that processor inputs have data on frames 0, 1, and 33 (should be left channel's data)
		QCOMPARE(ins.buffer(0)[0], 123.f);
		QCOMPARE(ins.buffer(0)[1], 456.f);
		QCOMPARE(ins.buffer(0)[33], 789.f);

		// Do work of processImpl - in this case it doubles the amplitude
		transformBuffer(ins, outs, [](auto s) { return s * 2; });

		// Construct buffer with the expected core bus result
		auto coreBufferExpected = std::vector<float>(MaxFrames * 2);
		auto coreBufferPtrExpected = coreBufferExpected.data();
		auto coreBusExpected = AudioBus<float>{&coreBufferPtrExpected, 1, MaxFrames};
		coreBusExpected.trackChannelPair(0).sampleFrameAt(0) = SampleFrame{123.f * 2, 123.f * 2};
		coreBusExpected.trackChannelPair(0).sampleFrameAt(1) = SampleFrame{456.f * 2, 456.f * 2};
		coreBusExpected.trackChannelPair(0).sampleFrameAt(33) = SampleFrame{789.f * 2, 789.f * 2};

		// Route from processor back to Core
		router.receive(outs, coreBus);

		// Check that result is the original left track channel with doubled amplitude
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[0], 123.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[1], 123.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[0], 456.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[1], 456.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[0], 789.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[1], 789.f * 2);

		// Test the rest of the buffer
		compareBuffers(coreBus, coreBusExpected);
	}

	//! Verifies correct default routing for 2x2 non-interleaved audio processor
	void Routing_NonInterleaved2x2_Default()
	{
		using namespace lmms;

		// Setup
		constexpr auto settings = AudioPortsSettings{AudioDataKind::F32, false, 2, 2};
		auto ap = PluginAudioPorts<settings>{false};
		ap.init();
		auto coreBus = getCoreBus();

		// Data on frames 0, 1, and 33
		auto trackChannels = coreBus.trackChannelPair(0); // channels 0/1
		trackChannels.sampleFrameAt(0).setLeft(123.f);
		trackChannels.sampleFrameAt(0).setRight(321.f);
		trackChannels.sampleFrameAt(1).setLeft(456.f);
		trackChannels.sampleFrameAt(1).setRight(654.f);
		trackChannels.sampleFrameAt(33).setLeft(789.f);
		trackChannels.sampleFrameAt(33).setRight(987.f);

		// Processor input and output buffers
		auto ins = ap.buffers()->input();
		auto outs = ap.buffers()->output();

		// Route to processor
		auto router = ap.getRouter();
		router.send(coreBus, ins);

		// Check that processor inputs have data on frames 0, 1, and 33
		QCOMPARE(ins.buffer(0)[0], 123.f);
		QCOMPARE(ins.buffer(1)[0], 321.f);
		QCOMPARE(ins.buffer(0)[1], 456.f);
		QCOMPARE(ins.buffer(1)[1], 654.f);
		QCOMPARE(ins.buffer(0)[33], 789.f);
		QCOMPARE(ins.buffer(1)[33], 987.f);

		// Do work of processImpl - in this case it doubles the amplitude
		transformBuffer(ins, outs, [](auto s) { return s * 2; });

		// Construct buffer with the expected core bus result
		auto coreBufferExpected = std::vector<float>(MaxFrames * 2);
		auto coreBufferPtrExpected = coreBufferExpected.data();
		auto coreBusExpected = AudioBus<float>{&coreBufferPtrExpected, 1, MaxFrames};
		transformBuffer(coreBus, coreBusExpected, [](auto s) { return s * 2; });

		// Sanity check for transformBuffer
		QCOMPARE(outs.buffer(0)[0], 123.f * 2);
		QCOMPARE(outs.buffer(1)[0], 321.f * 2);
		QCOMPARE(outs.buffer(0)[1], 456.f * 2);
		QCOMPARE(outs.buffer(1)[1], 654.f * 2);
		QCOMPARE(outs.buffer(0)[33], 789.f * 2);
		QCOMPARE(outs.buffer(1)[33], 987.f * 2);

		// Zero core bus just to be sure what the processor output is
		lmms::zeroBuffer(coreBus);

		// Route from processor back to Core
		router.receive(outs, coreBus);

		// Should be double the original
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[0], 123.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[1], 321.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[0], 456.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[1], 654.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[0], 789.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[1], 987.f * 2);

		// Test the rest of the buffer
		compareBuffers(coreBus, coreBusExpected);
	}

	//! Verifies correct partially-bypassed routing for 2x2 non-interleaved audio processor
	void Routing_NonInterleaved2x2_Bypass()
	{
		using namespace lmms;

		// Setup
		constexpr auto settings = AudioPortsSettings{AudioDataKind::F32, false, 2, 2};
		auto ap = PluginAudioPorts<settings>{false};
		ap.init();
		auto coreBus = getCoreBus();

		// Default input connections, disable right output channel
		// In    Out
		//  ___   ___
		// |X| | |X| |
		// | |X| | | |
		//  ---   ---
		auto& apm = ap.model();
		apm.in().setPin(0, 0, true);
		apm.in().setPin(0, 1, false);
		apm.in().setPin(1, 0, false);
		apm.in().setPin(1, 1, true);
		apm.out().setPin(0, 0, true);
		apm.out().setPin(0, 1, false);
		apm.out().setPin(1, 0, false);
		apm.out().setPin(1, 1, false);

		// Data on frames 0, 1, and 33
		auto trackChannels = coreBus.trackChannelPair(0); // channels 0/1
		trackChannels.sampleFrameAt(0).setLeft(123.f);
		trackChannels.sampleFrameAt(0).setRight(321.f);
		trackChannels.sampleFrameAt(1).setLeft(456.f);
		trackChannels.sampleFrameAt(1).setRight(654.f);
		trackChannels.sampleFrameAt(33).setLeft(789.f);
		trackChannels.sampleFrameAt(33).setRight(987.f);

		// Processor input and output buffers
		auto ins = ap.buffers()->input();
		auto outs = ap.buffers()->output();

		// Route to processor
		auto router = ap.getRouter();
		router.send(coreBus, ins);

		// Check that processor inputs have data on frames 0, 1, and 33
		QCOMPARE(ins.buffer(0)[0], 123.f);
		QCOMPARE(ins.buffer(1)[0], 321.f);
		QCOMPARE(ins.buffer(0)[1], 456.f);
		QCOMPARE(ins.buffer(1)[1], 654.f);
		QCOMPARE(ins.buffer(0)[33], 789.f);
		QCOMPARE(ins.buffer(1)[33], 987.f);

		// Do work of processImpl - in this case it doubles the amplitude
		transformBuffer(ins, outs, [](auto s) { return s * 2; });

		// Construct buffer with the expected core bus result
		auto coreBufferExpected = std::vector<float>(MaxFrames * 2);
		auto coreBufferPtrExpected = coreBufferExpected.data();
		auto coreBusExpected = AudioBus<float>{&coreBufferPtrExpected, 1, MaxFrames};
		for (f_cnt_t sampleIdx = 0; sampleIdx < coreBus.frames() * 2; sampleIdx += 2)
		{
			float& sampleL = coreBusExpected[0][sampleIdx];
			sampleL = coreBus[0][sampleIdx] * 2; // left channel:  doubled output from processor

			float& sampleR = coreBusExpected[0][sampleIdx + 1];
			sampleR = coreBus[0][sampleIdx + 1]; // right channel: bypassed
		}

		// Route from processor back to Core
		router.receive(outs, coreBus);

		// Right track channel should pass through, but left track channel
		// should be overwritten with processor's left output channel
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[0], 123.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[1], 321.f);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[0], 456.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[1], 654.f);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[0], 789.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[1], 987.f);

		// Test the rest of the buffer
		compareBuffers(coreBus, coreBusExpected);
	}

	//! Verifies correct default routing for 2x2 interleaved audio processor
	void Routing_Interleaved2x2_Default()
	{
		using namespace lmms;

		// Setup
		constexpr auto settings = AudioPortsSettings {
			AudioDataKind::F32, true, 2, 2, true
		};
		auto ap = PluginAudioPorts<settings>{false};
		ap.init();
		auto coreBus = getCoreBus();

		// Data on frames 0, 1, and 33
		auto trackChannels = coreBus.trackChannelPair(0); // channels 0/1
		trackChannels.sampleFrameAt(0).setLeft(123.f);
		trackChannels.sampleFrameAt(0).setRight(321.f);
		trackChannels.sampleFrameAt(1).setLeft(456.f);
		trackChannels.sampleFrameAt(1).setRight(654.f);
		trackChannels.sampleFrameAt(33).setLeft(789.f);
		trackChannels.sampleFrameAt(33).setRight(987.f);

		// Processor input/output buffer
		auto inOut = ap.buffers()->inputOutput();

		// Route to processor
		auto router = ap.getRouter();
		router.send(coreBus, inOut);

		// Check that processor inputs have data on frames 0, 1, and 33
		QCOMPARE(inOut[0][0], 123.f);
		QCOMPARE(inOut[0][1], 321.f);
		QCOMPARE(inOut[1][0], 456.f);
		QCOMPARE(inOut[1][1], 654.f);
		QCOMPARE(inOut[33][0], 789.f);
		QCOMPARE(inOut[33][1], 987.f);

		// Do work of processImpl - in this case it doubles the amplitude
		transformBuffer(inOut, [](auto s) { return s * 2; });

		// Construct buffer with the expected core bus result
		auto coreBufferExpected = std::vector<float>(MaxFrames * 2);
		auto coreBufferPtrExpected = coreBufferExpected.data();
		auto coreBusExpected = AudioBus<float>{&coreBufferPtrExpected, 1, MaxFrames};
		transformBuffer(coreBus, coreBusExpected, [](auto s) { return s * 2; });

		// Zero core bus just to be sure what the processor output is
		zeroBuffer(coreBus);

		// Route from processor back to Core
		router.receive(inOut, coreBus);

		// Should be double the original
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[0], 123.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[1], 321.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[0], 456.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[1], 654.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[0], 789.f * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[1], 987.f * 2);

		// Test the rest of the buffer
		compareBuffers(coreBus, coreBusExpected);
	}

	//! Verifies correct signal summing when routing a 1x2 non-interleaved audio processor
	void Routing_NonInterleaved1x2_Sum()
	{
		using namespace lmms;

		// Setup
		constexpr auto settings = AudioPortsSettings{AudioDataKind::F32, false, 1, 2};
		auto ap = PluginAudioPorts<settings>{false};
		ap.init();
		auto coreBus = getCoreBus();

		// Sum both track channels together for processor input, and sum the
		//     two processor output channels together for the left track channel output
		// In    Out
		//  _     ___
		// |X|   |X|X|
		// |X|   | | |
		//  -     ---
		auto& apm = ap.model();
		apm.in().setPin(0, 0, true);
		apm.in().setPin(1, 0, true);
		apm.out().setPin(0, 0, true);
		apm.out().setPin(0, 1, true);
		apm.out().setPin(1, 0, false);
		apm.out().setPin(1, 1, false);

		// Data on frames 0, 1, and 33
		auto trackChannels = coreBus.trackChannelPair(0); // channels 0/1
		trackChannels.sampleFrameAt(0).setLeft(123.f);
		trackChannels.sampleFrameAt(0).setRight(321.f);
		trackChannels.sampleFrameAt(1).setLeft(456.f);
		trackChannels.sampleFrameAt(1).setRight(654.f);
		trackChannels.sampleFrameAt(33).setLeft(789.f);
		trackChannels.sampleFrameAt(33).setRight(987.f);

		// Processor input and output buffers
		auto ins = ap.buffers()->input();
		auto outs = ap.buffers()->output();

		// Route to processor
		auto router = ap.getRouter();
		router.send(coreBus, ins);

		// Check that processor inputs have data on frames 0, 1, and 33 (should be both track channels summed together)
		QCOMPARE(ins.buffer(0)[0], 123.f + 321.f);
		QCOMPARE(ins.buffer(0)[1], 456.f + 654.f);
		QCOMPARE(ins.buffer(0)[33], 789.f + 987.f);

		// Do work of processImpl - in this case it does nothing (passthrough)
		const auto process = [](auto s) { return s; };
		std::transform(ins.bufferPtr(0), ins.bufferPtr(0) + ins.frames(), outs.bufferPtr(0), process);
		std::transform(ins.bufferPtr(0), ins.bufferPtr(0) + ins.frames(), outs.bufferPtr(1), process);

		// Construct buffer with the expected core bus result
		auto coreBufferExpected = std::vector<float>(MaxFrames * 2);
		auto coreBufferPtrExpected = coreBufferExpected.data();
		auto coreBusExpected = AudioBus<float>{&coreBufferPtrExpected, 1, MaxFrames};
		coreBusExpected.trackChannelPair(0).sampleFrameAt(0) = SampleFrame{(123.f + 321.f) * 2, 321.f};
		coreBusExpected.trackChannelPair(0).sampleFrameAt(1) = SampleFrame{(456.f + 654.f) * 2, 654.f};
		coreBusExpected.trackChannelPair(0).sampleFrameAt(33) = SampleFrame{(789.f + 987.f) * 2, 987.f};

		// Route from processor back to Core
		router.receive(outs, coreBus);

		// Check that result is the two original track channels added together then doubled
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[0], (123.f + 321.f) * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(0)[1], 321.f);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[0], (456.f + 654.f) * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(1)[1], 654.f);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[0], (789.f + 987.f) * 2);
		QCOMPARE(coreBus.trackChannelPair(0).frame(33)[1], 987.f);

		// Test the rest of the buffer
		compareBuffers(coreBus, coreBusExpected);
	}

	//! Verifies correct routing when the direct routing optimization is active
	void Routing_Interleaved2x2_DirectRouting()
	{
		using namespace lmms;

		// Helper for running this test with different settings
		auto testWithSettings = [&]<AudioPortsSettings settings>(PluginAudioPorts<settings>& ap) {
			ap.init();

			// Data on frames 0, 1, and 33
			auto coreBus = getCoreBus();
			auto trackChannels = coreBus.trackChannelPair(0); // channels 0/1
			trackChannels.sampleFrameAt(0).setLeft(123.f);
			trackChannels.sampleFrameAt(0).setRight(321.f);
			trackChannels.sampleFrameAt(1).setLeft(456.f);
			trackChannels.sampleFrameAt(1).setRight(654.f);
			trackChannels.sampleFrameAt(33).setLeft(789.f);
			trackChannels.sampleFrameAt(33).setRight(987.f);

			// Construct buffer with the expected core bus result
			auto coreBufferExpected = std::vector<float>(MaxFrames * 2);
			auto coreBufferPtrExpected = coreBufferExpected.data();
			auto coreBusExpected = AudioBus<float>{&coreBufferPtrExpected, 1, MaxFrames};
			transformBuffer(coreBus, coreBusExpected, [](auto s) { return s * 2; });

			// Audio processor's process method that doubles the amplitude. Works for any AudioPortsSettings.
			struct Process
			{
				PluginAudioPorts<settings>& ap;

				// Statically in-place settings
				auto operator()(InterleavedBufferView<float, 2> inOut)
				{
					for (auto& s : inOut.toSampleFrames()) { s *= 2; }
					return ProcessStatus::Continue;
				}

				// Dynamically in-place settings
				auto operator()(InterleavedBufferView<const float, 2> in, InterleavedBufferView<float, 2> out)
				{
					for (std::size_t frame = 0; frame < in.frames(); ++frame)
					{
						out[frame][0] = in[frame][0] * 2;
						out[frame][1] = in[frame][1] * 2;
					}
					return ProcessStatus::Continue;
				}
			};

			QCOMPARE(ap.directRouting().value_or(99), 0);

			// Use the Router::process method which handles routing into and out of the audio processor,
			// and calls the processor's process method we provide (in this case it doubles the amplitude).
			// Also applies the "direct routing" optimization.
			auto router = ap.getRouter();
			router.process(coreBus, *ap.buffers(), Process{ap});

			// Should be double the original
			QCOMPARE(coreBus.trackChannelPair(0).frame(0)[0], 123.f * 2);
			QCOMPARE(coreBus.trackChannelPair(0).frame(0)[1], 321.f * 2);
			QCOMPARE(coreBus.trackChannelPair(0).frame(1)[0], 456.f * 2);
			QCOMPARE(coreBus.trackChannelPair(0).frame(1)[1], 654.f * 2);
			QCOMPARE(coreBus.trackChannelPair(0).frame(33)[0], 789.f * 2);
			QCOMPARE(coreBus.trackChannelPair(0).frame(33)[1], 987.f * 2);

			// Test the rest of the buffer
			compareBuffers(coreBus, coreBusExpected);
		};

		// Test statically in-place, non-buffered
		{
			constexpr auto settings = AudioPortsSettings {
				AudioDataKind::F32, true, 2, 2, true, false
			};
			auto ap = PluginAudioPorts<settings>{false};
			testWithSettings(ap);
		}

		// Test statically in-place, buffered
		{
			constexpr auto settings = AudioPortsSettings {
				AudioDataKind::F32, true, 2, 2, true, true
			};
			auto ap = PluginAudioPorts<settings>{false};
			testWithSettings(ap);
		}

		// TODO: If/when interleaved audio processors support dynamically in-place
		//       processing, add those tests here
	}

	//! Verifies correct saving and loading of model
	void SaveLoad()
	{
		using namespace lmms;

		// Setup
		auto apm = AudioPortsModelImpl{2, 4, false};

		/*
		// For debugging
		auto print = [](const AudioPortsModel& m) {
			for (track_ch_t tc = 0; tc < m.trackChannelCount(); ++tc) {
				for (proc_ch_t pc = 0; pc < m.in().channelCount(); ++pc) {
					std::cout << (m.in().enabled(tc, pc) ? 'X' : 'O');
				}
				std::cout << "    ";
				for (proc_ch_t pc = 0; pc < m.out().channelCount(); ++pc) {
					std::cout << (m.out().enabled(tc, pc) ? 'X' : 'O');
				}
				std::cout << '\n';
			}
		};
		*/

		// In    Out
		//  ___   _______
		// | |X| | |X| | |
		// |X| | |X| |X|X|
		//  ---   -------

		apm.in().setPin(0, 0, false);
		apm.in().setPin(1, 1, false);
		apm.in().setPin(0, 1, true);
		apm.in().setPin(1, 0, true);

		apm.out().setPin(0, 0, false);
		apm.out().setPin(1, 1, false);
		apm.out().setPin(0, 1, true);
		apm.out().setPin(1, 0, true);
		apm.out().setPin(0, 2, false);
		apm.out().setPin(1, 3, false);
		apm.out().setPin(0, 3, true);
		apm.out().setPin(1, 2, true);

		// Save model
		auto doc = QDomDocument{"test-document"};
		auto elem = doc.createElement("test-element");
		apm.saveSettings(doc, elem);

		// New model with wrong channel counts and wrong pin connections
		auto apm2 = AudioPortsModelImpl{1, 1, false};

		int dataChangedCount = 0;
		int propertiesChangedCount = 0;
		connect(&apm2, &AudioPortsModel::dataChanged, [&]() { ++dataChangedCount; });
		connect(&apm2, &AudioPortsModel::propertiesChanged, [&]() { ++propertiesChangedCount; });

		// Load old model's settings into new model
		apm2.loadSettings(elem);

		Q_ASSERT(apm2.in().channelCount() == 2);
		Q_ASSERT(apm2.out().channelCount() == 4);

		Q_ASSERT(apm2.in().enabled(0, 0) == false);
		Q_ASSERT(apm2.in().enabled(1, 1) == false);
		Q_ASSERT(apm2.in().enabled(0, 1) == true);
		Q_ASSERT(apm2.in().enabled(1, 0) == true);

		Q_ASSERT(apm2.out().enabled(0, 0) == false);
		Q_ASSERT(apm2.out().enabled(1, 1) == false);
		Q_ASSERT(apm2.out().enabled(0, 1) == true);
		Q_ASSERT(apm2.out().enabled(1, 0) == true);
		Q_ASSERT(apm2.out().enabled(0, 2) == false);
		Q_ASSERT(apm2.out().enabled(1, 3) == false);
		Q_ASSERT(apm2.out().enabled(0, 3) == true);
		Q_ASSERT(apm2.out().enabled(1, 2) == true);

		// The `dataChanged` signal should only be emitted once
		Q_ASSERT(dataChangedCount == 1);

		// The `propertiesChanged` signal should only be emitted once
		Q_ASSERT(propertiesChangedCount == 1);
	}
};

QTEST_GUILESS_MAIN(AudioPortsTest)
#include "AudioPortsTest.moc"
