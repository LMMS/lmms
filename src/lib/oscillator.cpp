#ifndef SINGLE_SOURCE_COMPILE

/*
 * oscillator.cpp - implementation of powerful oscillator-class
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


#include "oscillator.h"



oscillator::oscillator( const waveShapes * _wave_shape,
			const modulationAlgos * _modulation_algo,
			const float * _freq,
			const float * _detuning,
			const float * _phase_offset,
			const float * _volume,
			oscillator * _sub_osc ) :
	m_waveShape( _wave_shape ),
	m_modulationAlgo( _modulation_algo ),
	m_freq( _freq ),
	m_detuning( _detuning ),
	m_volume( _volume ),
	m_ext_phaseOffset( _phase_offset ),
	m_subOsc( _sub_osc ),
	m_userWave( NULL )
{
	m_phaseOffset = *m_ext_phaseOffset;
	m_phase = m_phaseOffset;
}




void oscillator::update( sampleFrame * _ab, const fpab_t _frames,
							const ch_cnt_t _chnl )
{
	if( m_userWave )
	{
		m_userWave->lock();
	}

	if( m_subOsc != NULL )
	{
		switch( *m_modulationAlgo )
		{
			case PHASE_MODULATION:
				updatePM( _ab, _frames, _chnl );
				break;
			case AMP_MODULATION:
				updateAM( _ab, _frames, _chnl );
				break;
			case MIX:
				updateMix( _ab, _frames, _chnl );
				break;
			case SYNC:
				updateSync( _ab, _frames, _chnl );
				break;
			case FREQ_MODULATION:
				updateFM( _ab, _frames, _chnl );
		}
	}
	else
	{
		updateNoSub( _ab, _frames, _chnl );
	}

	if( m_userWave )
	{
		m_userWave->unlock();
	}
}




// if we have no sub-osc, we can't do any modulation... just get our samples
void oscillator::updateNoSub( sampleFrame * _ab, const fpab_t _frames,
						const ch_cnt_t _chnl )
{
	recalcPhase();
	float osc_coeff = *m_freq * *m_detuning;

	for( fpab_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] = getSample( m_phase ) * *m_volume;
		m_phase += osc_coeff;
	}
}


// do pm by using sub-osc as modulator
void oscillator::updatePM( sampleFrame * _ab, const fpab_t _frames,
						const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = *m_freq * *m_detuning;

	for( fpab_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] = getSample( m_phase + _ab[frame][_chnl] )
								* *m_volume;
		m_phase += osc_coeff;
	}
}


// do am by using sub-osc as modulator
void oscillator::updateAM( sampleFrame * _ab, const fpab_t _frames,
						const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = *m_freq * *m_detuning;

	for( fpab_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] *= getSample( m_phase ) * *m_volume;
		m_phase += osc_coeff;
	}
}


// do mix by using sub-osc as mix-sample
void oscillator::updateMix( sampleFrame * _ab, const fpab_t _frames,
						const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = *m_freq * *m_detuning;

	for( fpab_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] += getSample( m_phase ) * *m_volume;
		m_phase += osc_coeff;
	}
}


// sync with sub-osc (every time sub-osc starts new period, we also start new
// period)
void oscillator::updateSync( sampleFrame * _ab, const fpab_t _frames,
						const ch_cnt_t _chnl )
{
	const float sub_osc_coeff = m_subOsc->syncInit( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = *m_freq * *m_detuning;

	for( fpab_t frame = 0; frame < _frames ; ++frame )
	{
		if( m_subOsc->syncOk( sub_osc_coeff ) )
		{
			m_phase = m_phaseOffset;
		}
		_ab[frame][_chnl] = getSample( m_phase ) * *m_volume;
		m_phase += osc_coeff;
	}
}




// do fm by using sub-osc as modulator
void oscillator::updateFM( sampleFrame * _ab, const fpab_t _frames,
						const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = *m_freq * *m_detuning;

	for( fpab_t frame = 0; frame < _frames; ++frame )
	{
		m_phase += _ab[frame][_chnl];
		_ab[frame][_chnl] = getSample( m_phase ) * *m_volume;
		m_phase += osc_coeff;
	}
}




inline sample_t oscillator::getSample( const float _sample )
{
	switch( *m_waveShape )
	{
		case SIN_WAVE:
			return( sinSample( _sample ) );
		case TRIANGLE_WAVE:
			return( triangleSample( _sample ) );
		case SAW_WAVE:
			return( sawSample( _sample ) );
		case SQUARE_WAVE:
			return( squareSample( _sample ) );
		case MOOG_SAW_WAVE:
			return( moogSawSample( _sample ) );
		case EXP_WAVE:
			return( expSample( _sample ) );
		case WHITE_NOISE_WAVE:
			return( noiseSample( _sample ) );
		case USER_DEF_WAVE:
			return( userWaveSample( _sample ) );
		default:
			return( sinSample( _sample ) );
	}

}




// should be called every time phase-offset is changed...
inline void oscillator::recalcPhase( void )
{
	if( m_phaseOffset != *m_ext_phaseOffset )
	{
		m_phase -= m_phaseOffset;
		m_phaseOffset = *m_ext_phaseOffset;
		m_phase += m_phaseOffset;
	}
	m_phase = fraction( m_phase );
}




inline bool oscillator::syncOk( float _osc_coeff )
{
	const float v1 = m_phase;
	m_phase += _osc_coeff;
	// check whether m_phase is in next period
	return( floorf( m_phase ) > floorf( v1 ) );
}




float oscillator::syncInit( sampleFrame * _ab, const fpab_t _frames,
						const ch_cnt_t _chnl )
{
	if( m_subOsc != NULL )
	{
		m_subOsc->update( _ab, _frames, _chnl );
	}
	recalcPhase();
	return( *m_freq * *m_detuning );
}

#endif
