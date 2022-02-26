/*
 * Oscillator.h - declaration of class Oscillator
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <cassert>
#include <fftw3.h>

#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "Engine.h"
#include "lmms_constants.h"
#include "lmmsconfig.h"
#include "AudioEngine.h"
#include "OscillatorConstants.h"
#include "SampleBuffer.h"

class IntModel;


class LMMS_EXPORT Oscillator
{
	MM_OPERATORS
public:
	enum WaveShapes
	{
		SineWave,
		TriangleWave,
		SawWave,
		SquareWave,
		MoogSawWave,
		ExponentialWave,
		WhiteNoise,
		UserDefinedWave,
		NumWaveShapes,                                         //!< Number of all available wave shapes
		FirstWaveShapeTable = TriangleWave,                    //!< First wave shape that has a pre-generated table
		NumWaveShapeTables = WhiteNoise - FirstWaveShapeTable, //!< Number of band-limited wave shapes to be generated
	};

	enum ModulationAlgos
	{
		PhaseModulation,
		AmplitudeModulation,
		SignalMix,
		SynchronizedBySubOsc,
		FrequencyModulation,
		NumModulationAlgos
	} ;


	Oscillator( const IntModel *wave_shape_model,
			const IntModel *mod_algo_model,
			const float &freq,
			const float &detuning_div_samplerate,
			const float &phase_offset,
			const float &volume,
			Oscillator *m_subOsc = nullptr);
	virtual ~Oscillator()
	{
		delete m_subOsc;
	}

	static void waveTableInit();
	static void destroyFFTPlans();
	static void generateAntiAliasUserWaveTable(SampleBuffer* sampleBuffer);

	inline void setUseWaveTable(bool n)
	{
		m_useWaveTable = n;
	}

	inline void setUserWave( const SampleBuffer * _wave )
	{
		m_userWave = _wave;
	}

	void update(sampleFrame* ab, const fpp_t frames, const ch_cnt_t chnl, bool modulator = false);

	// now follow the wave-shape-routines...
	static inline sample_t sinSample( const float _sample )
	{
		return sinf( _sample * F_2PI );
	}

	static inline sample_t triangleSample( const float _sample )
	{
		const float ph = absFraction( _sample );
		if( ph <= 0.25f )
		{
			return ph * 4.0f;
		}
		else if( ph <= 0.75f )
		{
			return 2.0f - ph * 4.0f;
		}
		return ph * 4.0f - 4.0f;
	}

	static inline sample_t sawSample( const float _sample )
	{
		return -1.0f + absFraction( _sample ) * 2.0f;
	}

	static inline sample_t squareSample( const float _sample )
	{
		return ( absFraction( _sample ) > 0.5f ) ? -1.0f : 1.0f;
	}

	static inline sample_t moogSawSample( const float _sample )
	{
		const float ph = absFraction( _sample );
		if( ph < 0.5f )
		{
			return -1.0f + ph * 4.0f;
		}
		return 1.0f - 2.0f * ph;
	}

	static inline sample_t expSample( const float _sample )
	{
		float ph = absFraction( _sample );
		if( ph > 0.5f )
		{
			ph = 1.0f - ph;
		}
		return -1.0f + 8.0f * ph * ph;
	}

	static inline sample_t noiseSample( const float )
	{
		// Precise implementation
//		return 1.0f - rand() * 2.0f / RAND_MAX;

		// Fast implementation
		return 1.0f - fast_rand() * 2.0f / FAST_RAND_MAX;
	}

	inline sample_t userWaveSample( const float _sample ) const
	{
		return m_userWave->userWaveSample( _sample );
	}

	struct wtSampleControl {
		float frame;
		f_cnt_t f1;
		f_cnt_t f2;
		int band;
	};

	inline wtSampleControl getWtSampleControl(const float sample) const
	{
		wtSampleControl control;
		control.frame = sample * OscillatorConstants::WAVETABLE_LENGTH;
		control.f1 = static_cast<f_cnt_t>(control.frame) % OscillatorConstants::WAVETABLE_LENGTH;
		if (control.f1 < 0)
		{
			control.f1 += OscillatorConstants::WAVETABLE_LENGTH;
		}
		control.f2 = control.f1 < OscillatorConstants::WAVETABLE_LENGTH - 1 ?
					control.f1 + 1 :
					0;
		control.band = waveTableBandFromFreq(
			m_freq * m_detuning_div_samplerate * Engine::audioEngine()->processingSampleRate());
		return control;
	}

	inline sample_t wtSample(
		const sample_t table[OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT][OscillatorConstants::WAVETABLE_LENGTH],
		const float sample) const
	{
		assert(table != nullptr);
		wtSampleControl control = getWtSampleControl(sample);
		return linearInterpolate(table[control.band][control.f1],
				table[control.band][control.f2], fraction(control.frame));
	}

	inline sample_t wtSample(const std::unique_ptr<OscillatorConstants::waveform_t>& table, const float sample) const
	{
		assert(table != nullptr);
		wtSampleControl control = getWtSampleControl(sample);
		return linearInterpolate((*table)[control.band][control.f1],
				(*table)[control.band][control.f2], fraction(control.frame));
	}

	inline sample_t wtSample(sample_t **table, const float sample) const
	{
		assert(table != nullptr);
		wtSampleControl control = getWtSampleControl(sample);
		return linearInterpolate(table[control.band][control.f1],
				table[control.band][control.f2], fraction(control.frame));
	}

	static inline int waveTableBandFromFreq(float freq)
	{
		// Frequency bands are indexed relative to default MIDI key frequencies.
		// I.e., 440 Hz (A4, key 69): 69 + 12 * log2(1) = 69
		// To always avoid aliasing, ceil() is used instead of round(). It ensures that the nearest wavetable with
		// lower than optimal number of harmonics is used when exactly matching wavetable is not available.
		int band = (69 + static_cast<int>(std::ceil(12.0f * std::log2(freq / 440.0f)))) / OscillatorConstants::SEMITONES_PER_TABLE;
		// Limit the returned value to a valid wavetable index range.
		// (qBound would bring Qt into the audio code, which not a preferable option due to realtime safety.
		// C++17 std::clamp() could be used in the future.)
		return band <= 1 ? 1 : band >= OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT-1 ? OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT-1 : band;
	}

	static inline float freqFromWaveTableBand(int band)
	{
		return 440.0f * std::pow(2.0f, (band * OscillatorConstants::SEMITONES_PER_TABLE - 69.0f) / 12.0f);
	}

private:
	const IntModel * m_waveShapeModel;
	const IntModel * m_modulationAlgoModel;
	const float & m_freq;
	const float & m_detuning_div_samplerate;
	const float & m_volume;
	const float & m_ext_phaseOffset;
	Oscillator * m_subOsc;
	float m_phaseOffset;
	float m_phase;
	const SampleBuffer * m_userWave;
	bool m_useWaveTable;
	// There are many update*() variants; the modulator flag is stored as a member variable to avoid
	// adding more explicit parameters to all of them. Can be converted to a parameter if needed.
	bool m_isModulator;

	/* Multiband WaveTable */
	static sample_t s_waveTables[WaveShapes::NumWaveShapeTables][OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT][OscillatorConstants::WAVETABLE_LENGTH];
	static fftwf_plan s_fftPlan;
	static fftwf_plan s_ifftPlan;
	static fftwf_complex * s_specBuf;
	static float s_sampleBuffer[OscillatorConstants::WAVETABLE_LENGTH];

	static void generateSawWaveTable(int bands, sample_t* table, int firstBand = 1);
	static void generateTriangleWaveTable(int bands, sample_t* table, int firstBand = 1);
	static void generateSquareWaveTable(int bands, sample_t* table, int firstBand = 1);
	static void generateFromFFT(int bands, sample_t* table);
	static void generateWaveTables();
	static void createFFTPlans();

	/* End Multiband wavetable */


	void updateNoSub( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updatePM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updateAM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updateMix( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updateSync( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updateFM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );

	float syncInit( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	inline bool syncOk( float _osc_coeff );

	template<WaveShapes W>
	void updateNoSub( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShapes W>
	void updatePM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShapes W>
	void updateAM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShapes W>
	void updateMix( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShapes W>
	void updateSync( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShapes W>
	void updateFM( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );

	template<WaveShapes W>
	inline sample_t getSample( const float _sample );

	inline void recalcPhase();

} ;


#endif
