/*
 * Oscillator.cpp - implementation of powerful oscillator-class
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *               2018      Dave French	<dave/dot/french3/at/googlemail/dot/com>
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
#include "fftw3.h"



void Oscillator::waveTableInit()
{
	createFFTPlans();
	generateWaveTables();
	// The oscillator FFT plans remain throughout the application lifecycle
	// due to being expensive to create, and being used whenever a userwave form is changed
	// deleted in main.cpp main()

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
	m_useWaveTable( false )
{
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

void Oscillator::generateSineWaveTable(sample_t * table)
{
	// sinewaves are the only continuous waves containing no harmonics
	// hence by definition all wavetables contain a single signal
	// https://en.wikipedia.org/wiki/Sine_wave
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		table[i] = sinf(((float)i / (float)OscillatorConstants::WAVETABLE_LENGTH) * F_2PI);
	}
}

void Oscillator::generateSawWaveTable(int bands, sample_t * table)
{
	// sawtooth wave contain both even and odd harmonics
	// hence sinewaves are added for all bands
	// https://en.wikipedia.org/wiki/Sawtooth_wave
	float max = 0;
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		table[i] = 0.0;
		for (float n = 1; n <= bands; n++)
		{
			table[i] += ((int)n%2 ? 1.0f : -1.0f) / n * sinf(F_2PI * i * n / (float)OscillatorConstants::WAVETABLE_LENGTH);
		}
		max = fmax(max, table[i]);
	}

	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		table[i] /= max;
	}
}


void Oscillator::generateTriangleWaveTable(int bands, sample_t * table)
{
	// triangle waves contain only odd harmonics
	// hence sinewaves are added for alternate bands
	// https://en.wikipedia.org/wiki/Triangle_wave
	float max = 0;
	for (int i = 0 ; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		table[i] = 0.0;
		for (float n = 0; n <= bands * 0.5; n++)
		{
			table[i] += ((int)n%2 ? -1.0f : +1.0f) / powf((float)(2 * n + 1),
							   2.0f) *
					sinf(F_2PI * (2.0f * n + 1) * i / (float)OscillatorConstants::WAVETABLE_LENGTH);
		}
		max = fmax(max, table[i]);
	}

	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		table[i] /= max;
	}
}

void Oscillator::generateSquareWaveTable(int bands, sample_t * table)
{
	// square waves only contain odd harmonics,
	// at diffrent levels when compared to triangle waves
	// https://en.wikipedia.org/wiki/Square_wave
	float max = 0;
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		table[i] = 0.0f;
		for (float n = 1; n <= bands; n += 2)
		{
			table[i] += (1.0f / n) * sinf(F_2PI * i * n / OscillatorConstants::WAVETABLE_LENGTH);
		}
		max = fmax(max, table[i]);
	}
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		table[i] /= max;
	}
}



//expects sample in sample buffer
void Oscillator::generateFromFFT(int bands, float threshold, sample_t * table)
{
	float max = 0;
	
	//set unrequired bands to zero 
	for (int i = bands; i < OscillatorConstants::WAVETABLE_LENGTH * 2 + 1 - bands; i++)
	{
		s_specBuf[i][0] = 0.0f;
		s_specBuf[i][1] = 0.0f;
	}
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH * 2 + 1; ++i)
	{
		if (s_specBuf[i][0] * s_specBuf[i][0] + s_specBuf[i][1] * s_specBuf[i][1] < threshold)
		{
			s_specBuf[i][0] = 0.0f;
			s_specBuf[i][1] = 0.0f;
		}
	}
	//ifft
	fftwf_execute(s_ifftPlan);
	//normalise
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; ++i)
	{
		max = fmax(max, s_sampleBuffer[i]);
	}
	//copy to generateWaveTable
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; ++i)
	{
		table[i] = s_sampleBuffer[i] / max;
	}
}

void Oscillator::generateAntiAliasUserWaveTable(SampleBuffer *sampleBuffer)
{
	delete sampleBuffer->m_userAntiAliasWaveTable;
	sampleBuffer->m_userAntiAliasWaveTable = new sample_t *[OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT];
	for (int i=0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		sampleBuffer->m_userAntiAliasWaveTable[i] = new sample_t[OscillatorConstants::WAVETABLE_LENGTH];
	}
	for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; ++i)
		{
			s_sampleBuffer[i] = sampleBuffer->userWaveSample((float)i / (float)OscillatorConstants::WAVETABLE_LENGTH);
		}
		fftwf_execute(s_fftPlan);
		Oscillator::generateFromFFT(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), 0.1f, sampleBuffer->m_userAntiAliasWaveTable[i]);
	}
}



sample_t Oscillator::s_waveTables[Oscillator::WaveShapes::NumWaveShapes-2][128 / OscillatorConstants::SEMITONES_PER_TABLE][OscillatorConstants::WAVETABLE_LENGTH];
fftwf_plan Oscillator::s_fftPlan;
fftwf_plan Oscillator::s_ifftPlan;
fftwf_complex * Oscillator::s_specBuf;
float Oscillator::s_fftBuffer[OscillatorConstants::WAVETABLE_LENGTH*2];
float Oscillator::s_sampleBuffer[OscillatorConstants::WAVETABLE_LENGTH];



void Oscillator::createFFTPlans()
{
	Oscillator::s_specBuf = ( fftwf_complex * ) fftwf_malloc( ( OscillatorConstants::WAVETABLE_LENGTH * 2 + 1 ) * sizeof( fftwf_complex ) );
	Oscillator::s_fftPlan = fftwf_plan_dft_r2c_1d(OscillatorConstants::WAVETABLE_LENGTH, s_sampleBuffer, s_specBuf, FFTW_MEASURE );
	Oscillator::s_ifftPlan = fftwf_plan_dft_c2r_1d(OscillatorConstants::WAVETABLE_LENGTH, s_specBuf, s_sampleBuffer, FFTW_MEASURE);
}

void Oscillator::destroyFFTPlans()
{
	fftwf_destroy_plan(s_fftPlan);
	fftwf_destroy_plan(s_ifftPlan);
	fftwf_free(s_specBuf);
}

void Oscillator::generateWaveTables()
{
	//generate sine tables
	for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		generateSineWaveTable(s_waveTables[WaveShapes::SineWave][i]);
	}
	//generate saw tables
	for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		generateSawWaveTable(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), s_waveTables[WaveShapes::SawWave][i]);
	}
	//generate square tables
	for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		generateSquareWaveTable(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), s_waveTables[WaveShapes::SquareWave][i]);
	}
	//generate triangle tables
	for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		generateTriangleWaveTable(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), s_waveTables[WaveShapes::TriangleWave][i]);
	}
	//generate moogSaw tables
	//generate signal buffer
	for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; ++i)
		{
			Oscillator::s_sampleBuffer[i] = moogSawSample((float)i / (float)OscillatorConstants::WAVETABLE_LENGTH);
		}
		fftwf_execute(s_fftPlan);
		generateFromFFT(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), 0.2f, s_waveTables[WaveShapes::MoogSawWave][i]);
	}

	//generate Exp tables
	//generate signal buffer
	for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; ++i)
		{
			s_sampleBuffer[i] = expSample((float)i / (float)OscillatorConstants::WAVETABLE_LENGTH);
		}
		fftwf_execute(s_fftPlan);
		generateFromFFT(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), 0.2f, s_waveTables[WaveShapes::ExponentialWave][i]);
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
	if (m_useWaveTable)
	{
		return wtSample(s_waveTables[WaveShapes::SineWave],_sample);
	}
	else
	{
		return sinSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::TriangleWave>(
		const float _sample )
{
	if (m_useWaveTable)
	{
		return wtSample(s_waveTables[WaveShapes::TriangleWave],_sample);
	}
	else
	{
		return triangleSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::SawWave>(
		const float _sample )
{
	if (m_useWaveTable)
	{
		return wtSample(s_waveTables[WaveShapes::SawWave], _sample);
	}
	else
	{
		return sawSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::SquareWave>(
		const float _sample )
{
	if (m_useWaveTable)
	{
		return wtSample(s_waveTables[WaveShapes::SquareWave], _sample);
	}
	else
	{
		return squareSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::MoogSawWave>(
							const float _sample )
{
	if (m_useWaveTable)
	{
		return wtSample(s_waveTables[WaveShapes::MoogSawWave], _sample);
	}
	else
	{
		return moogSawSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::ExponentialWave>(
							const float _sample )
{
	if (m_useWaveTable)
	{
		return wtSample(s_waveTables[WaveShapes::ExponentialWave], _sample);
	}
	else
	{
		return expSample(_sample);
	}
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
	if (m_useWaveTable)
	{
		return wtSample(m_userWave->m_userAntiAliasWaveTable, _sample);
	}
	else
	{
		return userWaveSample(_sample);
	}
}




