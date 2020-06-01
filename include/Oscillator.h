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

#include "lmmsconfig.h"

#include <math.h>

#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <fftw3.h>
#include "OscillatorConstants.h"
#include "SampleBuffer.h"
#include "lmms_constants.h"

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
		UserDefinedWave, //to remain penultimate
		NumWaveShapes
	} ;

	enum ModulationAlgos
	{
		PhaseModulation,
		AmplitudeModulation,
		SignalMix,
		SynchronizedBySubOsc,
		FrequencyModulation,
		NumModulationAlgos
	} ;


	Oscillator( const IntModel * _wave_shape_model,
			const IntModel * _mod_algo_model,
			const float & _freq,
			const float & _detuning,
			const float & _phase_offset,
			const float & _volume,
			Oscillator * _m_subOsc = NULL );
	virtual ~Oscillator()
	{
		delete m_subOsc;
	}

	static void waveTableInit();
	static void destroyFFTPlans();
	static void generateAntiAliasUserWaveTable( SampleBuffer * sampleBuffer);

	inline void setUseWaveTable(bool n)
	{
		m_useWaveTable = n;
	}

	inline void setUserWave( const SampleBuffer * _wave )
	{
		m_userWave = _wave;
	}

	void update( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
// now follow the wave-shape-routines...

	static inline sample_t sinSample( const float _sample )
	{
		return sinf( _sample * F_2PI );
	}

	static inline sample_t triangleSample( const float _sample )
	{
		const float ph = fraction( _sample );
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
		return -1.0f + fraction( _sample ) * 2.0f;
	}

	static inline sample_t squareSample( const float _sample )
	{
		return ( fraction( _sample ) > 0.5f ) ? -1.0f : 1.0f;
	}

	static inline sample_t moogSawSample( const float _sample )
	{
		const float ph = fraction( _sample );
		if( ph < 0.5f )
		{
			return -1.0f + ph * 4.0f;
		}
		return 1.0f - 2.0f * ph;
	}

	static inline sample_t expSample( const float _sample )
	{
		float ph = fraction( _sample );
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

	inline wtSampleControl getWtSampleControl(const float sample)
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
		control.band = waveTableBandFromFreq(m_freq);
		return control;
	}

	inline sample_t wtSample(const sample_t table[OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT][OscillatorConstants::WAVETABLE_LENGTH], const float _sample)
	{
		wtSampleControl control = getWtSampleControl(_sample);
		return linearInterpolate(table[control.band][control.f1],
				table[control.band][control.f2], fraction(control.frame));
	}

	inline sample_t wtSample( sample_t **table, const float _sample)
	{
		wtSampleControl control = getWtSampleControl(_sample);
		return linearInterpolate(table[control.band][control.f1],
				table[control.band][control.f2], fraction(control.frame));
	}

	inline int  waveTableBandFromFreq(float freq)
	{
		int band = (69 + static_cast<int>(ceil(12.0f * log2f(freq / 440.0f)))) / OscillatorConstants::SEMITONES_PER_TABLE;
		//limit the returned value
		// qBound would require QT audio side, not a preferable option
		// c++17 std::clamp() could be used in the future
		return band <= 1 ? 1 : band >= OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT-1 ? OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT-1 : band;
	}

	static inline float freqFromWaveTableBand(int band)
	{
		return 440.0f * powf(2.0f, (band * OscillatorConstants::SEMITONES_PER_TABLE - 69.0f) / 12.0f);
	}

private:
	const IntModel * m_waveShapeModel;
	const IntModel * m_modulationAlgoModel;
	const float & m_freq;
	const float & m_detuning;
	const float & m_volume;
	const float & m_ext_phaseOffset;
	Oscillator * m_subOsc;
	float m_phaseOffset;
	float m_phase;
	const SampleBuffer * m_userWave;
	bool m_useWaveTable;

	/* Multiband WaveTable */
	static sample_t s_waveTables[WaveShapes::NumWaveShapes-2][OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT][OscillatorConstants::WAVETABLE_LENGTH];
	static fftwf_plan s_fftPlan;
	static fftwf_plan s_ifftPlan;
	static fftwf_complex * s_specBuf;
	static float s_fftBuffer[OscillatorConstants::WAVETABLE_LENGTH*2];
	static float s_sampleBuffer[OscillatorConstants::WAVETABLE_LENGTH];

	static void generateSineWaveTable(sample_t * table);
	static void generateSawWaveTable(int bands, sample_t * table);
	static void generateTriangleWaveTable(int bands, sample_t * table);
	static void generateSquareWaveTable(int bands, sample_t * table);
	static void generateFromFFT(int bands, float threshold, sample_t *table);
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
