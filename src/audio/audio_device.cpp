/*
 * audio_device.cpp - base-class for audio-devices used by LMMS-mixer
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>


#include "audio_device.h"
#include "buffer_allocator.h"
#include "debug.h"



audioDevice::audioDevice( Uint32 _sample_rate, Uint8 _channels ) :
	m_sampleRate( _sample_rate ),
	m_channels( _channels )
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
	unlock();
}




void audioDevice::writeBuffer( surroundSampleFrame * _ab, Uint32 _frames,
				Uint32 _src_sample_rate, float _master_gain )
{
	// make sure, no other thread is accessing device
	lock();
	// now were save to access the device
	if( _src_sample_rate != m_sampleRate )
	{
		surroundSampleFrame * temp = 
				bufferAllocator::alloc<surroundSampleFrame>(
							_frames * channels() );
		resample( _ab, _frames, temp, _src_sample_rate, m_sampleRate );
		writeBufferToDev( temp, _frames * m_sampleRate /
					_src_sample_rate, _master_gain );
		bufferAllocator::free( temp );
	}
	else
	{
		writeBufferToDev( _ab, _frames, _master_gain );
	}
	// release lock
	unlock();
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


void FASTCALL audioDevice::resample( surroundSampleFrame * _src, Uint32 _frames,
						surroundSampleFrame * _dst,
						Uint32 _src_sr, Uint32 _dst_sr )
{
#ifdef HAVE_SAMPLERATE_H
	if( m_srcState == NULL )
	{
		return;
	}
	m_srcData.input_frames = _frames;
	m_srcData.output_frames = _frames;
	m_srcData.data_in = _src[0];
	m_srcData.data_out = _dst[0];
	m_srcData.src_ratio = (float) _dst_sr / _src_sr;

	int error;
	if( ( error = src_process( m_srcState, &m_srcData ) ) )
	{
		printf( "audioDevice::resample(): error while resampling: %s\n",
							src_strerror( error ) );
	}
#else
	if( _src_sr == 2*SAMPLE_RATES[DEFAULT_QUALITY_LEVEL] )
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

		for( Uint32 frame = 0; frame < _frames; ++frame )
		{
			for( Uint8 chnl = 0; chnl < SURROUND_CHANNELS; ++chnl )
			{
				lp_hist[oldest][chnl] = _src[frame][chnl];
				if( frame % 2==0 )
				{
					_dst[frame/2][chnl] = 0.0f;
					for( Uint8 tap = 0;
						tap < LP_FILTER_TAPS; ++tap )
					{
						_dst[frame / 2][chnl] +=
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




int FASTCALL audioDevice::convertToS16( surroundSampleFrame * _ab,
					Uint32 _frames, float _master_gain,
					outputSampleType * _output_buffer,
					bool _convert_endian )
{
	for( Uint32 frame = 0; frame < _frames; ++frame )
	{
		for( Uint8 chnl = 0; chnl < channels(); ++chnl )
		{
			( _output_buffer + frame * channels() )[chnl] =
				static_cast<outputSampleType>(
					mixer::clip( _ab[frame][chnl] *
							_master_gain ) *
						OUTPUT_SAMPLE_MULTIPLIER );
		}
	}
	if( _convert_endian )
	{
		for( Uint32 frame = 0; frame < _frames; ++frame )
		{
			for( Uint8 chnl = 0; chnl < channels(); ++chnl )
			{
				Sint8 * ptr = reinterpret_cast<Sint8 *>(
						_output_buffer +
						frame * channels() +
								chnl );
				*(outputSampleType *)ptr =
					( ( outputSampleType )*ptr << 8
								) |
					( ( outputSampleType ) *
							( ptr+1 ) );
			}
		}
	}
	return( _frames * channels() * BYTES_PER_OUTPUT_SAMPLE );
}




void FASTCALL audioDevice::clearS16Buffer( outputSampleType * _outbuf,
							Uint32 _frames )
{
#ifdef LMMS_DEBUG
	assert( _outbuf != NULL );
#endif
	memset( _outbuf, 0,  _frames * channels() * BYTES_PER_OUTPUT_SAMPLE );
}

