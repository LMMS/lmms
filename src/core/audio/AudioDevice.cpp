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

namespace lmms
{

AudioDevice::AudioDevice( const ch_cnt_t _channels, AudioEngine*  _audioEngine ) :
	m_supportsCapture( false ),
	m_sampleRate( _audioEngine->outputSampleRate() ),
	m_channels( _channels ),
	m_audioEngine( _audioEngine ),
	m_buffer(new SampleFrame[audioEngine()->framesPerPeriod()])
{
}




AudioDevice::~AudioDevice()
{
	delete[] m_buffer;
	m_devMutex.tryLock();
	unlock();
}




void AudioDevice::processNextBuffer()
{
	const fpp_t frames = getNextBuffer( m_buffer );
	if (frames) { writeBuffer(m_buffer, frames); }
	else
	{
		m_inProcess = false;
	}
}

fpp_t AudioDevice::getNextBuffer(SampleFrame* _ab)
{
	fpp_t frames = audioEngine()->framesPerPeriod();
	const SampleFrame* b = audioEngine()->nextBuffer();

	if (!b) { return 0; }

	memcpy(_ab, b, frames * sizeof(SampleFrame));

	if (audioEngine()->hasFifoWriter()) { delete[] b; }
	return frames;
}




void AudioDevice::stopProcessing()
{
	if( audioEngine()->hasFifoWriter() )
	{
		while( m_inProcess )
		{
			processNextBuffer();
		}
	}
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
