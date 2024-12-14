/*
 * PluginPinConnectorTest.cpp
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include <QDir>
#include <QObject>
#include <QtTest/QtTest>
#include <algorithm>
#include <qtestcase.h>

#define LMMS_TESTING
#include "AudioData.h"
#include "AudioEngine.h"
#include "AudioPluginBuffer.h"
#include "Model.h"
#include "PluginPinConnector.h"
#include "SampleFrame.h"
#include "lmms_basics.h"


namespace lmms {

namespace {

/*
void zeroBuffer(CoreAudioDataMut buffer)
{
	zeroSampleFrames(buffer.data(), buffer.size());
}*/

template<typename SampleT, int extent>
void zeroBuffer(SplitAudioData<SampleT, extent> buffer)
{
	for (pi_ch_t idx = 0; idx < buffer.channels(); ++idx)
	{
		auto ptr = buffer.buffer(idx);
		std::fill_n(ptr, buffer.frames(), 0);
	}
}

void zeroBuffer(CoreAudioBusMut bus)
{
	for (ch_cnt_t channelPair = 0; channelPair < bus.channelPairs; ++channelPair)
	{
		SampleFrame* buffer = bus.bus[channelPair];
		zeroSampleFrames(buffer, bus.frames);
	}
}

template<class F>
void transformBuffer(CoreAudioBus in, CoreAudioBusMut out, const F& func)
{
	assert(in.channelPairs == out.channelPairs);
	assert(in.frames == out.frames);
	for (ch_cnt_t channelPair = 0; channelPair < in.channelPairs; ++channelPair)
	{
		for (std::size_t frame = 0; frame < in.frames; ++frame)
		{
			out.bus[channelPair][frame].leftRef() = func(in.bus[channelPair][frame].left());
			out.bus[channelPair][frame].rightRef() = func(in.bus[channelPair][frame].right());
		}
	}
}

template<class F>
void transformBuffer(CoreAudioData in, CoreAudioDataMut out, const F& func)
{
	assert(in.size() == out.size());
	for (std::size_t frame = 0; frame < in.size(); ++frame)
	{
		out[frame] = func(in[frame]);
	}
}

template<typename SampleT, int extent, class F>
void transformBuffer(SplitAudioData<SampleT, extent> in, SplitAudioData<SampleT, extent> out, const F& func)
{
	assert(in.channels() == out.channels());
	assert(in.frames() == out.frames());
	for (pi_ch_t idx = 0; idx < in.channels(); ++idx)
	{
		auto inPtr = in.buffer(idx);
		auto outPtr = out.buffer(idx);
		std::transform(inPtr, inPtr + in.frames(), outPtr, func);
	}
}

void compareBuffers(CoreAudioBus actual, CoreAudioBus expected)
{
	QCOMPARE(actual.channelPairs, expected.channelPairs);
	QCOMPARE(actual.frames, expected.frames);
	for (ch_cnt_t channelPair = 0; channelPair < actual.channelPairs; ++channelPair)
	{
		for (std::size_t frame = 0; frame < actual.frames; ++frame)
		{
			QCOMPARE(actual.bus[channelPair][frame].left(), expected.bus[channelPair][frame].left());
			QCOMPARE(actual.bus[channelPair][frame].right(), expected.bus[channelPair][frame].right());
		}
	}
}

/*
void compareBuffers(CoreAudioData actual, CoreAudioData expected)
{
	QCOMPARE(actual.size(), expected.size());
	for (std::size_t frame = 0; frame < actual.size(); ++frame)
	{
		QCOMPARE(actual[frame].left(), expected[frame].left());
		QCOMPARE(actual[frame].right(), expected[frame].right());
	}
}*/

template<typename SampleT, int extent>
void compareBuffers(SplitAudioData<SampleT, extent> actual, SplitAudioData<SampleT, extent> expected)
{
	QCOMPARE(actual.channels(), expected.channels());
	QCOMPARE(actual.frames(), expected.frames());
	for (pi_ch_t idx = 0; idx < actual.channels(); ++idx)
	{
		auto actualPtr = actual.buffer(idx);
		auto expectedPtr = expected.buffer(idx);
		for (f_cnt_t frame = 0; frame < actual.frames(); ++frame)
		{
			QCOMPARE(actualPtr[frame], expectedPtr[frame]);
		}
	}
}

} // namespace
} // namespace lmms


class PluginPinConnectorTest : public QObject
{
	Q_OBJECT

public:
	static constexpr lmms::f_cnt_t MaxFrames = lmms::DEFAULT_BUFFER_SIZE;

private:
	std::vector<lmms::SampleFrame> m_coreBuffer;
	lmms::SampleFrame* m_coreBufferPtr = nullptr;

	auto getCoreBus() -> lmms::CoreAudioBusMut
	{
		m_coreBuffer.resize(MaxFrames);
		m_coreBufferPtr = m_coreBuffer.data();

		std::fill_n(m_coreBuffer.data(), m_coreBuffer.size(), lmms::SampleFrame{});

		return lmms::CoreAudioBusMut{&m_coreBufferPtr, 1, MaxFrames};
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

		auto model = Model{nullptr};

		// Channel counts should stay zero until known
		auto pcNxN = PluginPinConnector{DynamicChannelCount, DynamicChannelCount, &model};
		QCOMPARE(pcNxN.in().channelCount(), 0);
		QCOMPARE(pcNxN.out().channelCount(), 0);

		pcNxN.setPluginChannelCountIn(4);
		QCOMPARE(pcNxN.in().channelCount(), 4);
		QCOMPARE(pcNxN.out().channelCount(), 0);

		pcNxN.setPluginChannelCountOut(8);
		QCOMPARE(pcNxN.in().channelCount(), 4);
		QCOMPARE(pcNxN.out().channelCount(), 8);

		// stereo/stereo effect
		auto pc2x2 = PluginPinConnector{2, 2, &model};
		QCOMPARE(pc2x2.in().channelCount(), 2);
		QCOMPARE(pc2x2.out().channelCount(), 2);

		// stereo instrument
		auto pc0x2 = PluginPinConnector{0, 2, &model};
		QCOMPARE(pc0x2.in().channelCount(), 0);
		QCOMPARE(pc0x2.out().channelCount(), 2);
	}

	//! Verifies that the correct default connections are used for different channel counts
	void DefaultConnections()
	{
		using namespace lmms;

		auto model = Model{nullptr};

		// 2 inputs, 2 outputs (stereo/stereo effect)
		//
		// In    Out
		//  ___   ___
		// |X| | |X| |
		// | |X| | |X|
		//  ---   ---

		auto pc2x2 = PluginPinConnector{2, 2, &model};
		QCOMPARE(pc2x2.in().enabled(0, 0), true);
		QCOMPARE(pc2x2.in().enabled(0, 1), false);
		QCOMPARE(pc2x2.in().enabled(1, 0), false);
		QCOMPARE(pc2x2.in().enabled(1, 1), true);

		QCOMPARE(pc2x2.out().enabled(0, 0), true);
		QCOMPARE(pc2x2.out().enabled(0, 1), false);
		QCOMPARE(pc2x2.out().enabled(1, 0), false);
		QCOMPARE(pc2x2.out().enabled(1, 1), true);

		// 1 input, 1 output (mono/mono effect)
		//
		// In    Out
		//  _     _
		// |X|   |X|
		// |X|   |X|
		//  -     -

		auto pc1x1 = PluginPinConnector{1, 1, &model};
		QCOMPARE(pc1x1.in().enabled(0, 0), true);
		QCOMPARE(pc1x1.in().enabled(1, 0), true);

		QCOMPARE(pc1x1.out().enabled(0, 0), true);
		QCOMPARE(pc1x1.out().enabled(1, 0), true);

		// 1 input, >2 outputs
		//
		// In    Out
		//  _     _______
		// |X|   |X| | | |
		// |X|   | |X| | |
		//  -     -------

		auto pc1x4 = PluginPinConnector{1, 4, &model};
		QCOMPARE(pc1x4.in().enabled(0, 0), true);
		QCOMPARE(pc1x4.in().enabled(1, 0), true);

		QCOMPARE(pc1x4.out().enabled(0, 0), true);
		QCOMPARE(pc1x4.out().enabled(0, 1), false);
		QCOMPARE(pc1x4.out().enabled(0, 2), false);
		QCOMPARE(pc1x4.out().enabled(0, 3), false);
		QCOMPARE(pc1x4.out().enabled(1, 0), false);
		QCOMPARE(pc1x4.out().enabled(1, 1), true);
		QCOMPARE(pc1x4.out().enabled(1, 2), false);
		QCOMPARE(pc1x4.out().enabled(1, 3), false);
	}

	//! Verifies that the routed channels optimization works
	void RoutedChannelsOptimization()
	{
		using namespace lmms;

		// Setup
		auto model = Model{nullptr};
		auto pc = PluginPinConnector{2, 2, &model};

		// Out
		//  ___
		// |X| | 0
		// | |X| 1
		//  ---

		// Track channels 0 and 1 should both have a plugin output channel routed to them
		QCOMPARE(pc.m_routedChannels[0], true);
		QCOMPARE(pc.m_routedChannels[1], true);

		// Out
		//  ___
		// | | | 0
		// | |X| 1
		//  ---

		pc.out().pins(0)[0]->setValue(false);

		// Now only track channel 1 should have a plugin channel routed to it
		QCOMPARE(pc.m_routedChannels[0], false);
		QCOMPARE(pc.m_routedChannels[1], true);

		// Out
		//  ___
		// | |X| 0
		// | |X| 1
		//  ---

		pc.out().pins(0)[1]->setValue(true);

		QCOMPARE(pc.m_routedChannels[0], true);
		QCOMPARE(pc.m_routedChannels[1], true);

		// Out
		//  ___
		// | |X| 0
		// |X|X| 1
		//  ---

		pc.out().pins(1)[0]->setValue(true);

		QCOMPARE(pc.m_routedChannels[0], true);
		QCOMPARE(pc.m_routedChannels[1], true);
	}

	//! Verifies correct default routing for 1x1 non-interleaved (split) plugin
	void Routing_Split1x1_Default()
	{
		using namespace lmms;

		// Setup
		auto model = Model{nullptr};
		auto pc = PluginPinConnector{1, 1, &model};
		auto coreBus = getCoreBus();
		SampleFrame* trackChannels = coreBus.bus[0]; // channels 0/1

		// Downmix stereo to mono plugin input, upmix mono plugin output to stereo
		// In    Out
		//  _     _
		// |X|   |X|
		// |X|   |X|
		//  -     -

		// Data on frames 0, 1, and 33
		trackChannels[0].setLeft(123.f);
		trackChannels[0].setRight(321.f);
		trackChannels[1].setLeft(456.f);
		trackChannels[1].setRight(654.f);
		trackChannels[33].setLeft(789.f);
		trackChannels[33].setRight(987.f);

		// Plugin input and output buffers
		auto bufferSplit1x1 = AudioPluginBufferDefaultImpl<AudioDataLayout::Split, float, 1, 1, false>{};
		auto ins = bufferSplit1x1.inputBuffer();
		auto outs = bufferSplit1x1.outputBuffer();

		// Route to plugin
		auto router = pc.getRouter<AudioDataLayout::Split, float, 1, 1>();
		router.routeToPlugin(coreBus, ins);

		// Check that plugin inputs have data on frames 0, 1, and 33 (should be mixed down to mono)
		QCOMPARE(ins.buffer(0)[0], (123.f + 321.f) / 2);
		QCOMPARE(ins.buffer(0)[1], (456.f + 654.f) / 2);
		QCOMPARE(ins.buffer(0)[33], (789.f + 987.f) / 2);

		// Do work of processImpl - in this case it doubles the amplitude
		transformBuffer(ins, outs, [](auto s) { return s * 2; });

		// Construct buffer with the expected core bus result
		auto coreBufferExpected = std::vector<SampleFrame>(MaxFrames);
		auto coreBufferPtrExpected = coreBufferExpected.data();
		auto coreBusExpected = CoreAudioBusMut{&coreBufferPtrExpected, 1, MaxFrames};
		coreBusExpected.bus[0][0] = SampleFrame{123.f + 321.f, 123.f + 321.f};
		coreBusExpected.bus[0][1] = SampleFrame{456.f + 654.f, 456.f + 654.f};
		coreBusExpected.bus[0][33] = SampleFrame{789.f + 987.f, 789.f + 987.f};

		// Route from plugin back to Core
		router.routeFromPlugin(outs, coreBus);

		// Check that result is mono mix with doubled amplitude
		QCOMPARE(coreBus.bus[0][0].left(), 123.f + 321.f);
		QCOMPARE(coreBus.bus[0][0].right(), 123.f + 321.f);
		QCOMPARE(coreBus.bus[0][1].left(), 456.f + 654.f);
		QCOMPARE(coreBus.bus[0][1].right(), 456.f + 654.f);
		QCOMPARE(coreBus.bus[0][33].left(), 789.f + 987.f);
		QCOMPARE(coreBus.bus[0][33].right(), 789.f + 987.f);

		// Test the rest of the buffer
		compareBuffers(coreBus, coreBusExpected);
	}

	//! Verifies correct default routing for 2x2 non-interleaved (split) plugin
	void Routing_Split2x2_Default()
	{
		using namespace lmms;

		// Setup
		auto model = Model{nullptr};
		auto pc = PluginPinConnector{2, 2, &model};
		auto coreBus = getCoreBus();
		SampleFrame* trackChannels = coreBus.bus[0]; // channels 0/1

		// Data on frames 0, 1, and 33
		trackChannels[0].setLeft(123.f);
		trackChannels[0].setRight(321.f);
		trackChannels[1].setLeft(456.f);
		trackChannels[1].setRight(654.f);
		trackChannels[33].setLeft(789.f);
		trackChannels[33].setRight(987.f);

		// Plugin input and output buffers
		auto bufferSplit2x2 = AudioPluginBufferDefaultImpl<AudioDataLayout::Split, float, 2, 2, false>{};
		auto ins = bufferSplit2x2.inputBuffer();
		auto outs = bufferSplit2x2.outputBuffer();

		// Route to plugin
		auto router = pc.getRouter<AudioDataLayout::Split, float, 2, 2>();
		router.routeToPlugin(coreBus, ins);

		// Check that plugin inputs have data on frames 0, 1, and 33
		QCOMPARE(ins.buffer(0)[0], 123.f);
		QCOMPARE(ins.buffer(1)[0], 321.f);
		QCOMPARE(ins.buffer(0)[1], 456.f);
		QCOMPARE(ins.buffer(1)[1], 654.f);
		QCOMPARE(ins.buffer(0)[33], 789.f);
		QCOMPARE(ins.buffer(1)[33], 987.f);

		// Do work of processImpl - in this case it doubles the amplitude
		transformBuffer(ins, outs, [](auto s) { return s * 2; });

		// Construct buffer with the expected core bus result
		auto coreBufferExpected = std::vector<SampleFrame>(MaxFrames);
		auto coreBufferPtrExpected = coreBufferExpected.data();
		auto coreBusExpected = CoreAudioBusMut{&coreBufferPtrExpected, 1, MaxFrames};
		transformBuffer(coreBus, coreBusExpected, [](auto s) { return s * 2; });

		// Sanity check for transformBuffer
		QCOMPARE(outs.buffer(0)[0], 123.f * 2);
		QCOMPARE(outs.buffer(1)[0], 321.f * 2);
		QCOMPARE(outs.buffer(0)[1], 456.f * 2);
		QCOMPARE(outs.buffer(1)[1], 654.f * 2);
		QCOMPARE(outs.buffer(0)[33], 789.f * 2);
		QCOMPARE(outs.buffer(1)[33], 987.f * 2);

		// Zero core bus just to be sure what the plugin output is
		lmms::zeroBuffer(coreBus);

		// Route from plugin back to Core
		router.routeFromPlugin(outs, coreBus);

		// Should be double the original
		QCOMPARE(coreBus.bus[0][0].left(), 123.f * 2);
		QCOMPARE(coreBus.bus[0][0].right(), 321.f * 2);
		QCOMPARE(coreBus.bus[0][1].left(), 456.f * 2);
		QCOMPARE(coreBus.bus[0][1].right(), 654.f * 2);
		QCOMPARE(coreBus.bus[0][33].left(), 789.f * 2);
		QCOMPARE(coreBus.bus[0][33].right(), 987.f * 2);

		// Test the rest of the buffer
		compareBuffers(coreBus, coreBusExpected);
	}

	//! Verifies correct partially-bypassed routing for 2x2 non-interleaved (split) plugin
	void Routing_Split2x2_Bypass()
	{
		using namespace lmms;

		// Setup
		auto model = Model{nullptr};
		auto pc = PluginPinConnector{2, 2, &model};
		auto coreBus = getCoreBus();
		SampleFrame* trackChannels = coreBus.bus[0]; // channels 0/1

		// Default input connections, disable right output channel
		// In    Out
		//  ___   ___
		// |X| | |X| |
		// | |X| | | |
		//  ---   ---
		pc.out().pins(1)[1]->setValue(false);

		// Data on frames 0, 1, and 33
		trackChannels[0].setLeft(123.f);
		trackChannels[0].setRight(321.f);
		trackChannels[1].setLeft(456.f);
		trackChannels[1].setRight(654.f);
		trackChannels[33].setLeft(789.f);
		trackChannels[33].setRight(987.f);

		// Plugin input and output buffers
		auto bufferSplit2x2 = AudioPluginBufferDefaultImpl<AudioDataLayout::Split, float, 2, 2, false>{};
		auto ins = bufferSplit2x2.inputBuffer();
		auto outs = bufferSplit2x2.outputBuffer();

		// Route to plugin
		auto router = pc.getRouter<AudioDataLayout::Split, float, 2, 2>();
		router.routeToPlugin(coreBus, ins);

		// Check that plugin inputs have data on frames 0, 1, and 33
		QCOMPARE(ins.buffer(0)[0], 123.f);
		QCOMPARE(ins.buffer(1)[0], 321.f);
		QCOMPARE(ins.buffer(0)[1], 456.f);
		QCOMPARE(ins.buffer(1)[1], 654.f);
		QCOMPARE(ins.buffer(0)[33], 789.f);
		QCOMPARE(ins.buffer(1)[33], 987.f);

		// Do work of processImpl - in this case it doubles the amplitude
		transformBuffer(ins, outs, [](auto s) { return s * 2; });

		// Construct buffer with the expected core bus result
		auto coreBufferExpected = std::vector<SampleFrame>(MaxFrames);
		auto coreBufferPtrExpected = coreBufferExpected.data();
		auto coreBusExpected = CoreAudioBusMut{&coreBufferPtrExpected, 1, MaxFrames};
		for (f_cnt_t frame = 0; frame < coreBus.frames; ++frame)
		{
			SampleFrame& sf = coreBusExpected.bus[0][frame];
			sf.leftRef() = coreBus.bus[0][frame].left() * 2; // left channel:  doubled output from plugin
			sf.rightRef() = coreBus.bus[0][frame].right();   // right channel: bypassed
		}

		// Route from plugin back to Core
		router.routeFromPlugin(outs, coreBus);

		// Right track channel should pass through, but left track channel
		// should be overwritten with plugin's left output channel
		QCOMPARE(coreBus.bus[0][0].left(), 123.f * 2);
		QCOMPARE(coreBus.bus[0][0].right(), 321.f);
		QCOMPARE(coreBus.bus[0][1].left(), 456.f * 2);
		QCOMPARE(coreBus.bus[0][1].right(), 654.f);
		QCOMPARE(coreBus.bus[0][33].left(), 789.f * 2);
		QCOMPARE(coreBus.bus[0][33].right(), 987.f);

		// Test the rest of the buffer
		compareBuffers(coreBus, coreBusExpected);
	}

	//! Verifies correct default routing for 2x2 SampleFrame-based plugin
	void Routing_SampleFrame2x2_Default()
	{
		using namespace lmms;

		// Setup
		auto model = Model{nullptr};
		auto pc = PluginPinConnector{2, 2, &model};
		auto coreBus = getCoreBus();
		SampleFrame* trackChannels = coreBus.bus[0]; // channels 0/1

		// Data on frames 0, 1, and 33
		trackChannels[0].setLeft(123.f);
		trackChannels[0].setRight(321.f);
		trackChannels[1].setLeft(456.f);
		trackChannels[1].setRight(654.f);
		trackChannels[33].setLeft(789.f);
		trackChannels[33].setRight(987.f);

		// Plugin input and output buffers
		auto pluginBuffers = AudioPluginBufferDefaultImpl<AudioDataLayout::Interleaved, SampleFrame, 2, 2, true>{};
		auto ins = pluginBuffers.inputBuffer();
		auto outs = pluginBuffers.outputBuffer();

		QCOMPARE(ins.data(), outs.data()); // in-place processing - should use same buffer for inputs and outputs

		// Route to plugin
		auto router = pc.getRouter<AudioDataLayout::Interleaved, SampleFrame, 2, 2>();
		router.routeToPlugin(coreBus, ins);

		// Check that plugin inputs have data on frames 0, 1, and 33
		QCOMPARE(ins[0].left(), 123.f);
		QCOMPARE(ins[0].right(), 321.f);
		QCOMPARE(ins[1].left(), 456.f);
		QCOMPARE(ins[1].right(), 654.f);
		QCOMPARE(ins[33].left(), 789.f);
		QCOMPARE(ins[33].right(), 987.f);

		// Do work of processImpl - in this case it doubles the amplitude
		transformBuffer(ins, outs, [](auto s) { return s * 2; });

		// Construct buffer with the expected core bus result
		auto coreBufferExpected = std::vector<SampleFrame>(MaxFrames);
		auto coreBufferPtrExpected = coreBufferExpected.data();
		auto coreBusExpected = CoreAudioBusMut{&coreBufferPtrExpected, 1, MaxFrames};
		transformBuffer(coreBus, coreBusExpected, [](auto s) { return s * 2; });

		// Zero core bus just to be sure what the plugin output is
		lmms::zeroBuffer(coreBus);

		// Route from plugin back to Core
		router.routeFromPlugin(outs, coreBus);

		// Should be double the original
		QCOMPARE(coreBus.bus[0][0].left(), 123.f * 2);
		QCOMPARE(coreBus.bus[0][0].right(), 321.f * 2);
		QCOMPARE(coreBus.bus[0][1].left(), 456.f * 2);
		QCOMPARE(coreBus.bus[0][1].right(), 654.f * 2);
		QCOMPARE(coreBus.bus[0][33].left(), 789.f * 2);
		QCOMPARE(coreBus.bus[0][33].right(), 987.f * 2);

		// Test the rest of the buffer
		compareBuffers(coreBus, coreBusExpected);
	}
};

QTEST_GUILESS_MAIN(PluginPinConnectorTest)
#include "PluginPinConnectorTest.moc"
