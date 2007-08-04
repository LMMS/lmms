#ifndef SINGLE_SOURCE_COMPILE

/*
 * oscillator.cpp - implementation of powerful oscillator-class
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



oscillator::oscillator( const waveShapes & _wave_shape,
			const modulationAlgos & _modulation_algo,
			const float & _freq,
			const float & _detuning,
			const float & _phase_offset,
			const float & _volume,
			oscillator * _sub_osc ) :
	m_waveShape( _wave_shape ),
	m_modulationAlgo( _modulation_algo ),
	m_freq( _freq ),
	m_detuning( _detuning ),
	m_volume( _volume ),
	m_ext_phaseOffset( _phase_offset ),
	m_subOsc( _sub_osc ),
	m_phaseOffset( _phase_offset ),
	m_phase( _phase_offset ),
	m_userWave( NULL )
{
}




void oscillator::update( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	if( m_subOsc != NULL )
	{
		switch( m_modulationAlgo )
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
}




void oscillator::updateNoSub( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShape )
	{
		case SIN_WAVE:
		default:
			updateNoSub<SIN_WAVE>( _ab, _frames, _chnl );
			break;
		case TRIANGLE_WAVE:
			updateNoSub<TRIANGLE_WAVE>( _ab, _frames, _chnl );
			break;
		case SAW_WAVE:
			updateNoSub<SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case SQUARE_WAVE:
			updateNoSub<SQUARE_WAVE>( _ab, _frames, _chnl );
			break;
		case MOOG_SAW_WAVE:
			updateNoSub<MOOG_SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case EXP_WAVE:
			updateNoSub<EXP_WAVE>( _ab, _frames, _chnl );
			break;
		case WHITE_NOISE_WAVE:
			updateNoSub<WHITE_NOISE_WAVE>( _ab, _frames, _chnl );
			break;
		case USER_DEF_WAVE:
			updateNoSub<USER_DEF_WAVE>( _ab, _frames, _chnl );
			break;
	}
}




void oscillator::updatePM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShape )
	{
		case SIN_WAVE:
		default:
			updatePM<SIN_WAVE>( _ab, _frames, _chnl );
			break;
		case TRIANGLE_WAVE:
			updatePM<TRIANGLE_WAVE>( _ab, _frames, _chnl );
			break;
		case SAW_WAVE:
			updatePM<SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case SQUARE_WAVE:
			updatePM<SQUARE_WAVE>( _ab, _frames, _chnl );
			break;
		case MOOG_SAW_WAVE:
			updatePM<MOOG_SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case EXP_WAVE:
			updatePM<EXP_WAVE>( _ab, _frames, _chnl );
			break;
		case WHITE_NOISE_WAVE:
			updatePM<WHITE_NOISE_WAVE>( _ab, _frames, _chnl );
			break;
		case USER_DEF_WAVE:
			updatePM<USER_DEF_WAVE>( _ab, _frames, _chnl );
			break;
	}
}




void oscillator::updateAM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShape )
	{
		case SIN_WAVE:
		default:
			updateAM<SIN_WAVE>( _ab, _frames, _chnl );
			break;
		case TRIANGLE_WAVE:
			updateAM<TRIANGLE_WAVE>( _ab, _frames, _chnl );
			break;
		case SAW_WAVE:
			updateAM<SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case SQUARE_WAVE:
			updateAM<SQUARE_WAVE>( _ab, _frames, _chnl );
			break;
		case MOOG_SAW_WAVE:
			updateAM<MOOG_SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case EXP_WAVE:
			updateAM<EXP_WAVE>( _ab, _frames, _chnl );
			break;
		case WHITE_NOISE_WAVE:
			updateAM<WHITE_NOISE_WAVE>( _ab, _frames, _chnl );
			break;
		case USER_DEF_WAVE:
			updateAM<USER_DEF_WAVE>( _ab, _frames, _chnl );
			break;
	}
}




void oscillator::updateMix( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShape )
	{
		case SIN_WAVE:
		default:
			updateMix<SIN_WAVE>( _ab, _frames, _chnl );
			break;
		case TRIANGLE_WAVE:
			updateMix<TRIANGLE_WAVE>( _ab, _frames, _chnl );
			break;
		case SAW_WAVE:
			updateMix<SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case SQUARE_WAVE:
			updateMix<SQUARE_WAVE>( _ab, _frames, _chnl );
			break;
		case MOOG_SAW_WAVE:
			updateMix<MOOG_SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case EXP_WAVE:
			updateMix<EXP_WAVE>( _ab, _frames, _chnl );
			break;
		case WHITE_NOISE_WAVE:
			updateMix<WHITE_NOISE_WAVE>( _ab, _frames, _chnl );
			break;
		case USER_DEF_WAVE:
			updateMix<USER_DEF_WAVE>( _ab, _frames, _chnl );
			break;
	}
}




void oscillator::updateSync( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShape )
	{
		case SIN_WAVE:
		default:
			updateSync<SIN_WAVE>( _ab, _frames, _chnl );
			break;
		case TRIANGLE_WAVE:
			updateSync<TRIANGLE_WAVE>( _ab, _frames, _chnl );
			break;
		case SAW_WAVE:
			updateSync<SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case SQUARE_WAVE:
			updateSync<SQUARE_WAVE>( _ab, _frames, _chnl );
			break;
		case MOOG_SAW_WAVE:
			updateSync<MOOG_SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case EXP_WAVE:
			updateSync<EXP_WAVE>( _ab, _frames, _chnl );
			break;
		case WHITE_NOISE_WAVE:
			updateSync<WHITE_NOISE_WAVE>( _ab, _frames, _chnl );
			break;
		case USER_DEF_WAVE:
			updateSync<USER_DEF_WAVE>( _ab, _frames, _chnl );
			break;
	}
}




void oscillator::updateFM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShape )
	{
		case SIN_WAVE:
		default:
			updateFM<SIN_WAVE>( _ab, _frames, _chnl );
			break;
		case TRIANGLE_WAVE:
			updateFM<TRIANGLE_WAVE>( _ab, _frames, _chnl );
			break;
		case SAW_WAVE:
			updateFM<SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case SQUARE_WAVE:
			updateFM<SQUARE_WAVE>( _ab, _frames, _chnl );
			break;
		case MOOG_SAW_WAVE:
			updateFM<MOOG_SAW_WAVE>( _ab, _frames, _chnl );
			break;
		case EXP_WAVE:
			updateFM<EXP_WAVE>( _ab, _frames, _chnl );
			break;
		case WHITE_NOISE_WAVE:
			updateFM<WHITE_NOISE_WAVE>( _ab, _frames, _chnl );
			break;
		case USER_DEF_WAVE:
			updateFM<USER_DEF_WAVE>( _ab, _frames, _chnl );
			break;
	}
}




// should be called every time phase-offset is changed...
inline void oscillator::recalcPhase( void )
{
	if( m_phaseOffset != m_ext_phaseOffset )
	{
		m_phase -= m_phaseOffset;
		m_phaseOffset = m_ext_phaseOffset;
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




float oscillator::syncInit( sampleFrame * _ab, const fpp_t _frames,
						const ch_cnt_t _chnl )
{
	if( m_subOsc != NULL )
	{
		m_subOsc->update( _ab, _frames, _chnl );
	}
	recalcPhase();
	return( m_freq * m_detuning );
}




// if we have no sub-osc, we can't do any modulation... just get our samples
template<oscillator::waveShapes W>
void oscillator::updateNoSub( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning;

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] = getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}




// do pm by using sub-osc as modulator
template<oscillator::waveShapes W>
void oscillator::updatePM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning;

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] = getSample<W>( m_phase + _ab[frame][_chnl] )
								* m_volume;
		m_phase += osc_coeff;
	}
}




// do am by using sub-osc as modulator
template<oscillator::waveShapes W>
void oscillator::updateAM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning;

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] *= getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}




// do mix by using sub-osc as mix-sample
template<oscillator::waveShapes W>
void oscillator::updateMix( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning;

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] += getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}




// sync with sub-osc (every time sub-osc starts new period, we also start new
// period)
template<oscillator::waveShapes W>
void oscillator::updateSync( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	const float sub_osc_coeff = m_subOsc->syncInit( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning;

	for( fpp_t frame = 0; frame < _frames ; ++frame )
	{
		if( m_subOsc->syncOk( sub_osc_coeff ) )
		{
			m_phase = m_phaseOffset;
		}
		_ab[frame][_chnl] = getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}




// do fm by using sub-osc as modulator
template<oscillator::waveShapes W>
void oscillator::updateFM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning;

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		m_phase += _ab[frame][_chnl];
		_ab[frame][_chnl] = getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}




template<>
inline sample_t oscillator::getSample<oscillator::SIN_WAVE>(
							const float _sample )
{
	return( sinSample( _sample ) );
}




template<>
inline sample_t oscillator::getSample<oscillator::TRIANGLE_WAVE>(
							const float _sample )
{
	return( triangleSample( _sample ) );
}




template<>
inline sample_t oscillator::getSample<oscillator::SAW_WAVE>(
							const float _sample )
{
	return( sawSample( _sample ) );
}




template<>
inline sample_t oscillator::getSample<oscillator::SQUARE_WAVE>(
							const float _sample )
{
	return( squareSample( _sample ) );
}




template<>
inline sample_t oscillator::getSample<oscillator::MOOG_SAW_WAVE>(
							const float _sample )
{
	return( moogSawSample( _sample ) );
}




template<>
inline sample_t oscillator::getSample<oscillator::EXP_WAVE>(
							const float _sample )
{
	return( expSample( _sample ) );
}




template<>
inline sample_t oscillator::getSample<oscillator::WHITE_NOISE_WAVE>(
							const float _sample )
{
	return( noiseSample( _sample ) );
}




template<>
inline sample_t oscillator::getSample<oscillator::USER_DEF_WAVE>(
							const float _sample )
{
	return( userWaveSample( _sample ) );
}




#endif
