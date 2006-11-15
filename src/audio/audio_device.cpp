#ifndef SINGLE_SOURCE_COMPILE

/*
 * audio_device.cpp - base-class for audio-devices used by LMMS-mixer
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>


#include "audio_device.h"
#include "buffer_allocator.h"
#include "debug.h"



audioDevice::audioDevice( const sample_rate_t _sample_rate,
				const ch_cnt_t _channels, mixer * _mixer ) :
	m_sampleRate( _sample_rate ),
	m_channels( _channels ),
	m_mixer( _mixer ),
	m_buffer( bufferAllocator::alloc<surroundSampleFrame>(
				getMixer()->framesPerAudioBuffer() ) )
{
#ifdef HAVE_SAMPLERATE_H
	int error;
	if( ( m_srcState = src_new(
#ifdef HQ_SINC
					SRC_SINC_BEST_QUALITY,
#else
					SRC_SINC_FASTEST,
#endif
				SURROUND_CHANNELS, &error ) ) == NULL )
	{
		printf( "Error: src_new() failed in audio_device.cpp!\n" );
	}
	m_srcData.end_of_input = 0;
#endif
}




audioDevice::~audioDevice()
{
#ifdef HAVE_SAMPLERATE_H
	src_delete( m_srcState );
#endif
	bufferAllocator::free( m_buffer );
#ifdef QT3
	if( m_devMutex.locked() )
	{
		unlock();
	}
#else
	m_devMutex.tryLock();
	unlock();
#endif
}




void audioDevice::processNextBuffer( void )
{
	const fpab_t frames = getNextBuffer( m_buffer );
	writeBuffer( m_buffer, frames, getMixer()->masterGain() );
}




fpab_t audioDevice::getNextBuffer( surroundSampleFrame * _ab )
{
	fpab_t frames = getMixer()->framesPerAudioBuffer();
	const surroundSampleFrame * b = getMixer()->renderNextBuffer();

	// make sure, no other thread is accessing device
	lock();

	// now were safe to access the device
	if( getMixer()->sampleRate() != m_sampleRate )
	{
		resample( b, frames, _ab, getMixer()->sampleRate(),
								m_sampleRate );
		frames = frames * m_sampleRate / getMixer()->sampleRate();
	}
	else
	{
		memcpy( _ab, b, frames * sizeof( surroundSampleFrame ) );
	}

	// release lock
	unlock();

	return( frames );
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




#ifndef HAVE_SAMPLERATE_H
const Uint8 LP_FILTER_TAPS = 24;
const float LP_FILTER_COEFFS[LP_FILTER_TAPS] =
{
	+0.000511851442,
	-0.001446936402,
	-0.005058312516,
	-0.002347181570,
	+0.011236146012,
	+0.020351310667,
	-0.000479735368,
	-0.045333228189
	-0.055186434405,
	+0.032962246498,
	+0.202439670159,
	+0.342350604673,
	+0.342350604673,
	+0.202439670159,
	+0.032962246498,
	-0.055186434405,
	-0.045333228189
	-0.000479735368,
	+0.020351310667,
	+0.011236146012,
	-0.002347181570,
	-0.005058312516,
	-0.001446936402,
	+0.000511851442
} ;
#endif


void FASTCALL audioDevice::resample( const surroundSampleFrame * _src,
						const fpab_t _frames,
						surroundSampleFrame * _dst,
						const sample_rate_t _src_sr,
						const sample_rate_t _dst_sr )
{
#ifdef HAVE_SAMPLERATE_H
	if( m_srcState == NULL )
	{
		return;
	}
	m_srcData.input_frames = _frames;
	m_srcData.output_frames = _frames;
	m_srcData.data_in = (float *) _src[0];
	m_srcData.data_out = _dst[0];
	m_srcData.src_ratio = (float) _dst_sr / _src_sr;

	int error;
	if( ( error = src_process( m_srcState, &m_srcData ) ) )
	{
		printf( "audioDevice::resample(): error while resampling: %s\n",
							src_strerror( error ) );
	}
#else
	if( _src_sr == 2 * SAMPLE_RATES[DEFAULT_QUALITY_LEVEL] )
	{
		// we use a simple N-tap FIR-Filter with
		// precalculated/-designed LP-Coeffs
		static surroundSampleFrame lp_hist[LP_FILTER_TAPS] =
		{
#ifndef DISABLE_SURROUND
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 0.0f }
#else
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f }
#endif
		} ;
		static Uint8 oldest = 0;

		for( fpab_t frame = 0; frame < _frames; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < SURROUND_CHANNELS;
									++chnl )
			{
				lp_hist[oldest][chnl] = _src[frame][chnl];
				if( frame % 2 == 0 )
				{
					const fpab_t f = frame / 2;
					_dst[f][chnl] = 0.0f;
					for( Uint8 tap = 0;
						tap < LP_FILTER_TAPS; ++tap )
					{
						_dst[f][chnl] +=
LP_FILTER_COEFFS[tap] * lp_hist[( oldest + tap ) % LP_FILTER_TAPS][chnl];
					}
				}
			}
			oldest = ( oldest + 1 ) % LP_FILTER_TAPS;
		}
	}
	else if( _src_sr == SAMPLE_RATES[DEFAULT_QUALITY_LEVEL] / 2 )
	{
		printf( "No resampling for given sample-rates implemented!\n"
			"Consider installing libsamplerate and recompile "
								"LMMS!\n" );
	}
#endif
}




Uint32 FASTCALL audioDevice::convertToS16( const surroundSampleFrame * _ab,
						const fpab_t _frames,
						const float _master_gain,
						int_sample_t * _output_buffer,
						const bool _convert_endian )
{
	if( _convert_endian )
	{
		Uint16 temp;
		for( fpab_t frame = 0; frame < _frames; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < channels(); ++chnl )
			{
				temp = static_cast<int_sample_t>(
						mixer::clip( _ab[frame][chnl] *
						_master_gain ) *
						OUTPUT_SAMPLE_MULTIPLIER );
				
				( _output_buffer + frame * channels() )[chnl] =
						( temp & 0x00ff ) << 8 |
						( temp & 0xff00 ) >> 8;
			}
		}
	}
	else
	{
		for( fpab_t frame = 0; frame < _frames; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < channels(); ++chnl )
			{
				( _output_buffer + frame * channels() )[chnl] =
						static_cast<int_sample_t>(
						mixer::clip( _ab[frame][chnl] *
						_master_gain ) *
						OUTPUT_SAMPLE_MULTIPLIER );
			}
		}
	}

	return( _frames * channels() * BYTES_PER_INT_SAMPLE );
}




void FASTCALL audioDevice::clearS16Buffer( int_sample_t * _outbuf,
							const fpab_t _frames )
{
#ifdef LMMS_DEBUG
	assert( _outbuf != NULL );
#endif
	memset( _outbuf, 0,  _frames * channels() * BYTES_PER_INT_SAMPLE );
}


#endif
