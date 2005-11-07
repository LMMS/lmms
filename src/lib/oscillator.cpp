/*
 * oscillator.cpp - implementation of powerful oscillator-class
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "oscillator.h"



oscillator::oscillator( modulationAlgos _modulation_algo, float _freq,
				Sint16 _phase_offset, float _volume_factor,
				oscillator * _sub_osc ) :
	m_freq(_freq),
	m_volumeFactor(_volume_factor),
	m_phaseOffset(_phase_offset),
	m_subOsc(_sub_osc),
	m_userWaveData( &ZERO_FRAME ),
	m_userWaveFrames( 1 )
{

	if( m_subOsc != NULL )
	{
		switch( _modulation_algo )
		{
			case FREQ_MODULATION:
				m_callUpdate = &oscillator::updateFM;
				break;

			case AMP_MODULATION:
				m_callUpdate = &oscillator::updateAM;
				break;

			case MIX:
				m_callUpdate = &oscillator::updateMix;
				break;
			case SYNC:
				m_callUpdate = &oscillator::updateSync;
				break;
		}
	}
	else
	{
		m_callUpdate = &oscillator::updateNoSub;
	}

	recalcOscCoeff();
}



// if we have no sub-osc, we can't do any modulation... just get our samples
#define defineNoSubUpdateFor(x,getSampleFunction) 			\
void x::updateNoSub( sampleFrame * _ab, Uint32 _frames, Uint8 _chnl )	\
{									\
	for( Uint16 frame = 0; frame < _frames; ++frame )		\
	{								\
		_ab[frame][_chnl] = getSampleFunction( ++m_sample *	\
					m_oscCoeff ) * m_volumeFactor;	\
	}								\
}


// do fm by using sub-osc as modulator
#define defineFMUpdateFor(x,getSampleFunction)				\
void x::updateFM( sampleFrame * _ab, Uint32 _frames, Uint8 _chnl )	\
{									\
	m_subOsc->update( _ab, _frames, _chnl );			\
	for( Uint16 frame = 0; frame < _frames; ++frame )		\
	{								\
		_ab[frame][_chnl] = getSampleFunction( ++m_sample *	\
							m_oscCoeff +	\
						_ab[frame][_chnl] ) *	\
							m_volumeFactor;	\
		/* following line is REAL FM */				\
/*		float new_freq = powf( 2.0, _ab[frame][_chnl] );	\
		_ab[frame][_chnl] = getSampleFunction( ++m_sample*((m_freq * \
		new_freq )/mixer::inst()->sampleRate() )) * m_volumeFactor;  \
		_ab[frame][_chnl] = getSampleFunction( ++m_sample*(m_oscCoeff *\
			_ab[frame][_chnl] )) * m_volumeFactor;*/	\
	}								\
}


// do am by using sub-osc as modulator
#define defineAMUpdateFor(x,getSampleFunction)				\
void x::updateAM( sampleFrame * _ab, Uint32 _frames, Uint8 _chnl )	\
{									\
	m_subOsc->update( _ab, _frames, _chnl );			\
	for( Uint16 frame = 0; frame < _frames; ++frame )		\
	{								\
		_ab[frame][_chnl] *= getSampleFunction( ++m_sample *	\
					m_oscCoeff ) * m_volumeFactor;	\
	}								\
}


// do mix by using sub-osc as mix-sample
#define defineMixUpdateFor(x,getSampleFunction)				\
void x::updateMix( sampleFrame * _ab, Uint32 _frames, Uint8 _chnl )	\
{									\
	m_subOsc->update( _ab, _frames, _chnl );			\
	for( Uint16 frame = 0; frame < _frames; ++frame )		\
	{								\
		_ab[frame][_chnl] += getSampleFunction( ++m_sample *	\
					m_oscCoeff ) * m_volumeFactor;	\
	}								\
}


// sync with sub-osc (every time sub-osc starts new period, we also start new
// period)
#define defineSyncUpdateFor(x,getSampleFunction)			\
void x::updateSync( sampleFrame * _ab, Uint32 _frames, Uint8 _chnl )	\
{									\
	for( Uint16 frame = 0; frame < _frames ; ++frame )		\
	{								\
		if( m_subOsc->syncOk() )				\
		{							\
			sync();						\
		}							\
		_ab[frame][_chnl] = getSampleFunction( ++m_sample *	\
					m_oscCoeff ) * m_volumeFactor;	\
	}								\
}



#define generateOscillatorCodeFor(x,getSampleFunction);			\
class x : public oscillator						\
{									\
public:									\
	x( modulationAlgos modulation_algo, float _freq, Sint16 _phase_offset, \
			float _volume_factor, oscillator * _sub_osc) :	\
	oscillator (modulation_algo, _freq, _phase_offset, _volume_factor, \
								_sub_osc ) \
	{								\
	}								\
									\
protected:								\
	void updateNoSub( sampleFrame * _ab, Uint32 _frames,		\
							Uint8 _chnl );	\
	void updateFM( sampleFrame * _ab, Uint32 _frames,		\
							Uint8 _chnl );	\
	void updateAM( sampleFrame * _ab, Uint32 _frames,		\
							Uint8 _chnl );	\
	void updateMix( sampleFrame * _ab, Uint32 _frames,		\
							Uint8 _chnl );	\
	void updateSync( sampleFrame * _ab, Uint32 _frames,		\
							Uint8 _chnl );	\
									\
} ;									\
									\
defineNoSubUpdateFor( x, getSampleFunction )				\
defineFMUpdateFor( x, getSampleFunction )				\
defineAMUpdateFor( x, getSampleFunction )				\
defineMixUpdateFor( x, getSampleFunction )				\
defineSyncUpdateFor( x, getSampleFunction )


generateOscillatorCodeFor( sinWaveOsc, sinSample );
generateOscillatorCodeFor( triangleWaveOsc, triangleSample );
generateOscillatorCodeFor( sawWaveOsc, sawSample );
generateOscillatorCodeFor( squareWaveOsc, squareSample );
generateOscillatorCodeFor( moogSawWaveOsc, moogSawSample );
generateOscillatorCodeFor( expWaveOsc, expSample );
generateOscillatorCodeFor( noiseWaveOsc, noiseSample );
generateOscillatorCodeFor( userWaveOsc, userWaveSample );



oscillator * oscillator::createOsc( waveShapes _wave_shape,
					modulationAlgos _modulation_algo,
					float _freq, Sint16 _phase_offset,
					float _volume_factor,
							oscillator * _sub_osc )
{
	switch( _wave_shape )
	{
		case SIN_WAVE:
			return( new sinWaveOsc( _modulation_algo, _freq,
						_phase_offset, _volume_factor,
								_sub_osc ) );
		case TRIANGLE_WAVE:
			return( new triangleWaveOsc( _modulation_algo, _freq,
						_phase_offset, _volume_factor,
								_sub_osc ) );
		case SAW_WAVE:
			return( new sawWaveOsc( _modulation_algo, _freq,
						_phase_offset, _volume_factor,
								_sub_osc ) );
		case SQUARE_WAVE:
			return( new squareWaveOsc( _modulation_algo, _freq,
						_phase_offset, _volume_factor,
								_sub_osc ) );
		case MOOG_SAW_WAVE:
			return( new moogSawWaveOsc( _modulation_algo, _freq,
						_phase_offset, _volume_factor,
								_sub_osc ) );
		case EXP_WAVE:
			return( new expWaveOsc( _modulation_algo, _freq,
						_phase_offset, _volume_factor,
								_sub_osc ) );
		case WHITE_NOISE_WAVE:
			return( new noiseWaveOsc( _modulation_algo, _freq,
						_phase_offset, _volume_factor,
								_sub_osc ) );
		case USER_DEF_WAVE:
			return( new userWaveOsc( _modulation_algo, _freq,
						_phase_offset, _volume_factor,
								_sub_osc ) );
		default:
			return( new sinWaveOsc( _modulation_algo, _freq,
						_phase_offset, _volume_factor,
								_sub_osc ) );
	}

}




// should be called every time phase-offset or frequency is changed...
void oscillator::recalcOscCoeff( const float additional_phase_offset )
{
	m_oscCoeff = m_freq / static_cast<float>( mixer::inst()->sampleRate() );

	m_sample = static_cast<Uint32>( ( m_phaseOffset * ( 1.0f / 360.0f ) +
						additional_phase_offset ) *
						( mixer::inst()->sampleRate() /
								m_freq ) );
	// because we pre-increment m_sample in update-function, we should
	// decrement it here... (not possible when 0 because it is
	// unsigned - overflow!!!)
	if( m_sample > 0 )
	{
		--m_sample;
	}
}
