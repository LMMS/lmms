/*
 * AudioFilePlayHandle.cpp
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#include "AudioFilePlayHandle.h"

#include "AudioBusHandle.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "SampleFrame.h"

namespace lmms {
AudioFilePlayHandle::AudioFilePlayHandle(const std::filesystem::path& path, int interpolationMode)
	: PlayHandle(Type::SamplePlayHandle)
	, m_audioFile(path, AudioFile::Mode::Read)
	, m_audioResampler(interpolationMode, DEFAULT_CHANNELS)
	, m_sourceBuffer(FramesPerBuffer * m_audioFile.channels())
	, m_channelConvertBuffer(FramesPerBuffer * DEFAULT_CHANNELS)
{
	setAudioBusHandle(new AudioBusHandle("AudioFilePlayHandle", false));
}

AudioFilePlayHandle::~AudioFilePlayHandle()
{
	delete audioBusHandle();
}

void AudioFilePlayHandle::play(SampleFrame* dst)
{
	// TODO: Reading from the audio file technically should be done on another background thread, but since our audio
	// threads do not have real-time priority (yet), having an extra thread will add more thread contention for little
	// gain. Adding the background thread actually introduced lagspikes while testing with my system. Once
	// the audio threads are made to run with real-time priority, we can introduce the new thread after confirming there
	// is a noticeable improvement.

	const auto needsResampling = Engine::audioEngine()->outputSampleRate() != m_audioFile.sampleRate();
	const auto needsChannelConversion = m_audioFile.channels() != DEFAULT_CHANNELS;

	const auto framesPerPeriod = Engine::audioEngine()->framesPerPeriod();
	const auto ratio = static_cast<double>(Engine::audioEngine()->outputSampleRate()) / m_audioFile.sampleRate();

	auto sink = InterleavedBufferView<float>{&dst[0][0], DEFAULT_CHANNELS, framesPerPeriod};

	if (!needsResampling && !needsChannelConversion)
	{
		const auto framesRead = m_audioFile.read(sink);
		m_framesRead += framesRead;
		return;
	}

	while (!sink.empty())
	{
		if (!m_channelConvertBufferView.empty())
		{
			auto input = m_channelConvertBufferView.data();
			auto inputFrames = m_channelConvertBufferView.frames();

			auto output = sink.data();
			auto outputFrames = sink.frames();

			const auto result = m_audioResampler.resample(input, inputFrames, output, outputFrames, ratio);

			input += result.inputFramesUsed * DEFAULT_CHANNELS;
			inputFrames -= result.inputFramesUsed;

			output += result.outputFramesGenerated * DEFAULT_CHANNELS;
			outputFrames -= result.outputFramesGenerated;

			m_channelConvertBufferView = {input, DEFAULT_CHANNELS, inputFrames};
			sink = {output, DEFAULT_CHANNELS, outputFrames};
		}
		else if (!m_sourceBufferView.empty())
		{
			auto input = m_sourceBufferView.data();
			auto inputFrames = m_sourceBufferView.frames();

			auto output = m_channelConvertBuffer.data();
			auto outputFrames = m_channelConvertBuffer.size() / DEFAULT_CHANNELS;

			const auto framesToConvert = std::min(inputFrames, outputFrames);
			const auto mono = m_audioFile.channels() == 1;

			for (auto frame = std::size_t{0}; frame < framesToConvert; ++frame)
			{
				output[frame * 2] = input[frame * m_audioFile.channels()];
				output[frame * 2 + 1] = mono ? output[frame * 2] : input[frame * m_audioFile.channels() + 1];
			}

			input += framesToConvert * m_audioFile.channels();
			inputFrames -= framesToConvert;

			m_sourceBufferView = {input, m_audioFile.channels(), inputFrames};
			m_channelConvertBufferView = {output, DEFAULT_CHANNELS, framesToConvert};
		}
		else
		{
			const auto framesRead = m_audioFile.read(
				{m_sourceBuffer.data(), m_audioFile.channels(), m_sourceBuffer.size() / m_audioFile.channels()});

			if (framesRead == 0)
			{
				std::fill_n(sink.data(), sink.frames() * sink.channels(), 0.f);
				break;
			}

			m_framesRead += framesRead;
			m_sourceBufferView = {m_sourceBuffer.data(), m_audioFile.channels(), framesRead};
		}
	}
}

f_cnt_t AudioFilePlayHandle::seek(f_cnt_t frames, AudioFile::Whence whence)
{
	return m_audioFile.seek(frames, whence);
}

bool AudioFilePlayHandle::isFinished() const
{
	return m_framesRead == m_audioFile.frames();
}

} // namespace lmms
