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
	if(!s_waveTableBandFreqs)
	{
		s_waveTableBandFreqs = new float[127];
		s_waveTablesPerWaveformCount = 0;
		for (int i = 1; i < 127; i+=4)
		{
			s_waveTableBandFreqs[s_waveTablesPerWaveformCount] = 440.0 * powf(2.0, (i - 69.0) / 12.0);
			s_waveTablesPerWaveformCount++;
		}
	}

//	printf("table %d\n",m_tableCount);
	if (!s_waveTables)
	{
		allocWaveTables();
		generateWaveTables();
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
	m_generatedWaveTable( 0 )
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

void Oscillator::generateSineWaveTable(int bands)
{
	bands = bands;
	m_generatedWaveTable = new sample_t[WAVETABLE_LENGTH];
	for (int i = 0; i < WAVETABLE_LENGTH; i++)
	{
		m_generatedWaveTable[i] = sinf(((float)i / (float)WAVETABLE_LENGTH) * F_2PI);
	}
}

void Oscillator::generateSawWaveTable(int bands)
{
	float max = 0;
	m_generatedWaveTable = new sample_t[WAVETABLE_LENGTH];
	for (int i = 0; i < WAVETABLE_LENGTH; i++)
	{
		m_generatedWaveTable[i] = 0.0;
		for (int g = 1; g <= bands; g++)
		{
			double n = double(g);
			m_generatedWaveTable[i] +=powf((float)-1.0, (float)(g + 1)) *
					(1.0 / n) * sinf(F_2PI * i * n / (float)WAVETABLE_LENGTH);
		}
		max = fmax(max, m_generatedWaveTable[i]);
	}

	for (int i = 0; i < WAVETABLE_LENGTH; i++)
	{
		m_generatedWaveTable[i] /= max;
	}
}

void Oscillator::generateTriangleWaveTable(int bands)
{
	float max = 0;
	m_generatedWaveTable = new sample_t[WAVETABLE_LENGTH];
	for (int i = 0 ; i < WAVETABLE_LENGTH; i++)
	{
		m_generatedWaveTable[i] = 0.0;
		for (int g = 0; g <= bands * 0.5; g++)
		{
			double n = double(g);
			m_generatedWaveTable[i] += powf((float)-1.0, (float)n) *
					(1.0 / powf((float)(2 * n + 1),
							   (float)2.0)) *
					sinf(F_2PI * (2.0 * n + 1) * i / (float)WAVETABLE_LENGTH);
		}
		max = fmax(max, m_generatedWaveTable[i]);
	}

	for (int i = 0; i < WAVETABLE_LENGTH; i++)
	{
		m_generatedWaveTable[i] /= max;
	}
}

void Oscillator::generateSquareWaveTable(int bands)
{
	float max = 0;
	m_generatedWaveTable = new sample_t[WAVETABLE_LENGTH];
	for (int i = 0; i < WAVETABLE_LENGTH; i++)
	{
		m_generatedWaveTable[i] = 0.0;
		for (int g = 1; g <= bands; g += 2)
		{
			double n = double(g);
			m_generatedWaveTable[i] += (1.0 / n) * sinf(F_2PI * i * n / WAVETABLE_LENGTH);
		}
		max = fmax(max, m_generatedWaveTable[i]);
	}
	for (int i = 0; i < WAVETABLE_LENGTH; i++)
	{
		m_generatedWaveTable[i] /= max;
	}
}

int Oscillator::waveTableBandFromFreq(float freq)
{
	int i;
	for (i = 0; i < s_waveTablesPerWaveformCount; ++i)
	{
		if (s_waveTableBandFreqs[i] > freq) return i;
	}
	return i;
}

sample_t ***Oscillator::s_waveTables = 0;
float* Oscillator::s_waveTableBandFreqs = 0;
int Oscillator::s_waveTablesPerWaveformCount = 0;



void Oscillator::allocWaveTables()
{
	s_waveTables = new sample_t**[WaveShapes::NumWaveShapes];
	memset(s_waveTables, 0, sizeof(sample_t) * WaveShapes::NumWaveShapes);
	for (int i = 0; i < WaveShapes::NumWaveShapes; ++i)
	{
		s_waveTables[i] = new sample_t*[s_waveTablesPerWaveformCount +1];
		memset(s_waveTables[i], 0, sizeof(sample_t) * s_waveTablesPerWaveformCount);
	}
}

void Oscillator::generateWaveTables()
{
	//generate sine tables
	for (int i = 0; i < s_waveTablesPerWaveformCount; ++i)
	{
		generateSineWaveTable(MAX_FREQ / s_waveTableBandFreqs[i]);
		s_waveTables[WaveShapes::SineWave][i] = m_generatedWaveTable;
	}
	//generate saw tables
	for (int i = 0; i < s_waveTablesPerWaveformCount; ++i)
	{
		generateSawWaveTable(MAX_FREQ / s_waveTableBandFreqs[i]);
		s_waveTables[WaveShapes::SawWave][i] = m_generatedWaveTable;
	}
	//generate square tables
	for (int i = 0; i < s_waveTablesPerWaveformCount; ++i)
	{
		generateSquareWaveTable(MAX_FREQ / s_waveTableBandFreqs[i]);
		s_waveTables[WaveShapes::SquareWave][i] = m_generatedWaveTable;
	}
	//generate triangle tables
	for (int i = 0; i < s_waveTablesPerWaveformCount; ++i)
	{
		generateTriangleWaveTable(MAX_FREQ / s_waveTableBandFreqs[i]);
		s_waveTables[WaveShapes::TriangleWave][i] = m_generatedWaveTable;
	}
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
	return wtSample(WaveShapes::SineWave,_sample);
	return( sinSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::TriangleWave>(
		const float _sample )
{
	return wtSample(WaveShapes::TriangleWave,_sample);
	return( triangleSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::SawWave>(
		const float _sample )
{
	return wtSample(WaveShapes::SawWave, _sample);
	return( sawSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::SquareWave>(
		const float _sample )
{
	return wtSample(WaveShapes::SquareWave, _sample);
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




