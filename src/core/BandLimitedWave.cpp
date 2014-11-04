/*
 * BandLimitedWave.h - helper functions for band-limited
 *                    	waveform generation
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 *
 * This file is part of LMMS - http://lmms.io
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
bool BandLimitedWave::s_wavesGenerated = false;
QString BandLimitedWave::s_wavetableDir = "";


QDataStream& operator<< ( QDataStream &out, WaveMipMap &waveMipMap )
{
	for( int tbl = 0; tbl <= MAXTBL; tbl++ )
	{
		for( int i = 0; i < TLENS[tbl]; i++ )
		{
			out << waveMipMap.sampleAt( tbl, i );
		}
	}
    return out;
}

QDataStream& operator>> ( QDataStream &in, WaveMipMap &waveMipMap )
{
	sample_t sample;
	for( int tbl = 0; tbl <= MAXTBL; tbl++ )
	{
		for( int i = 0; i < TLENS[tbl]; i++ )
		{
			in >> sample;
			waveMipMap.setSampleAt( tbl, i, sample );
		}
	}
    return in;
}


void BandLimitedWave::generateWaves()
{
// don't generate if they already exist
	if( s_wavesGenerated ) return;
	
	int i;

// set wavetable directory
	s_wavetableDir = configManager::inst()->dataDir() + "wavetables/";

// set wavetable files
	QFile saw_file( s_wavetableDir + "saw.bin" );
	QFile sqr_file( s_wavetableDir + "sqr.bin" );
	QFile tri_file( s_wavetableDir + "tri.bin" );
	QFile moog_file( s_wavetableDir + "moog.bin" );

// saw wave - BLSaw
// check for file and use it if exists
	if( saw_file.exists() )
	{
		saw_file.open( QIODevice::ReadOnly );
		QDataStream in( &saw_file );
		in >> s_waveforms[ BandLimitedWave::BLSaw ];
		saw_file.close();
	}
	else
	{
		for( i = 0; i <= MAXTBL; i++ )
		{
			const int len = TLENS[i];
			//const double om = 1.0 / len;
			double max = 0.0;
			
			for( int ph = 0; ph < len; ph++ )
			{
				int harm = 1;
				double s = 0.0f;
				double hlen;
				do
				{
					hlen = static_cast<double>( len ) / static_cast<double>( harm );
					const double amp = -1.0 / static_cast<double>( harm );
					//const double a2 = cos( om * harm * F_2PI );
					s += amp * /*a2 **/sin( static_cast<double>( ph * harm ) / static_cast<double>( len ) * F_2PI );
					harm++;
				} while( hlen > 2.0 );
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
	}
	
// square wave - BLSquare
// check for file and use it if exists
	if( sqr_file.exists() )
	{
		sqr_file.open( QIODevice::ReadOnly );
		QDataStream in( &sqr_file );
		in >> s_waveforms[ BandLimitedWave::BLSquare ];
		sqr_file.close();
	}
	else
	{
		for( i = 0; i <= MAXTBL; i++ )
		{
			const int len = TLENS[i];
			//const double om = 1.0 / len;
			double max = 0.0;
			
			for( int ph = 0; ph < len; ph++ )
			{
				int harm = 1;
				double s = 0.0f;
				double hlen;
				do
				{
					hlen = static_cast<double>( len ) / static_cast<double>( harm );
					const double amp = 1.0 / static_cast<double>( harm );
					//const double a2 = cos( om * harm * F_2PI );
					s += amp * /*a2 **/ sin( static_cast<double>( ph * harm ) / static_cast<double>( len ) * F_2PI );
					harm += 2;
				} while( hlen > 2.0 );
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
	}

// triangle wave - BLTriangle
	if( tri_file.exists() )
	{
		tri_file.open( QIODevice::ReadOnly );
		QDataStream in( &tri_file );
		in >> s_waveforms[ BandLimitedWave::BLTriangle ];
		tri_file.close();
	}
	else
	{
		for( i = 0; i <= MAXTBL; i++ )
		{
			const int len = TLENS[i];
			//const double om = 1.0 / len;
			double max = 0.0;
			
			for( int ph = 0; ph < len; ph++ )
			{
				int harm = 1;
				double s = 0.0f;
				double hlen;
				do
				{
					hlen = static_cast<double>( len ) / static_cast<double>( harm );
					const double amp = 1.0 / static_cast<double>( harm * harm );
					//const double a2 = cos( om * harm * F_2PI );
					s += amp * /*a2 **/ sin( ( static_cast<double>( ph * harm ) / static_cast<double>( len ) + 
							( ( harm + 1 ) % 4 == 0 ? 0.5 : 0.0 ) ) * F_2PI );
					harm += 2;
				} while( hlen > 2.0 );
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
	}
		
// moog saw wave - BLMoog
// basically, just add in triangle + 270-phase saw
	if( moog_file.exists() )
	{
		moog_file.open( QIODevice::ReadOnly );
		QDataStream in( &moog_file );
		in >> s_waveforms[ BandLimitedWave::BLMoog ];
		moog_file.close();
	}
	else
	{
		for( i = 0; i <= MAXTBL; i++ )
		{
			const int len = TLENS[i];
			
			for( int ph = 0; ph < len; ph++ )
			{
				const int sawph = ( ph + static_cast<int>( len * 0.75 ) ) % len;
				const sample_t saw = s_waveforms[ BandLimitedWave::BLSaw ].sampleAt( i, sawph );
				const sample_t tri = s_waveforms[ BandLimitedWave::BLTriangle ].sampleAt( i, ph );
				s_waveforms[ BandLimitedWave::BLMoog ].setSampleAt( i, ph, ( saw + tri ) * 0.5f );
			}
		}
	}
	
// set the generated flag so we don't load/generate them again needlessly
	s_wavesGenerated = true;


// generate files, serialize mipmaps as QDataStreams and save them on disk
//
// normally these are now provided with LMMS as pre-generated so we don't have to do this,
// but I'm leaving the code here in case it's needed in the future 
// (maybe we add more waveforms or change the generation code or mipmap format, etc.)

/*

// if you want to generate the files, you need to set the filenames and paths here - 
// can't use the usual wavetable directory here as it can require permissions on
// some systems... 

QFile sawfile( "path-to-wavetables/saw.bin" );
QFile sqrfile( "path-to-wavetables/sqr.bin" );
QFile trifile( "path-to-wavetables/tri.bin" );
QFile moogfile( "path-to-wavetables/moog.bin" );

sawfile.open( QIODevice::WriteOnly );
QDataStream sawout( &sawfile );
sawout << s_waveforms[ BandLimitedWave::BLSaw ];
sawfile.close();

sqrfile.open( QIODevice::WriteOnly );
QDataStream sqrout( &sqrfile );
sqrout << s_waveforms[ BandLimitedWave::BLSquare ];
sqrfile.close();

trifile.open( QIODevice::WriteOnly );
QDataStream triout( &trifile );
triout << s_waveforms[ BandLimitedWave::BLTriangle ];
trifile.close();

moogfile.open( QIODevice::WriteOnly );
QDataStream moogout( &moogfile );
moogout << s_waveforms[ BandLimitedWave::BLMoog ];
moogfile.close();

*/

}
