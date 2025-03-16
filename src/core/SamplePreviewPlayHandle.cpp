/*
 * SamplePreviewPlayHandle.cpp
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

#include "SamplePreviewPlayHandle.h"

#include "AudioBusHandle.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "SampleFrame.h"
#include "ThreadPool.h"

namespace lmms {
SamplePreviewPlayHandle::SamplePreviewPlayHandle(const std::filesystem::path& path)
	: PlayHandle(Type::SamplePreviewPlayHandle)
	, m_writeChunkSize(static_cast<sf_count_t>(Engine::audioEngine()->framesPerPeriod()))
{
#ifdef LMMS_BUILD_WIN32
	m_sndfile = sf_wchar_open(path.c_str(), SFM_READ, &m_sfinfo);
#else
	m_sndfile = sf_open(path.c_str(), SFM_READ, &m_sfinfo);
#endif

	if (m_sndfile == nullptr || m_sfinfo.channels == 0 || m_sfinfo.frames == 0)
	{
		throw std::runtime_error{"Failed to create sample preview stream"};
	}

	const auto initialFrameCount = std::min(static_cast<sf_count_t>(m_sfinfo.samplerate), m_sfinfo.frames);
	m_buffer.resize((initialFrameCount + 1) * m_sfinfo.channels);
	sf_readf_float(m_sndfile, m_buffer.data(), initialFrameCount);

	m_frameWriteIndex = (m_frameWriteIndex + initialFrameCount) % framesInBuffer();
	m_framesWritten += initialFrameCount;

	m_diskStream = ThreadPool::instance().enqueue(&SamplePreviewPlayHandle::runDiskStream, this);
	setAudioBusHandle(new AudioBusHandle("SamplePreviewPlayHandle", false));
}

SamplePreviewPlayHandle::~SamplePreviewPlayHandle() noexcept
{
	m_quit = true;
	m_diskStream.get();
	sf_close(m_sndfile);
	delete audioBusHandle();
}

void SamplePreviewPlayHandle::play(SampleFrame* dst)
{
	// TODO: Should be a parameter in the function
	const auto dstFrameCount = static_cast<sf_count_t>(Engine::audioEngine()->framesPerPeriod());
	std::fill_n(dst, dstFrameCount, SampleFrame{});

	const auto resamplingRatio = Engine::audioEngine()->outputSampleRate() / static_cast<double>(m_sfinfo.samplerate);
	const auto srcFramesNeeded = std::ceil(dstFrameCount / resamplingRatio);

	const auto srcFramesToRead = std::min<sf_count_t>(framesAvailableToRead(), srcFramesNeeded);
	const auto channelsToRead = std::min(m_sfinfo.channels, static_cast<int>(DEFAULT_CHANNELS));

	const auto frameReadIndex = m_frameReadIndex.load(std::memory_order::acquire);

	for (auto dstFrameIndex = 0; dstFrameIndex < dstFrameCount; ++dstFrameIndex)
	{
		const auto srcFrameIndex = dstFrameIndex / resamplingRatio;

		const auto currentFrameReadIndex = static_cast<double>(frameReadIndex) + srcFrameIndex;
		const auto prevFrameReadIndex = std::floor(currentFrameReadIndex);
		const auto nextFrameReadIndex = std::ceil(currentFrameReadIndex);

		const auto prevSrcFrame = bufferAt(static_cast<sf_count_t>(prevFrameReadIndex));
		const auto nextSrcFrame = bufferAt(static_cast<sf_count_t>(nextFrameReadIndex));

		const auto interpolatedSrcFramePos = currentFrameReadIndex - prevFrameReadIndex;

		for (auto channel = 0; channel < channelsToRead; ++channel)
		{
			const auto interpolatedSample
				= std::lerp(prevSrcFrame[channel], nextSrcFrame[channel], interpolatedSrcFramePos);

			if (channelsToRead == 1)
			{
				dst[dstFrameIndex][0] = static_cast<float>(interpolatedSample);
				dst[dstFrameIndex][1] = static_cast<float>(interpolatedSample);
			}
			else
			{
				dst[dstFrameIndex][channel] = static_cast<float>(interpolatedSample);
			}
		}
	}

	m_frameReadIndex.store((frameReadIndex + srcFramesToRead) % framesInBuffer(), std::memory_order_release);
	m_framesRead += srcFramesToRead;
}

void SamplePreviewPlayHandle::runDiskStream()
{
	while (m_framesWritten < m_sfinfo.frames && !m_quit)
	{
		const auto framesToWrite
			= std::min({framesAvailableToWrite(), m_sfinfo.frames - m_framesWritten, m_writeChunkSize});
		if (framesToWrite == 0) { continue; }

		const auto frameWriteIndex = m_frameWriteIndex.load(std::memory_order_acquire);

		const auto framesToWriteUntilEnd = std::min(framesInBuffer() - frameWriteIndex, framesToWrite);
		sf_readf_float(m_sndfile, bufferAt(frameWriteIndex), framesToWriteUntilEnd);

		const auto framesToWriteAtBegin = framesToWrite - framesToWriteUntilEnd;
		sf_readf_float(m_sndfile, bufferAt(0), framesToWriteAtBegin);

		m_frameWriteIndex.store((frameWriteIndex + framesToWrite) % framesInBuffer(), std::memory_order_release);
		m_framesWritten += framesToWrite;
	}
}

} // namespace lmms
