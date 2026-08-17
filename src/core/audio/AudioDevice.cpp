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
#include "endian_handling.h"

namespace lmms
{

AudioDevice::AudioDevice(const ch_cnt_t _channels, AudioEngine* _audioEngine)
	: m_supportsCapture(false)
	, m_sampleRate(_audioEngine->outputSampleRate())
	, m_channels(_channels)
	, m_audioEngine(_audioEngine)
{
}



AudioDevice::~AudioDevice()
{
	assert(!isRunning() && "device should have been stopped before being destroyed");
}

void AudioDevice::startProcessing()
{
	m_running.test_and_set(std::memory_order_acquire);
	startProcessingImpl();
}

void AudioDevice::stopProcessing()
{
	m_running.clear(std::memory_order_release);
	stopProcessingImpl();
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


void AudioDevice::toInt16le(std::span<const SampleFrame>&& src, std::span<int_sample_t>&& dst)
{
	// FIXME: SampleFrame is hardcoded stereo (DEFAULT_CHANNELS) so this function cannot fully respect
	// AudioDevice::m_channels.
	assert(m_channels <= 2);

	if (m_channels == 2)
	{
		assert(dst.size() == src.size() * 2); // Every SampleFrame corresponds to two int_sample_t
		for (f_cnt_t frame = 0; frame < src.size(); ++frame)
		{
			const sample_t l = AudioEngine::clip(src[frame].left()) * OUTPUT_SAMPLE_MULTIPLIER;
			const sample_t r = AudioEngine::clip(src[frame].right()) * OUTPUT_SAMPLE_MULTIPLIER;
			dst[DEFAULT_CHANNELS * frame + 0] = swap16IfBE(static_cast<int_sample_t>(l));
			dst[DEFAULT_CHANNELS * frame + 1] = swap16IfBE(static_cast<int_sample_t>(r));
		}
	}
	else if (m_channels == 1) // Mixdown to mono if only one channel
	{
		assert(dst.size() == src.size()); // Every SampleFrame corresponds to one int_sample_t
		for (f_cnt_t frame = 0; m_channels == 1 && frame < src.size(); ++frame)
		{
			const sample_t m = AudioEngine::clip(src[frame].average()) * OUTPUT_SAMPLE_MULTIPLIER;
			dst[frame] = swap16IfBE(static_cast<int_sample_t>(m));
		}
	}
}


void AudioDevice::toInt16(std::span<const sample_t>&& src, std::span<int_sample_t>&& dst)
{
	assert(dst.size() == src.size());

	for (f_cnt_t frame = 0; frame < src.size(); ++frame)
	{
		dst[DEFAULT_CHANNELS * frame] =
			static_cast<int_sample_t>(AudioEngine::clip(src[frame]) * OUTPUT_SAMPLE_MULTIPLIER);
	}
}


} // namespace lmms
