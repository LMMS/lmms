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
#include "debug.h"

namespace lmms
{

AudioDevice::AudioDevice( const ch_cnt_t _channels, AudioEngine*  _audioEngine ) :
	m_supportsCapture( false ),
	m_sampleRate( _audioEngine->processingSampleRate() ),
	m_channels( _channels ),
	m_audioEngine( _audioEngine ),
	m_buffer( new surroundSampleFrame[audioEngine()->framesPerPeriod()] )
{
	int error;
	if( ( m_srcState = src_new(
		audioEngine()->currentQualitySettings().libsrcInterpolation(),
				SURROUND_CHANNELS, &error ) ) == nullptr )
	{
		printf( "Error: src_new() failed in audio_device.cpp!\n" );
	}
}




AudioDevice::~AudioDevice()
{
	src_delete( m_srcState );
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




fpp_t AudioDevice::getNextBuffer( surroundSampleFrame * _ab )
{
	fpp_t frames = audioEngine()->framesPerPeriod();
	const surroundSampleFrame * b = audioEngine()->nextBuffer();
	if( !b )
	{
		return 0;
	}

	// make sure, no other thread is accessing device
	lock();

	// resample if necessary
	if( audioEngine()->processingSampleRate() != m_sampleRate )
	{
		frames = resample( b, frames, _ab, audioEngine()->processingSampleRate(), m_sampleRate );
	}
	else
	{
		memcpy( _ab, b, frames * sizeof( surroundSampleFrame ) );
	}

	// release lock
	unlock();

	if( audioEngine()->hasFifoWriter() )
	{
		delete[] b;
	}

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




void AudioDevice::applyQualitySettings()
{
	src_delete( m_srcState );

	int error;
	if( ( m_srcState = src_new(
		audioEngine()->currentQualitySettings().libsrcInterpolation(),
				SURROUND_CHANNELS, &error ) ) == nullptr )
	{
		printf( "Error: src_new() failed in audio_device.cpp!\n" );
	}
}




void AudioDevice::registerPort( AudioPort * )
{
}




void AudioDevice::unregisterPort( AudioPort * _port )
{
}




void AudioDevice::renamePort( AudioPort * )
{
}




fpp_t AudioDevice::resample( const surroundSampleFrame * _src,
						const fpp_t _frames,
						surroundSampleFrame * _dst,
						const sample_rate_t _src_sr,
						const sample_rate_t _dst_sr )
{
	if( m_srcState == nullptr )
	{
		return _frames;
	}
	m_srcData.input_frames = _frames;
	m_srcData.output_frames = _frames;
	m_srcData.data_in = const_cast<float*>(_src[0].data());
	m_srcData.data_out = _dst[0].data ();
	m_srcData.src_ratio = (double) _dst_sr / _src_sr;
	m_srcData.end_of_input = 0;
	int error;
	if( ( error = src_process( m_srcState, &m_srcData ) ) )
	{
		printf( "AudioDevice::resample(): error while resampling: %s\n",
							src_strerror( error ) );
	}
	return static_cast<fpp_t>(m_srcData.output_frames_gen);
}



int AudioDevice::convertToS16( const surroundSampleFrame * _ab,
								const fpp_t _frames,
								int_sample_t * _output_buffer,
								const bool _convert_endian )
{
	if( _convert_endian )
	{
		int_sample_t temp;
		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < channels(); ++chnl )
			{
				temp = static_cast<int_sample_t>(AudioEngine::clip(_ab[frame][chnl]) * OUTPUT_SAMPLE_MULTIPLIER);

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




bool AudioDevice::hqAudio() const
{
	return ConfigManager::inst()->value( "audioengine", "hqaudio" ).toInt();
}


} // namespace lmms