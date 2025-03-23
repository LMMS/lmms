/*
 * AudioDevice.cpp - base-class for audio-devices used by LMMS audio engine
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <cstring>

#include "AudioDevice.h"
#include "AudioEngine.h"
#include "ConfigManager.h"

namespace lmms
{

AudioDevice::AudioDevice(const ch_cnt_t _channels, AudioEngine* _audioEngine)
	: m_supportsCapture(false)
	, m_framesPerPeriod(
		  std::clamp<fpp_t>(ConfigManager::inst()
								->value("audioengine", "framesperaudiobuffer", QString::number(DEFAULT_BUFFER_SIZE))
								.toULong(),
			  MINIMUM_BUFFER_SIZE, MAXIMUM_BUFFER_SIZE))
	, m_sampleRate(_audioEngine->outputSampleRate())
	, m_channels(_channels)
	, m_audioEngine(_audioEngine)
{
}




AudioDevice::~AudioDevice()
{
	m_devMutex.tryLock();
	unlock();
}

bool AudioDevice::nextBuffer(void* dst, std::size_t dstFrameCount, std::size_t dstChannelCount, bool interleaved)
{
	assert(dstChannelCount > 0);

	if (!m_running.test_and_set(std::memory_order_acquire))
	{
		m_running.clear(std::memory_order_release);
		return false;
	}

	if (!dst)
	{
		m_audioEngine->renderNextBuffer();
		return true;
	}

	const auto srcBufferFrameCount = m_audioEngine->framesPerPeriod();
	auto framesRead = std::size_t{0};

	while (framesRead != dstFrameCount)
	{
		if (!m_audioEngineBuffer) { m_audioEngineBuffer = m_audioEngine->renderNextBuffer(); }

		// The number of frames to read equal either the number of frames we can read, or how many frames we need to
		// read left. Whichever one is smaller.
		const auto framesToRead = std::min(srcBufferFrameCount - m_audioEngineBufferIndex, dstFrameCount - framesRead);

		if (interleaved)
		{
			const auto dstBuffer = static_cast<float*>(dst);

			for (auto frame = std::size_t{0}; frame < framesToRead; ++frame)
			{
				const auto dstFrameIndex = framesRead + frame;
				const auto srcFrameIndex = m_audioEngineBufferIndex + frame;

				if (dstChannelCount == 1)
				{
					dstBuffer[dstFrameIndex * dstChannelCount] = m_audioEngineBuffer[srcFrameIndex].average();
				}
				else
				{
					dstBuffer[dstFrameIndex * dstChannelCount] = m_audioEngineBuffer[srcFrameIndex].left();
					dstBuffer[dstFrameIndex * dstChannelCount + 1] = m_audioEngineBuffer[srcFrameIndex].right();

					// If dst has more channels than 2 (stereo), then silence them. LMMS only outputs stereo audio.
					std::fill_n(dstBuffer + dstFrameIndex * dstChannelCount + 2, dstChannelCount - 2, 0.f);
				}
			}
		}
		else
		{
			const auto dstBuffer = static_cast<float**>(dst);

			for (auto channel = std::size_t{0}; channel < dstChannelCount; ++channel)
			{
				// If dst has more channels than 2 (stereo), then silence them. LMMS only outputs stereo audio.
				if (channel > 1)
				{
					std::fill_n(dstBuffer[channel] + framesRead, framesToRead, 0.f);
					break;
				}

				for (auto frame = std::size_t{0}; frame < framesToRead; ++frame)
				{
					const auto& srcFrame = m_audioEngineBuffer[m_audioEngineBufferIndex + frame];
					dstBuffer[channel][frame] = dstChannelCount == 1 ? srcFrame.average() : srcFrame[channel];
				}
			}
		}

		m_audioEngineBufferIndex = (m_audioEngineBufferIndex + framesToRead) % srcBufferFrameCount;
		framesRead += framesToRead;
		if (m_audioEngineBufferIndex == 0) { m_audioEngineBuffer = nullptr; }
	}

	return true;
}

void AudioDevice::stopProcessingThread( QThread * thread )
{
	if( !thread->wait( 30000 ) )
	{
		fprintf( stderr, "Terminating audio device thread\n" );
		thread->terminate();
		if( !thread->wait( 1000 ) )
		{
			fprintf( stderr, "Thread not terminated yet\n" );
		}
	}
}



void AudioDevice::registerPort(AudioBusHandle*)
{
}




void AudioDevice::unregisterPort(AudioBusHandle*)
{
}




void AudioDevice::renamePort(AudioBusHandle*)
{
}

int AudioDevice::convertToS16(const SampleFrame* _ab,
								const fpp_t _frames,
								int_sample_t * _output_buffer,
								const bool _convert_endian )
{
	if( _convert_endian )
	{
		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < channels(); ++chnl )
			{
				auto temp = static_cast<int_sample_t>(AudioEngine::clip(_ab[frame][chnl]) * OUTPUT_SAMPLE_MULTIPLIER);

				( _output_buffer + frame * channels() )[chnl] =
						( temp & 0x00ff ) << 8 |
						( temp & 0xff00 ) >> 8;
			}
		}
	}
	else
	{
		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < channels(); ++chnl )
			{
				(_output_buffer + frame * channels())[chnl]
					= static_cast<int_sample_t>(AudioEngine::clip(_ab[frame][chnl]) * OUTPUT_SAMPLE_MULTIPLIER);
			}
		}
	}

	return _frames * channels() * BYTES_PER_INT_SAMPLE;
}




void AudioDevice::clearS16Buffer( int_sample_t * _outbuf, const fpp_t _frames )
{

	assert( _outbuf != nullptr );

	memset( _outbuf, 0,  _frames * channels() * BYTES_PER_INT_SAMPLE );
}

} // namespace lmms
