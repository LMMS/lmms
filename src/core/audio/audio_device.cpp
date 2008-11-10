#ifndef SINGLE_SOURCE_COMPILE

/*
 * audio_device.cpp - base-class for audio-devices used by LMMS-mixer
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include "audio_device.h"
#include "config_mgr.h"
#include "debug.h"
#include "basic_ops.h"



audioDevice::audioDevice( const ch_cnt_t _channels, mixer * _mixer ) :
	m_supportsCapture( false ),
	m_sampleRate( _mixer->processingSampleRate() ),
	m_channels( _channels ),
	m_mixer( _mixer ),
	m_buffer( alignedAllocFrames( getMixer()->framesPerPeriod() ) )
{
	int error;
	if( ( m_srcState = src_new(
		getMixer()->currentQualitySettings().libsrcInterpolation(),
				SURROUND_CHANNELS, &error ) ) == NULL )
	{
		printf( "Error: src_new() failed in audio_device.cpp!\n" );
	}
}




audioDevice::~audioDevice()
{
	src_delete( m_srcState );
	alignedFreeFrames( m_buffer );

	m_devMutex.tryLock();
	unlock();
}




void audioDevice::processNextBuffer( void )
{
	const fpp_t frames = getNextBuffer( m_buffer );
	if( frames )
	{
		writeBuffer( m_buffer, frames, getMixer()->masterGain() );
	}
	else
	{
		m_inProcess = FALSE;
	}
}




fpp_t audioDevice::getNextBuffer( sampleFrameA * _ab )
{
	fpp_t frames = getMixer()->framesPerPeriod();
	sampleFrameA * b = getMixer()->nextBuffer();
	if( !b )
	{
		return( 0 );
	}

	// make sure, no other thread is accessing device
	lock();

	// resample if neccessary
	if( getMixer()->processingSampleRate() != m_sampleRate )
	{
		resample( b, frames, _ab, getMixer()->processingSampleRate(),
								m_sampleRate );
		frames = frames * m_sampleRate /
					getMixer()->processingSampleRate();
	}
	else
	{
		alignedMemCpy( _ab, b, frames * sizeof( surroundSampleFrame ) );
	}

	// release lock
	unlock();

	if( getMixer()->hasFifoWriter() )
	{
		alignedFreeFrames( b );
	}

	return frames;
}




void audioDevice::stopProcessing( void )
{
	if( getMixer()->hasFifoWriter() )
	{
		while( m_inProcess )
		{
			processNextBuffer();
		}
	}
}




void audioDevice::applyQualitySettings( void )
{
	src_delete( m_srcState );

	int error;
	if( ( m_srcState = src_new(
		getMixer()->currentQualitySettings().libsrcInterpolation(),
				SURROUND_CHANNELS, &error ) ) == NULL )
	{
		printf( "Error: src_new() failed in audio_device.cpp!\n" );
	}
}




void audioDevice::registerPort( audioPort * )
{
}




void audioDevice::unregisterPort( audioPort * _port )
{
}




void audioDevice::renamePort( audioPort * )
{
}




void audioDevice::resample( const sampleFrame * _src, const fpp_t _frames,
					sampleFrame * _dst,
					const sample_rate_t _src_sr,
					const sample_rate_t _dst_sr )
{
	if( m_srcState == NULL )
	{
		return;
	}
	m_srcData.input_frames = _frames;
	m_srcData.output_frames = _frames;
	m_srcData.data_in = (float *) _src[0];
	m_srcData.data_out = _dst[0];
	m_srcData.src_ratio = (double) _dst_sr / _src_sr;
	m_srcData.end_of_input = 0;
	int error;
	if( ( error = src_process( m_srcState, &m_srcData ) ) )
	{
		printf( "audioDevice::resample(): error while resampling: %s\n",
							src_strerror( error ) );
	}
}




void audioDevice::clearS16Buffer( intSampleFrameA * _outbuf, const fpp_t _frames )
{
	alignedMemClear( _outbuf, _frames * sizeof( *_outbuf ) );
//	memset( _outbuf, 0,  _frames * channels() * BYTES_PER_INT_SAMPLE );
}




bool audioDevice::hqAudio( void ) const
{
	return( configManager::inst()->value( "mixer", "hqaudio" ).toInt() );
}



#endif
