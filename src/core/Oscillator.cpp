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
#include <numbers>

#include "Engine.h"
#include "AudioEngine.h"
#include "AutomatableModel.h"
#include "fftw3.h"
#include "fft_helpers.h"


namespace lmms
{


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




void Oscillator::update(SampleFrame* ab, const fpp_t frames, const ch_cnt_t chnl, bool modulator)
{
	if (m_freq >= Engine::audioEngine()->outputSampleRate() / 2)
	{
		zeroSampleFrames(ab, frames);
		return;
	}
	// If this oscillator is used to PM or PF modulate another oscillator, take a note.
	// The sampling functions will check this variable and avoid using band-limited
	// wavetables, since they contain ringing that would lead to unexpected results.
	m_isModulator = modulator;
	if (m_subOsc != nullptr)
	{
		switch (static_cast<ModulationAlgo>(m_modulationAlgoModel->value()))
		{
			case ModulationAlgo::PhaseModulation:
				updatePM(ab, frames, chnl);
				break;
			case ModulationAlgo::AmplitudeModulation:
				updateAM(ab, frames, chnl);
				break;
			case ModulationAlgo::SignalMix:
			default:
				updateMix(ab, frames, chnl);
				break;
			case ModulationAlgo::SynchronizedBySubOsc:
				updateSync(ab, frames, chnl);
				break;
			case ModulationAlgo::FrequencyModulation:
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
	using namespace std::numbers;
	// sawtooth wave contain both even and odd harmonics
	// hence sinewaves are added for all bands
	// https://en.wikipedia.org/wiki/Sawtooth_wave
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		// add offset to the position index to match phase of the non-wavetable saw wave; precompute "/ period"
		const float imod = (i - OscillatorConstants::WAVETABLE_LENGTH / 2.f) / OscillatorConstants::WAVETABLE_LENGTH;
		for (int n = firstBand; n <= bands; n++)
		{
			table[i] += (n % 2 ? 1.0f : -1.0f) / n * std::sin(2 * pi_v<float> * n * imod) / (pi_v<float> * 0.5f);
		}
	}
}


void Oscillator::generateTriangleWaveTable(int bands, sample_t* table, int firstBand)
{
	using namespace std::numbers;
	constexpr float pi_sqr = pi_v<float> * pi_v<float>;
	// triangle waves contain only odd harmonics
	// hence sinewaves are added for alternate bands
	// https://en.wikipedia.org/wiki/Triangle_wave
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		for (int n = firstBand | 1; n <= bands; n += 2)
		{
			table[i] += (n & 2 ? -1.0f : 1.0f) / (n * n)
				* std::sin(2 * pi_v<float> * n * i / (float)OscillatorConstants::WAVETABLE_LENGTH) / (pi_sqr / 8.f);
		}
	}
}


void Oscillator::generateSquareWaveTable(int bands, sample_t* table, int firstBand)
{
	using namespace std::numbers;
	// square waves only contain odd harmonics,
	// at diffrent levels when compared to triangle waves
	// https://en.wikipedia.org/wiki/Square_wave
	for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; i++)
	{
		for (int n = firstBand | 1; n <= bands; n += 2)
		{
			table[i] += (1.0f / n)
				* std::sin(2 * pi_v<float> * i * n / OscillatorConstants::WAVETABLE_LENGTH)
				/ (pi_v<float> / 4.f);
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
	normalize(s_sampleBuffer.data(), table, OscillatorConstants::WAVETABLE_LENGTH, 2*OscillatorConstants::WAVETABLE_LENGTH + 1);
}

std::unique_ptr<OscillatorConstants::waveform_t> Oscillator::generateAntiAliasUserWaveTable(const SampleBuffer* sampleBuffer)
{
	auto userAntiAliasWaveTable = std::make_unique<OscillatorConstants::waveform_t>();
	for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
	{
		// TODO: This loop seems to be doing the same thing for each iteration of the outer loop,
		// and could probably be moved out of it
		for (int j = 0; j < OscillatorConstants::WAVETABLE_LENGTH; ++j)
		{
			s_sampleBuffer[j] = Oscillator::userWaveSample(
				sampleBuffer, static_cast<float>(j) / OscillatorConstants::WAVETABLE_LENGTH);
		}
		fftwf_execute(s_fftPlan);
		Oscillator::generateFromFFT(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), (*userAntiAliasWaveTable)[i].data());
	}

	return userAntiAliasWaveTable;
}



sample_t Oscillator::s_waveTables
	[Oscillator::NumWaveShapeTables]
	[OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT]
	[OscillatorConstants::WAVETABLE_LENGTH];
fftwf_plan Oscillator::s_fftPlan;
fftwf_plan Oscillator::s_ifftPlan;
fftwf_complex * Oscillator::s_specBuf;
std::array<float, OscillatorConstants::WAVETABLE_LENGTH> Oscillator::s_sampleBuffer;



void Oscillator::createFFTPlans()
{
	Oscillator::s_specBuf = ( fftwf_complex * ) fftwf_malloc( ( OscillatorConstants::WAVETABLE_LENGTH * 2 + 1 ) * sizeof( fftwf_complex ) );
	Oscillator::s_fftPlan = fftwf_plan_dft_r2c_1d(OscillatorConstants::WAVETABLE_LENGTH, s_sampleBuffer.data(), s_specBuf, FFTW_MEASURE );
	Oscillator::s_ifftPlan = fftwf_plan_dft_c2r_1d(OscillatorConstants::WAVETABLE_LENGTH, s_specBuf, s_sampleBuffer.data(), FFTW_MEASURE);
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
	using generator_t = void (*)(int, sample_t*, int);
	auto simpleGen = [](WaveShape shape, generator_t generator)
	{
		const int shapeID = static_cast<std::size_t>(shape) - FirstWaveShapeTable;
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
			generateFromFFT(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), s_waveTables[static_cast<std::size_t>(WaveShape::MoogSaw) - FirstWaveShapeTable][i]);
		}

		// Generate exponential tables
		for (int i = 0; i < OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT; ++i)
		{
			for (int i = 0; i < OscillatorConstants::WAVETABLE_LENGTH; ++i)
			{
				s_sampleBuffer[i] = expSample((float)i / (float)OscillatorConstants::WAVETABLE_LENGTH);
			}
			fftwf_execute(s_fftPlan);
			generateFromFFT(OscillatorConstants::MAX_FREQ / freqFromWaveTableBand(i), s_waveTables[static_cast<std::size_t>(WaveShape::Exponential) - FirstWaveShapeTable][i]);
		}
	};

// TODO: Mingw compilers currently do not support std::thread. There are some 3rd-party workarounds available,
// but since threading is not essential in this case, it is easier and more reliable to simply generate
// the wavetables serially. Remove the the check and #else branch once std::thread is well supported.
#if !defined(__MINGW32__) && !defined(__MINGW64__)
	std::thread sawThread(simpleGen, WaveShape::Saw, generateSawWaveTable);
	std::thread squareThread(simpleGen, WaveShape::Square, generateSquareWaveTable);
	std::thread triangleThread(simpleGen, WaveShape::Triangle, generateTriangleWaveTable);
	std::thread fftThread(fftGen);
	sawThread.join();
	squareThread.join();
	triangleThread.join();
	fftThread.join();
#else
	simpleGen(WaveShape::Saw, generateSawWaveTable);
	simpleGen(WaveShape::Square, generateSquareWaveTable);
	simpleGen(WaveShape::Triangle, generateTriangleWaveTable);
	fftGen();
#endif
}




void Oscillator::updateNoSub( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( static_cast<WaveShape>(m_waveShapeModel->value()) )
	{
		case WaveShape::Sine:
		default:
			updateNoSub<WaveShape::Sine>( _ab, _frames, _chnl );
			break;
		case WaveShape::Triangle:
			updateNoSub<WaveShape::Triangle>( _ab, _frames, _chnl );
			break;
		case WaveShape::Saw:
			updateNoSub<WaveShape::Saw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Square:
			updateNoSub<WaveShape::Square>( _ab, _frames, _chnl );
			break;
		case WaveShape::MoogSaw:
			updateNoSub<WaveShape::MoogSaw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Exponential:
			updateNoSub<WaveShape::Exponential>( _ab, _frames, _chnl );
			break;
		case WaveShape::WhiteNoise:
			updateNoSub<WaveShape::WhiteNoise>( _ab, _frames, _chnl );
			break;
		case WaveShape::UserDefined:
			updateNoSub<WaveShape::UserDefined>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updatePM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( static_cast<WaveShape>(m_waveShapeModel->value()) )
	{
		case WaveShape::Sine:
		default:
			updatePM<WaveShape::Sine>( _ab, _frames, _chnl );
			break;
		case WaveShape::Triangle:
			updatePM<WaveShape::Triangle>( _ab, _frames, _chnl );
			break;
		case WaveShape::Saw:
			updatePM<WaveShape::Saw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Square:
			updatePM<WaveShape::Square>( _ab, _frames, _chnl );
			break;
		case WaveShape::MoogSaw:
			updatePM<WaveShape::MoogSaw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Exponential:
			updatePM<WaveShape::Exponential>( _ab, _frames, _chnl );
			break;
		case WaveShape::WhiteNoise:
			updatePM<WaveShape::WhiteNoise>( _ab, _frames, _chnl );
			break;
		case WaveShape::UserDefined:
			updatePM<WaveShape::UserDefined>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updateAM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( static_cast<WaveShape>(m_waveShapeModel->value()) )
	{
		case WaveShape::Sine:
		default:
			updateAM<WaveShape::Sine>( _ab, _frames, _chnl );
			break;
		case WaveShape::Triangle:
			updateAM<WaveShape::Triangle>( _ab, _frames, _chnl );
			break;
		case WaveShape::Saw:
			updateAM<WaveShape::Saw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Square:
			updateAM<WaveShape::Square>( _ab, _frames, _chnl );
			break;
		case WaveShape::MoogSaw:
			updateAM<WaveShape::MoogSaw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Exponential:
			updateAM<WaveShape::Exponential>( _ab, _frames, _chnl );
			break;
		case WaveShape::WhiteNoise:
			updateAM<WaveShape::WhiteNoise>( _ab, _frames, _chnl );
			break;
		case WaveShape::UserDefined:
			updateAM<WaveShape::UserDefined>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updateMix( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( static_cast<WaveShape>(m_waveShapeModel->value()) )
	{
		case WaveShape::Sine:
		default:
			updateMix<WaveShape::Sine>( _ab, _frames, _chnl );
			break;
		case WaveShape::Triangle:
			updateMix<WaveShape::Triangle>( _ab, _frames, _chnl );
			break;
		case WaveShape::Saw:
			updateMix<WaveShape::Saw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Square:
			updateMix<WaveShape::Square>( _ab, _frames, _chnl );
			break;
		case WaveShape::MoogSaw:
			updateMix<WaveShape::MoogSaw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Exponential:
			updateMix<WaveShape::Exponential>( _ab, _frames, _chnl );
			break;
		case WaveShape::WhiteNoise:
			updateMix<WaveShape::WhiteNoise>( _ab, _frames, _chnl );
			break;
		case WaveShape::UserDefined:
			updateMix<WaveShape::UserDefined>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updateSync( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( static_cast<WaveShape>(m_waveShapeModel->value()) )
	{
		case WaveShape::Sine:
		default:
			updateSync<WaveShape::Sine>( _ab, _frames, _chnl );
			break;
		case WaveShape::Triangle:
			updateSync<WaveShape::Triangle>( _ab, _frames, _chnl );
			break;
		case WaveShape::Saw:
			updateSync<WaveShape::Saw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Square:
			updateSync<WaveShape::Square>( _ab, _frames, _chnl );
			break;
		case WaveShape::MoogSaw:
			updateSync<WaveShape::MoogSaw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Exponential:
			updateSync<WaveShape::Exponential>( _ab, _frames, _chnl );
			break;
		case WaveShape::WhiteNoise:
			updateSync<WaveShape::WhiteNoise>( _ab, _frames, _chnl );
			break;
		case WaveShape::UserDefined:
			updateSync<WaveShape::UserDefined>( _ab, _frames, _chnl );
			break;
	}
}




void Oscillator::updateFM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	switch( static_cast<WaveShape>(m_waveShapeModel->value()) )
	{
		case WaveShape::Sine:
		default:
			updateFM<WaveShape::Sine>( _ab, _frames, _chnl );
			break;
		case WaveShape::Triangle:
			updateFM<WaveShape::Triangle>( _ab, _frames, _chnl );
			break;
		case WaveShape::Saw:
			updateFM<WaveShape::Saw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Square:
			updateFM<WaveShape::Square>( _ab, _frames, _chnl );
			break;
		case WaveShape::MoogSaw:
			updateFM<WaveShape::MoogSaw>( _ab, _frames, _chnl );
			break;
		case WaveShape::Exponential:
			updateFM<WaveShape::Exponential>( _ab, _frames, _chnl );
			break;
		case WaveShape::WhiteNoise:
			updateFM<WaveShape::WhiteNoise>( _ab, _frames, _chnl );
			break;
		case WaveShape::UserDefined:
			updateFM<WaveShape::UserDefined>( _ab, _frames, _chnl );
			break;
	}
}




// should be called every time phase-offset is changed...
inline void Oscillator::recalcPhase()
{
	if (!approximatelyEqual(m_phaseOffset, m_ext_phaseOffset))
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




float Oscillator::syncInit( SampleFrame* _ab, const fpp_t _frames,
						const ch_cnt_t _chnl )
{
	if( m_subOsc != nullptr )
	{
		m_subOsc->update( _ab, _frames, _chnl );
	}
	recalcPhase();
	return m_freq * m_detuning_div_samplerate;
}




// if we have no sub-osc, we can't do any modulation... just get our samples
template<Oscillator::WaveShape W>
void Oscillator::updateNoSub( SampleFrame* _ab, const fpp_t _frames,
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
template<Oscillator::WaveShape W>
void Oscillator::updatePM( SampleFrame* _ab, const fpp_t _frames,
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
template<Oscillator::WaveShape W>
void Oscillator::updateAM( SampleFrame* _ab, const fpp_t _frames,
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
template<Oscillator::WaveShape W>
void Oscillator::updateMix( SampleFrame* _ab, const fpp_t _frames,
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
template<Oscillator::WaveShape W>
void Oscillator::updateSync( SampleFrame* _ab, const fpp_t _frames,
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
template<Oscillator::WaveShape W>
void Oscillator::updateFM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl )
{
	m_subOsc->update( _ab, _frames, _chnl, true );
	recalcPhase();
	const float osc_coeff = m_freq * m_detuning_div_samplerate;
	const float sampleRateCorrection = 44100.0f / Engine::audioEngine()->outputSampleRate();

	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		m_phase += _ab[frame][_chnl] * sampleRateCorrection;
		_ab[frame][_chnl] = getSample<W>( m_phase ) * m_volume;
		m_phase += osc_coeff;
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::WaveShape::Sine>(const float sample)
{
	const float current_freq = m_freq * m_detuning_div_samplerate * Engine::audioEngine()->outputSampleRate();

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
inline sample_t Oscillator::getSample<Oscillator::WaveShape::Triangle>(
		const float _sample )
{
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[static_cast<std::size_t>(WaveShape::Triangle) - FirstWaveShapeTable],_sample);
	}
	else
	{
		return triangleSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::WaveShape::Saw>(
		const float _sample )
{
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[static_cast<std::size_t>(WaveShape::Saw) - FirstWaveShapeTable], _sample);
	}
	else
	{
		return sawSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::WaveShape::Square>(
		const float _sample )
{
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[static_cast<std::size_t>(WaveShape::Square) - FirstWaveShapeTable], _sample);
	}
	else
	{
		return squareSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::WaveShape::MoogSaw>(
							const float _sample )
{
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[static_cast<std::size_t>(WaveShape::MoogSaw) - FirstWaveShapeTable], _sample);
	}
	else
	{
		return moogSawSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::WaveShape::Exponential>(
							const float _sample )
{
	if (m_useWaveTable && !m_isModulator)
	{
		return wtSample(s_waveTables[static_cast<std::size_t>(WaveShape::Exponential) - FirstWaveShapeTable], _sample);
	}
	else
	{
		return expSample(_sample);
	}
}




template<>
inline sample_t Oscillator::getSample<Oscillator::WaveShape::WhiteNoise>(
							const float _sample )
{
	return( noiseSample( _sample ) );
}




template<>
inline sample_t Oscillator::getSample<Oscillator::WaveShape::UserDefined>(
							const float _sample )
{
	if (m_useWaveTable && m_userAntiAliasWaveTable && !m_isModulator)
	{
		return wtSample(m_userAntiAliasWaveTable.get(), _sample);
	}
	else
	{
		return userWaveSample(m_userWave.get(), _sample);
	}
}


} // namespace lmms
