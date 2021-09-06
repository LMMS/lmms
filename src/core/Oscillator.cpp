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

#include <algorithm>
#if !defined(__MINGW32__) && !defined(__MINGW64__)
	#include <thread>
#endif

#include "BufferManager.h"
#include "Engine.h"
#include "Mixer.h"
#include "AutomatableModel.h"
#include "fftw3.h"
#include "fft_helpers.h"



void Oscillator::waveTableInit()
{
	createFFTPlans();
	generateWaveTables();
	// The oscillator FFT plans remain throughout the application lifecycle
	// due to being expensive to create, and being used whenever a userwave form is changed
	// deleted in main.cpp main()

}

Oscillator::Oscillator(const IntModel *wave_shape_model,
			const IntModel *mod_algo_model,
			const float &freq,
			const float &detuning_div_samplerate,
			const float &phase_offset,
			const float &volume,
			Oscillator *sub_osc) :
	m_waveShapeModel(wave_shape_model),
	m_modulationAlgoModel(mod_algo_model),
	m_freq(freq),
	m_detuning_div_samplerate(detuning_div_samplerate),
	m_volume(volume),
	m_ext_phaseOffset(phase_offset),
	m_subOsc(sub_osc),
	m_phaseOffset(phase_offset),
	m_phase(phase_offset),
	m_userWave(nullptr),
	m_useWaveTable(false),
	m_isModulator(false)
{
}




void Oscillator::update(sampleFrame* ab, const fpp_t frames, const ch_cnt_t chnl, bool modulator)
{
	if (m_freq >= Engine::mixer()->processingSampleRate() / 2)
	{
		BufferManager::clear(ab, frames);
		return;
	}
	// If this oscillator is used to PM or PF modulate another oscillator, take a note.
	// The sampling functions will check this variable and avoid using band-limited
	// wavetables, since they contain ringing that would lead to unexpected results.
	m_isModulator = modulator;
	if (m_subOsc != nullptr)
	{
		switch (m_modulationAlgoModel->value())
		{
			case PhaseModulation:
				updatePM(ab, frames, chnl);
				break;
			case AmplitudeModulation:
				updateAM(ab, frames, chnl);
				break;
			case SignalMix:
				updateMix(ab, frames, chnl);
				break;
			case SynchronizedBySubOsc:
				updateSync(ab, frames, chnl);
				break;
			case FrequencyModulation:
				updateFM(ab, frames, chnl);
		}
	}
	else
	{
		updateNoSub(ab, frames, chnl);
	}
}


void Oscillator::generateSawWaveTable(int bands, sample_t* table, int firstBand)
{
	// sawtooth wave contain both even and odd harmonics
	// hence sinewaves are added for all bands
	// https://en.wikipedia.org/wiki/Sawtooth_wave
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		// add offset to the position index to match phase of the non-wavetable saw wave; precompute "/ period"
		const float imod = (i - OscillatorConstants::WAVETABLE_LENGTH / 2.f) / OscillatorConstants::WAVETABLE_LENGTH;
		for (int n = firstBand; n <= bands; n++)
		{
			table[i] += (n % 2 ? 1.0f : -1.0f) / n * sinf(F_2PI * n * imod) / F_PI_2;
		}
	}
}


void Oscillator::generateTriangleWaveTable(int bands, sample_t* table, int firstBand)
{
	// triangle waves contain only odd harmonics
	// hence sinewaves are added for alternate bands
	// https://en.wikipedia.org/wiki/Triangle_wave
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		for (int n = firstBand | 1; n <= bands; n += 2)
		{
			table[i] += (n & 2 ? -1.0f : 1.0f) / powf(n, 2.0f) *
				sinf(F_2PI * n * i / (float)OscillatorConstants::WAVETABLE_LENGTH) / (F_PI_SQR / 8);
		}
	}
}


void Oscillator::generateSquareWaveTable(int bands, sample_t* table, int firstBand)
{
	// square waves only contain odd harmonics,
	// at diffrent levels when compared to triangle waves
	// https://en.wikipedia.org/wiki/Square_wave
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		for (int n = firstBand | 1; n <= bands; n += 2)
		{
			table[i] += (1.0f / n) * sinf(F_2PI * i * n / OscillatorConstants::WAVETABLE_LENGTH) / (F_PI / 4);
		}
	}
}



// Expects waveform converted to frequency domain to be present in the spectrum buffer
void Oscillator::generateFromFFT(int bands, sample_t* table)
{
	// Keep only specified number of bands, set the rest to zero.
	// Add a +1 offset to the requested number of bands, since the first "useful" frequency falls into bin 1.
	// I.e., for bands = 1, keeping just bin 0 (center 0 Hz, +- 4 Hz) makes no sense, it would not produce any tone.
	for (int i = bands + 1; i < OscillatorConstants::WAVETABLE_LENGTH * 2 - bands; i++)
	{
		s_specBuf[i][0] = 0.0f;
		s_specBuf[i][1] = 0.0f;
	}
	//ifft
	fftwf_execute(s_ifftPlan);
	//normalize and copy to result buffer
	normalize(s_sampleBuffer, table, OscillatorConstants::WAVETABLE_LENGTH, 2*OscillatorConstants::WAVETABLE_LENGTH + 1);
}

void Oscillator::generateAntiAliasUserWaveTable(SampleBuffer *sampleBuffer)
{
	if (sampleBuffer->m_userAntiAliasWaveTable == NULL) {return;}

	for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; ++i)
		{
			s_sampleBuffer[i] = sampleBuffer->userWaveSample((float)i / (float)OscillatorConstants::WAVETABLE_LENGTH);
		}
		fftwf_execute(s_fftPlan);
		Oscillator::generateFromFFT(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), (*(sampleBuffer->m_userAntiAliasWaveTable))[i].data());
	}
}



sample_t Oscillator::s_waveTables
	[Oscillator::WaveShapes::NumWaveShapeTables]
	[OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT]
	[OscillatorConstants::WAVETABLE_LENGTH];
fftwf_plan Oscillator::s_fftPlan;
fftwf_plan Oscillator::s_ifftPlan;
fftwf_complex * Oscillator::s_specBuf;
float Oscillator::s_sampleBuffer[OscillatorConstants::WAVETABLE_LENGTH];



void Oscillator::createFFTPlans()
{
	Oscillator::s_specBuf = ( fftwf_complex * ) fftwf_malloc( ( OscillatorConstants::WAVETABLE_LENGTH * 2 + 1 ) * sizeof( fftwf_complex ) );
	Oscillator::s_fftPlan = fftwf_plan_dft_r2c_1d(OscillatorConstants::WAVETABLE_LENGTH, s_sampleBuffer, s_specBuf, FFTW_MEASURE );
	Oscillator::s_ifftPlan = fftwf_plan_dft_c2r_1d(OscillatorConstants::WAVETABLE_LENGTH, s_specBuf, s_sampleBuffer, FFTW_MEASURE);
	// initialize s_specBuf content to zero, since the values are used in a condition inside generateFromFFT()
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH * 2 + 1; i++)
	{
		s_specBuf[i][0] = 0.0f;
		s_specBuf[i][1] = 0.0f;
	}
}

void Oscillator::destroyFFTPlans()
{
	fftwf_destroy_plan(s_fftPlan);
	fftwf_destroy_plan(s_ifftPlan);
	fftwf_free(s_specBuf);
}

void Oscillator::generateWaveTables()
{
	// Generate tables for simple shaped (constructed by summing sine waves).
	// Start from the table that contains the least number of bands, and re-use each table in the following
	// iteration, adding more bands in each step and avoiding repeated computation of earlier bands.
	typedef void (*generator_t)(int, sample_t*, int);
	auto simpleGen = [](WaveShapes shape, generator_t generator)
	{
		const int shapeID = shape - FirstWaveShapeTable;
		int lastBands = 0;

		// Clear the first wave table
		std::fill(
		    std::begin(s_waveTables[shapeID][OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT - 1]),
		    std::end(s_waveTables[shapeID][OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT - 1]),
		    0.f);

		for (int i = OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT - 1; i >= 0; i--)
		{
			const int bands = OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i);
			generator(bands, s_waveTables[shapeID][i], lastBands + 1);
			lastBands = bands;
			if (i)
			{
				std::copy(
					s_waveTables[shapeID][i],
					s_waveTables[shapeID][i] + OscillatorConstants::WAVETABLE_LENGTH,
					s_waveTables[shapeID][i - 1]);
			}
		}
	};

	// FFT-based wave shapes: make standard wave table without band limit, convert to frequency domain, remove bands
	// above maximum frequency and convert back to time domain.
	auto fftGen = []()
	{
		// Generate moogSaw tables
		for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
		{
			for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; ++i)
			{
				Oscillator::s_sampleBuffer[i] = moogSawSample((float)i / (float)OscillatorConstants::WAVETABLE_LENGTH);
			}
			fftwf_execute(s_fftPlan);
			generateFromFFT(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), s_waveTables[WaveShapes::MoogSawWave - FirstWaveShapeTable][i]);
		}

		// Generate exponential tables
		for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
		{
			for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; ++i)
			{
				s_sampleBuffer[i] = expSample((float)i / (float)OscillatorConstants::WAVETABLE_LENGTH);
			}
			fftwf_execute(s_fftPlan);
			generateFromFFT(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), s_waveTables[WaveShapes::ExponentialWave - FirstWaveShapeTable][i]);
		}
	};

// TODO: Mingw compilers currently do not support std::thread. There are some 3rd-party workarounds available,
// but since threading is not essential in this case, it is easier and more reliable to simply generate
// the wavetables serially. Remove the the check and #else branch once std::thread is well supported.
#if !defined(__MINGW32__) && !defined(__MINGW64__)
	std::thread sawThread(simpleGen, WaveShapes::SawWave, generateSawWaveTable);
	std::thread squareThread(simpleGen, WaveShapes::SquareWave, generateSquareWaveTable);
	std::thread triangleThread(simpleGen, WaveShapes::TriangleWave, generateTriangleWaveTable);
	std::thread fftThread(fftGen);
	sawThread.join();
	squareThread.join();
	triangleThread.join();
	fftThread.join();
#else
	simpleGen(WaveShapes::SawWave, generateSawWaveTable);
	simpleGen(WaveShapes::SquareWave, generateSquareWaveTable);
	simpleGen(WaveShapes::TriangleWave, generateTriangleWaveTable);
	fftGen();
#endif
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
	m_phase = absFraction( m_phase );
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
	return m_freq * m_detuning_div_samplerate;
}




// if we have no sub-osc, we can't do any modulation... just get our samples
template<Oscillator::WaveShapes W>
void Oscillator::updateNoSub( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

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
	m_subOsc->update( _ab, _frames, _chnl, true );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

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
	m_subOsc->update( _ab, _frames, _chnl, false );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

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
	m_subOsc->update( _ab, _frames, _chnl, false );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

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
	const float osc_coeff = m_freq * m_detuning_div_samplerate;

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
	m_subOsc->update( _ab, _frames, _chnl, true );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;
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
inline sample_t Oscillator::getSample<Oscillator::SineWave>(const float sample)
{
	const float current_freq = m_freq * m_detuning_div_samplerate * Engine::mixer()->processingSampleRate();

	if (!m_useWaveTable || current_freq < OscillatorConstants::MAX_FREQ)
	{
		return sinSample(sample);
	}
	else
	{
		return 0;
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::TriangleWave>(
		const float _sample )
{
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[WaveShapes::TriangleWave - FirstWaveShapeTable],_sample);
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
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[WaveShapes::SawWave - FirstWaveShapeTable], _sample);
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
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[WaveShapes::SquareWave - FirstWaveShapeTable], _sample);
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
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[WaveShapes::MoogSawWave - FirstWaveShapeTable], _sample);
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
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[WaveShapes::ExponentialWave - FirstWaveShapeTable], _sample);
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
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(m_userWave->m_userAntiAliasWaveTable, _sample);
	}
	else
	{
		return userWaveSample(_sample);
	}
}



