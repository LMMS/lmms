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
	, m_audioResampler(interpolationMode, m_audioFile.channels())
	, m_channelConvertBuffer(Engine::audioEngine()->framesPerPeriod() * m_audioFile.channels())
	, m_resampleBuffer(Engine::audioEngine()->framesPerPeriod())
{
	setAudioBusHandle(new AudioBusHandle("AudioFilePlayHandle", false));
}

AudioFilePlayHandle::~AudioFilePlayHandle() noexcept
{
	delete audioBusHandle();
}

void AudioFilePlayHandle::play(SampleFrame* dst)
{
	// TODO: Reading from the audio file technically should be done on another background thread, but since our audio
	// threads are not made to run in real-time (yet), having an extra thread will add more thread contention for little
	// gain. Adding the background thread actually introduced lagspikes while testing with my system. Once
	// the audio threads are made to run with real-time priority, we can introduce the new thread after confirming there
	// is a noticeable improvement.

	const auto needsResampling = Engine::audioEngine()->outputSampleRate() != m_audioFile.sampleRate();
	const auto framesPerPeriod = Engine::audioEngine()->framesPerPeriod();
	const auto needsChannelConversion = m_audioFile.channels() != 2;

	if (!needsResampling && !needsChannelConversion)
	{
		const auto dstAudioView = InterleavedBufferView<float, 2>{&dst[0][0], framesPerPeriod};
		const auto framesRead = m_audioFile.read(dstAudioView);
		std::fill(dst + framesRead, dst + framesPerPeriod, SampleFrame{});
		m_totalFramesRead += framesRead;
		return;
	}
	else if (!needsResampling && needsChannelConversion)
	{
		const auto framesRead = m_audioFile.read({&m_channelConvertBuffer[0], m_audioFile.channels(), framesPerPeriod});
		convertToStereo(&m_channelConvertBuffer[0], m_audioFile.channels(), &dst[0][0], framesRead);
		m_totalFramesRead += framesRead;
		std::fill(dst + framesRead, dst + framesPerPeriod, SampleFrame{});
		return;
	}

	auto dstIndex = f_cnt_t{0};
	auto dstFrames = framesPerPeriod;
	const auto ratio = static_cast<double>(Engine::audioEngine()->outputSampleRate()) / m_audioFile.sampleRate();

	while (dstFrames > 0)
	{
		if (m_resampleFrames == 0)
		{
			auto framesRead = f_cnt_t{0};
			if (needsChannelConversion)
			{
				framesRead = m_audioFile.read({&m_channelConvertBuffer[0], m_audioFile.channels(),
					m_channelConvertBuffer.size() / m_audioFile.channels()});

				convertToStereo(
					&m_channelConvertBuffer[0], m_audioFile.channels(), &m_resampleBuffer[0][0], framesPerPeriod);
			}
			else { framesRead = m_audioFile.read({&m_resampleBuffer[0][0], 2, framesPerPeriod}); }

			if (framesRead == 0)
			{
				std::fill_n(dst + dstIndex, dstFrames, SampleFrame{});
				return;
			}

			m_resampleIndex = 0;
			m_resampleFrames = framesPerPeriod;
			m_totalFramesRead += framesRead;
		}

		const auto result = m_audioResampler.resample(
			&m_resampleBuffer[m_resampleIndex][0], m_resampleFrames, &dst[dstIndex][0], dstFrames, ratio);

		m_resampleIndex += result.inputFramesUsed;
		m_resampleFrames -= result.inputFramesUsed;

		dstIndex += result.outputFramesGenerated;
		dstFrames -= result.outputFramesGenerated;
	}
}

void AudioFilePlayHandle::convertToStereo(float* in, ch_cnt_t inChannels, float* out, f_cnt_t frames)
{
	for (auto frame = 0; frame < frames; ++frame)
	{
		out[frame * 2] = in[frame * inChannels];
		out[frame * 2 + 1] = inChannels == 1 ? out[frame * 2] : in[frame * inChannels + 1];
	}
}

f_cnt_t AudioFilePlayHandle::seek(f_cnt_t frames, AudioFile::Whence whence)
{
	return m_audioFile.seek(frames, whence);
}

bool AudioFilePlayHandle::isFinished() const
{
	return m_totalFramesRead == m_audioFile.frames();
}

} // namespace lmms
