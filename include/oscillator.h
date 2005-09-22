/*
 * oscillator.h - header-file for oscillator.cpp, a powerful oscillator-class
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


#ifndef _OSCILLATOR_H
#define _OSCILLATOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "mixer.h"
#include "interpolation.h"


// fwd-decl because we need it for the typedef below...
class oscillator;

typedef void ( oscillator:: * oscFuncPtr )
			( sampleFrame * _ab, Uint32 _frames, Uint8 _chnl );



class oscillator
{
public:
	enum waveShapes
	{
		SIN_WAVE,
		TRIANGLE_WAVE,
		SAW_WAVE,
		SQUARE_WAVE,
		MOOG_SAW_WAVE,
		EXP_WAVE,
		WHITE_NOISE_WAVE,
		USER_DEF_WAVE
	} ;

	enum modulationAlgos
	{
		FREQ_MODULATION, AMP_MODULATION, MIX, SYNC
	} ;

	oscillator( modulationAlgos _modulation_algo, float _freq,
				Sint16 _phase_offset, float _volume_factor,
				oscillator * _m_subOsc );
	inline virtual ~oscillator()
	{
		delete m_subOsc;
	}
	inline void setUserWave( const sampleFrame * _data, Uint32 _frames )
	{
		m_userWaveData = _data;
		m_userWaveFrames = _frames;
	}
	inline void update( sampleFrame * _ab, Uint32 _frames, Uint8 _chnl )
	{
		( this->*m_callUpdate )( _ab, _frames, _chnl );
	}
	inline void setNewFreq( float _new_freq )
	{
		// save current state - we need it later for restoring same
		// phase (otherwise we'll get clicks in the audio-stream)
		const float v = m_sample * m_oscCoeff;
		m_freq = _new_freq;
		recalcOscCoeff( phase( v ) );
	}

	static oscillator * FASTCALL createNewOsc( waveShapes _wave_shape,
				modulationAlgos _modulation_algo, float _freq,
				Sint16 _phase_offset, float _volume_factor,
						oscillator * _m_subOsc = NULL );

	inline bool syncOk( void )
	{
		const float v1 = m_sample * m_oscCoeff;
		const float v2 = ++m_sample * m_oscCoeff;
		// check whether v2 is in next period
		return( floorf( v2 ) > floorf( v1 ) );
	}
	static inline float phase( float _sample )
	{
		float t;
		return( modff( _sample, &t ) );
		//return( _sample - floorf( _sample ) );
	}
	// now follow the wave-shape-routines...
	static inline sampleType sinSample( float _sample )
	{
		return( sinf( _sample * static_cast<sampleType>( 2.0f * M_PI
									) ) );
	}

	static inline sampleType triangleSample( float _sample )
	{
		const float ph = phase( _sample );
		if( ph <= 0.25f )
		{
			return( ph * 4.0f );
		}
		else if( ph <= 0.75f )
		{
			return( 2.0f - ph * 4.0f );
		}
		return( ph * 4.0f - 4.0f );
	}

	static inline sampleType sawSample( float _sample )
	{
		return( -1.0f + phase( _sample ) * 2.0f );
	}

	static inline sampleType squareSample( float _sample )
	{
		return( ( phase( _sample ) > 0.5f ) ? -1.0f : 1.0f );
	}

	static inline sampleType moogSawSample( float _sample )
	{
		const float ph= phase( _sample );
		if( ph < 0.5f )
		{
			return( -1.0f + ph * 4.0f );
		}
		return( 1.0f - 2.0f * ph );
	}

	static inline sampleType expSample( float _sample )
	{
		float ph = phase( _sample );
		if( ph > 0.5f )
		{
			ph = 1.0f - ph;
		}
		return( -1.0f + 8.0f * ph * ph );
	}

	static inline sampleType noiseSample( float )
	{
		return( 1.0f - 2.0f * ( ( float )rand() * ( 1.0f /
								RAND_MAX ) ) );
	}
	inline sampleType userWaveSample( float _sample )
	{
		const float frame = phase( _sample ) * m_userWaveFrames;
		const Uint32 f1 = static_cast<Uint32>( frame );
		const Uint32 f2 = ( f1 + 1 ) % m_userWaveFrames;
		return( linearInterpolate( m_userWaveData[f1][0],
						m_userWaveData[f2][0],
						frame - floorf( frame ) ) );
	}


protected:
	float m_freq;
	float m_volumeFactor;
	Sint16 m_phaseOffset;
	oscillator * m_subOsc;
	Uint32 m_sample;
	float m_oscCoeff;
	sampleFrame const * m_userWaveData;
	Uint32 m_userWaveFrames;
	oscFuncPtr m_callUpdate;


	virtual void FASTCALL updateNoSub( sampleFrame * _ab, Uint32 _frames,
							Uint8 _chnl ) = 0;
	virtual void FASTCALL updateFM( sampleFrame * _ab, Uint32 _frames,
							Uint8 _chnl ) = 0;
	virtual void FASTCALL updateAM( sampleFrame * _ab, Uint32 _frames,
							Uint8 _chnl ) = 0;
	virtual void FASTCALL updateMix( sampleFrame * _ab, Uint32 _frames,
							Uint8 _chnl ) = 0;
	virtual void FASTCALL updateSync( sampleFrame * _ab, Uint32 _frames,
							Uint8 _chnl ) = 0;

	inline void sync( void )
	{
		m_sample = 0;
	}
	void FASTCALL recalcOscCoeff( const float _additional_phase_offset =
									0.0 );

} ;


#endif
