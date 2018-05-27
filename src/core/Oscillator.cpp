/*
 * Oscillator.cpp - implementation of powerful oscillator-class
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Oscillator.h"

#include "BufferManager.h"
#include "Engine.h"
#include "Mixer.h"
#include "AutomatableModel.h"



void Oscillator::waveTableInit()
{
	if(!m_bandFreq)
	{
		m_bandFreq = new float[127];
		m_tableCount = 0;
		for (int i = 1; i < 127; i+=4 )
		{
			m_bandFreq[m_tableCount] = 440.0 * powf(2.0, (i - 69.0)/12.0);
			m_tableCount++;
		}
	}

//	printf("table %d\n",m_tableCount);
	if( !squareTables )
	{
		allocTables();
		qDebug("generate tables start \n");
		generateWaveTables();
		qDebug("generate Tables end \n" );
	}
}

Oscillator::Oscillator( const IntModel * _wave_shape_model,
				const IntModel * _mod_algo_model,
				const float & _freq,
				const float & _detuning,
				const float & _phase_offset,
				const float & _volume,
			Oscillator * _sub_osc ) :
	m_waveShapeModel( _wave_shape_model ),
	m_modulationAlgoModel( _mod_algo_model ),
	m_freq( _freq ),
	m_detuning( _detuning ),
	m_volume( _volume ),
	m_ext_phaseOffset( _phase_offset ),
	m_subOsc( _sub_osc ),
	m_phaseOffset( _phase_offset ),
	m_phase( _phase_offset ),
	m_userWave( NULL ),
	sineTable( 0 ),
	squareTable( 0 ),
	triTable( 0 ),
	sawTable( 0 )
{
	waveTableInit();
}




void Oscillator::update( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	if( m_freq >= Engine::mixer()->processingSampleRate() / 2 )
	{
		BufferManager::clear( _ab, _frames );
		return;
	}
	if( m_subOsc != NULL )
	{
		switch( m_modulationAlgoModel->value() )
		{
			case PhaseModulation:
				updatePM( _ab, _frames, _chnl );
				break;
			case AmplitudeModulation:
				updateAM( _ab, _frames, _chnl );
				break;
			case SignalMix:
				updateMix( _ab, _frames, _chnl );
				break;
			case SynchronizedBySubOsc:
				updateSync( _ab, _frames, _chnl );
				break;
			case FrequencyModulation:
				updateFM( _ab, _frames, _chnl );
		}
	}
	else
	{
		updateNoSub( _ab, _frames, _chnl );
	}
}

void Oscillator::generateSineTable( int bands )
{
	bands = bands;
	sineTable = new sample_t[ TABLE_LEN ];
	for(int i = 0; i < TABLE_LEN; i++)
	{
		sineTable[i] = sinf( ((float)i/(float)TABLE_LEN) * F_2PI );
	}
}

void Oscillator::generateSine2Table(int bands)
{
	bands = bands;
	float max = 0;
	sawTable = new sample_t[ TABLE_LEN ];
	for(int i = 0 ; i < TABLE_LEN; i++)
	{
		sawTable[i] = 0.0;
		for(int g = 1; g <= 2; g++)
		{
			sawTable[i] += sinf( ((float)i/(float)TABLE_LEN) * F_2PI * g);
		}
		max = fmax( max, sawTable[i] );
	}

	for( int i = 0; i < TABLE_LEN; i++ )
	{
		sawTable[i] /= max;
	}
}

void Oscillator::generateSawTable(int bands)
{
	float max = 0;
	sawTable = new sample_t[ TABLE_LEN ];
	for(int i = 0 ; i < TABLE_LEN; i++)
	{
		sawTable[i] = 0.0;
		for(int g = 1; g <= bands; g++)
		{
			double n = double(g);
			sawTable[i] +=powf((float)-1.0, (float) ( g + 1 ) ) *
					(1.0 /n ) * sinf( F_2PI * i * n / (float)TABLE_LEN );
		}
		max = fmax( max, sawTable[i] );
	}

	for( int i = 0; i < TABLE_LEN; i++ )
	{
		sawTable[i] /= max;
	}
}

void Oscillator::generateTriTable(int bands)
{
	float max = 0;
	triTable = new sample_t[ TABLE_LEN ];
	for(int i = 0 ; i < TABLE_LEN; i++)
	{
		triTable[i] = 0.0;
		for(int g = 0; g <= bands * 0.5; g ++)
		{
			double n = double(g);
			triTable[i] += powf((float)-1.0, (float) n ) *
					(1.0/ powf(( float )( 2 * n +1 ),
							   ( float )2.0 )) *
					sinf(F_2PI * ( 2.0 * n + 1) * i/(float)TABLE_LEN);
		}
		max = fmax( max, triTable[i] );
	}

	for( int i = 0; i < TABLE_LEN; i++ )
	{
		triTable[i] /= max;
	}
}

void Oscillator::generateSquareTable(int bands)
{
	float max = 0;
	squareTable = new sample_t[ TABLE_LEN ];
	for(int i = 0 ; i < TABLE_LEN; i++)
	{
		squareTable[i] = 0.0;
		for(int g = 1; g <= bands; g += 2)
		{
			double n = double(g);
			squareTable[i] += (1.0/n) * sinf(F_2PI * i * n / TABLE_LEN );
		}
		max = fmax( max, squareTable[i] );
	}

	for( int i = 0; i < TABLE_LEN; i++ )
	{
		squareTable[i] /= max;
	}
}

int Oscillator::bandFromFreq(float freq)
{
	int i;
	for( i = 0; i < m_tableCount; ++i )
	{
		if (m_bandFreq[i] > freq) return i;
	}
	return i;
}

sample_t **Oscillator::squareTables = 0;
sample_t **Oscillator::sineTables = 0;
sample_t **Oscillator::sawTables = 0;
sample_t **Oscillator::triTables = 0;
float* Oscillator::m_bandFreq = 0;



void Oscillator::allocTables()
{

	squareTables = new sample_t*[ m_tableCount +1];
	sineTables = new sample_t*[ m_tableCount +1 ];
	sawTables = new sample_t*[ m_tableCount +1 ];
	triTables = new sample_t*[ m_tableCount +1 ];
	memset( squareTables, 0, sizeof( sample_t ) * m_tableCount );
	memset( sineTables, 0, sizeof( sample_t ) * m_tableCount );
	memset( sawTables, 0, sizeof( sample_t ) * m_tableCount );
	memset( triTables, 0, sizeof( sample_t ) * m_tableCount );
}

void Oscillator::generateWaveTables()
{

	for(int i = 0 ; i < m_tableCount; ++i)
	{
		generateSineTable( MAX_FREQ / m_bandFreq[i] );
		sineTables[i] = sineTable;
	}

	for(int i = 0 ; i < m_tableCount; ++i)
	{
		generateSawTable( MAX_FREQ / m_bandFreq[i] );
		sawTables[i] = sawTable;
	}
	for( int i = 0; i < m_tableCount; ++i)
	{
		generateSquareTable( MAX_FREQ / m_bandFreq[i]  );
		squareTables[i] = squareTable;
	}

	for( int i = 0; i < m_tableCount; ++i )
	{
		generateTriTable( MAX_FREQ / m_bandFreq[i]  );
		triTables[i] = triTable;
	}

	for( int i = 0; i < m_tableCount; ++i )
	{
		generateSine2Table( MAX_FREQ / m_bandFreq[i] );
	}
	//	return 1;
}




void Oscillator::updateNoSub( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShapeModel->value() )
	{
		case SineWave:
		default:
			updateNoSub<SineWave>( _ab, _frames, _chnl );
			break;
		case TriangleWave:
			updateNoSub<TriangleWave>( _ab, _frames, _chnl );
			break;
		case SawWave:
			updateNoSub<SawWave>( _ab, _frames, _chnl );
			break;
		case SquareWave:
			updateNoSub<SquareWave>( _ab, _frames, _chnl );
			break;
		case MoogSawWave:
			updateNoSub<MoogSawWave>( _ab, _frames, _chnl );
			break;
		case ExponentialWave:
			updateNoSub<ExponentialWave>( _ab, _frames, _chnl );
			break;
		case WhiteNoise:
			updateNoSub<WhiteNoise>( _ab, _frames, _chnl );
			break;
		case UserDefinedWave:
			updateNoSub<UserDefinedWave>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updatePM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShapeModel->value() )
	{
		case SineWave:
		default:
			updatePM<SineWave>( _ab, _frames, _chnl );
			break;
		case TriangleWave:
			updatePM<TriangleWave>( _ab, _frames, _chnl );
			break;
		case SawWave:
			updatePM<SawWave>( _ab, _frames, _chnl );
			break;
		case SquareWave:
			updatePM<SquareWave>( _ab, _frames, _chnl );
			break;
		case MoogSawWave:
			updatePM<MoogSawWave>( _ab, _frames, _chnl );
			break;
		case ExponentialWave:
			updatePM<ExponentialWave>( _ab, _frames, _chnl );
			break;
		case WhiteNoise:
			updatePM<WhiteNoise>( _ab, _frames, _chnl );
			break;
		case UserDefinedWave:
			updatePM<UserDefinedWave>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updateAM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShapeModel->value() )
	{
		case SineWave:
		default:
			updateAM<SineWave>( _ab, _frames, _chnl );
			break;
		case TriangleWave:
			updateAM<TriangleWave>( _ab, _frames, _chnl );
			break;
		case SawWave:
			updateAM<SawWave>( _ab, _frames, _chnl );
			break;
		case SquareWave:
			updateAM<SquareWave>( _ab, _frames, _chnl );
			break;
		case MoogSawWave:
			updateAM<MoogSawWave>( _ab, _frames, _chnl );
			break;
		case ExponentialWave:
			updateAM<ExponentialWave>( _ab, _frames, _chnl );
			break;
		case WhiteNoise:
			updateAM<WhiteNoise>( _ab, _frames, _chnl );
			break;
		case UserDefinedWave:
			updateAM<UserDefinedWave>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updateMix( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShapeModel->value() )
	{
		case SineWave:
		default:
			updateMix<SineWave>( _ab, _frames, _chnl );
			break;
		case TriangleWave:
			updateMix<TriangleWave>( _ab, _frames, _chnl );
			break;
		case SawWave:
			updateMix<SawWave>( _ab, _frames, _chnl );
			break;
		case SquareWave:
			updateMix<SquareWave>( _ab, _frames, _chnl );
			break;
		case MoogSawWave:
			updateMix<MoogSawWave>( _ab, _frames, _chnl );
			break;
		case ExponentialWave:
			updateMix<ExponentialWave>( _ab, _frames, _chnl );
			break;
		case WhiteNoise:
			updateMix<WhiteNoise>( _ab, _frames, _chnl );
			break;
		case UserDefinedWave:
			updateMix<UserDefinedWave>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updateSync( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShapeModel->value() )
	{
		case SineWave:
		default:
			updateSync<SineWave>( _ab, _frames, _chnl );
			break;
		case TriangleWave:
			updateSync<TriangleWave>( _ab, _frames, _chnl );
			break;
		case SawWave:
			updateSync<SawWave>( _ab, _frames, _chnl );
			break;
		case SquareWave:
			updateSync<SquareWave>( _ab, _frames, _chnl );
			break;
		case MoogSawWave:
			updateSync<MoogSawWave>( _ab, _frames, _chnl );
			break;
		case ExponentialWave:
			updateSync<ExponentialWave>( _ab, _frames, _chnl );
			break;
		case WhiteNoise:
			updateSync<WhiteNoise>( _ab, _frames, _chnl );
			break;
		case UserDefinedWave:
			updateSync<UserDefinedWave>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updateFM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( m_waveShapeModel->value() )
	{
		case SineWave:
		default:
			updateFM<SineWave>( _ab, _frames, _chnl );
			break;
		case TriangleWave:
			updateFM<TriangleWave>( _ab, _frames, _chnl );
			break;
		case SawWave:
			updateFM<SawWave>( _ab, _frames, _chnl );
			break;
		case SquareWave:
			updateFM<SquareWave>( _ab, _frames, _chnl );
			break;
		case MoogSawWave:
			updateFM<MoogSawWave>( _ab, _frames, _chnl );
			break;
		case ExponentialWave:
			updateFM<ExponentialWave>( _ab, _frames, _chnl );
			break;
		case WhiteNoise:
			updateFM<WhiteNoise>( _ab, _frames, _chnl );
			break;
		case UserDefinedWave:
			updateFM<UserDefinedWave>( _ab, _frames, _chnl );
			break;
	}
}




// should be called every time phase-offset is changed...
inline void Oscillator::recalcPhase()
{
	if( !typeInfo<float>::isEqual( m_phaseOffset, m_ext_phaseOffset ) )
	{
		m_phase -= m_phaseOffset;
		m_phaseOffset = m_ext_phaseOffset;
		m_phase += m_phaseOffset;
	}
	m_phase = absFraction( m_phase )+2;	// make sure we're not running
						// negative when doing PM
}




inline bool Oscillator::syncOk( float _osc_coeff )
{
	const float v1 = m_phase;
	m_phase += _osc_coeff;
	// check whether m_phase is in next period
	return( floorf( m_phase ) > floorf( v1 ) );
}




float Oscillator::syncInit( sampleFrame * _ab, const fpp_t _frames,
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
template<Oscillator::WaveShapes W>
void Oscillator::updateNoSub( sampleFrame * _ab, const fpp_t _frames,
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
template<Oscillator::WaveShapes W>
void Oscillator::updatePM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning;

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		_ab[frame][_chnl] = getSample<W>( m_phase +
					_ab[frame][_chnl] )
							* m_volume;
		m_phase += osc_coeff;
	}
}




// do am by using sub-osc as modulator
template<Oscillator::WaveShapes W>
void Oscillator::updateAM( sampleFrame * _ab, const fpp_t _frames,
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
template<Oscillator::WaveShapes W>
void Oscillator::updateMix( sampleFrame * _ab, const fpp_t _frames,
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
template<Oscillator::WaveShapes W>
void Oscillator::updateSync( sampleFrame * _ab, const fpp_t _frames,
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
template<Oscillator::WaveShapes W>
void Oscillator::updateFM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning;
	const float sampleRateCorrection = 44100.0f /
				Engine::mixer()->processingSampleRate();

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		m_phase += _ab[frame][_chnl] * sampleRateCorrection;
		_ab[frame][_chnl] = getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::SineWave>(
							const float _sample )
{
	return( sinSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::TriangleWave>(
		const float _sample )
{
	return( WtTriangleSample( _sample ) );
	return( triangleSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::SawWave>(
		const float _sample )
{
	return( WtSawSample( _sample ) );
	return( sawSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::SquareWave>(
		const float _sample )
{
	return( WtSquareSample( _sample ) );
	return( squareSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::MoogSawWave>(
							const float _sample )
{
	return( moogSawSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::ExponentialWave>(
							const float _sample )
{
	return( expSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::WhiteNoise>(
							const float _sample )
{
	return( noiseSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::UserDefinedWave>(
							const float _sample )
{
	return( userWaveSample( _sample ) );
}




