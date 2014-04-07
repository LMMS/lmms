/*
 * BandLimitedWave.h - helper functions for band-limited
 *                    	waveform generation
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
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

#include "BandLimitedWave.h"


WaveMipMap BandLimitedWave::s_waveforms[4] = {  };


void BandLimitedWave::generateWaves()
{
	int i;

// saw wave - BLSaw
	for( i = 1; i <= MAXLEN; i++ )
	{
		const int len = 1 << i;
		const double om = 1.0 / len;
		double max = 0.0;
		
		for( int ph = 0; ph < len; ph++ )
		{
			int harm = 1;
			double s = 0.0f;
			do
			{
				const double amp = -1.0 / static_cast<double>( harm );
				const double a2 = cos( om * harm * F_2PI );
				s += amp * a2 * sin( static_cast<double>( ph * harm ) / static_cast<double>( len ) * F_2PI );
				harm++;
			} while( len/harm > 2 );
			s_waveforms[ BandLimitedWave::BLSaw ].setSampleAt( i, ph, s );
			max = qMax( max, qAbs( s ) );
		}
		// normalize
		for( int ph = 0; ph < len; ph++ )
		{
			sample_t s = s_waveforms[ BandLimitedWave::BLSaw ].sampleAt( i, ph ) / max;
			s_waveforms[ BandLimitedWave::BLSaw ].setSampleAt( i, ph, s );
		}
	}
	
// square wave - BLSquare
	for( i = 1; i <= MAXLEN; i++ )
	{
		const int len = 1 << i;
		const double om = 1.0 / len;
		double max = 0.0;
		
		for( int ph = 0; ph < len; ph++ )
		{
			int harm = 1;
			double s = 0.0f;
			do
			{
				const double amp = 1.0 / static_cast<double>( harm );
				const double a2 = cos( om * harm * F_2PI );
				s += amp * a2 * sin( static_cast<double>( ph * harm ) / static_cast<double>( len ) * F_2PI );
				harm += 2;
			} while( len/harm > 2 );
			s_waveforms[ BandLimitedWave::BLSquare ].setSampleAt( i, ph, s );
			max = qMax( max, qAbs( s ) );
		}
		// normalize
		for( int ph = 0; ph < len; ph++ )
		{
			sample_t s = s_waveforms[ BandLimitedWave::BLSquare ].sampleAt( i, ph ) / max;
			s_waveforms[ BandLimitedWave::BLSquare ].setSampleAt( i, ph, s );
		}
	}


// triangle wave - BLTriangle
	for( i = 1; i <= MAXLEN; i++ )
	{
		const int len = 1 << i;
		//const double om = 1.0 / len;
		double max = 0.0;
		
		for( int ph = 0; ph < len; ph++ )
		{
			int harm = 1;
			double s = 0.0f;
			do
			{
				const double amp = 1.0 / static_cast<double>( harm * harm );
				//const double a2 = cos( om * harm * F_2PI );
				s += amp * /*a2 **/ sin( ( static_cast<double>( ph * harm ) / static_cast<double>( len ) + 
						( ( harm + 1 ) % 4 == 0 ? 0.5 : 0.0 ) ) * F_2PI );
				harm += 2;
			} while( len/harm > 2 );
			s_waveforms[ BandLimitedWave::BLTriangle ].setSampleAt( i, ph, s );
			max = qMax( max, qAbs( s ) );
		}
		// normalize
		for( int ph = 0; ph < len; ph++ )
		{
			sample_t s = s_waveforms[ BandLimitedWave::BLTriangle ].sampleAt( i, ph ) / max;
			s_waveforms[ BandLimitedWave::BLTriangle ].setSampleAt( i, ph, s );
		}
	}
	
	
// moog saw wave - BLMoog
// basically, just add in triangle + 270-phase saw
	for( i = 1; i <= MAXLEN; i++ )
	{
		const int len = 1 << i;
		
		for( int ph = 0; ph < len; ph++ )
		{
			const int sawph = ( ph + static_cast<int>( len * 0.75 ) ) % len;
			const sample_t saw = s_waveforms[ BandLimitedWave::BLSaw ].sampleAt( i, sawph );
			const sample_t tri = s_waveforms[ BandLimitedWave::BLTriangle ].sampleAt( i, ph );
			s_waveforms[ BandLimitedWave::BLMoog ].setSampleAt( i, ph, ( saw + tri ) * 0.5f );
		}
	}
	
}
