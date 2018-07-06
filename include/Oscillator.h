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
#include "SampleBuffer.h"
#include "lmms_constants.h"


class IntModel;


class EXPORT Oscillator
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

	static const int WAVETABLE_LENGTH = 1024;
	static const int MAX_FREQ = 20000; //limit to the audio spectrum
	static const int SEMITONES_PER_TABLE = 4;
	static const int WAVE_TABLES_PER_WAVEFORM_COUNT = 128 / Oscillator::SEMITONES_PER_TABLE;

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


	void waveTableInit();


	inline void setUseWaveTable(bool n)
	{
		m_useWaveTable = n;
	}

	inline void setUserWave( const SampleBuffer * _wave )
	{
		m_userWave = _wave;

		 //the generation of the anti alised wave tables may block
		// as allcation, and deallocation of fft tables is perfoemed,
		// along side the processing
		// may require calling from the functions, when the file access is performed
		generateAntiAliasUserWaveTable();
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

	inline sample_t wtSample(sample_t table[WAVE_TABLES_PER_WAVEFORM_COUNT][WAVETABLE_LENGTH], const float _sample)
	{
		const float frame = _sample * WAVETABLE_LENGTH;
		f_cnt_t f1 = static_cast<f_cnt_t>(frame) % WAVETABLE_LENGTH;
		if (f1 < 0)
		{
			f1 += WAVETABLE_LENGTH;
		}
		f_cnt_t f2 = f1 < WAVETABLE_LENGTH - 1 ?
					f1 + 1 :
					0;
		int band = waveTableBandFromFreq(m_freq);
		return linearInterpolate(table[band][f1],
				table[band][f2], fraction(frame));
	}

	inline int  waveTableBandFromFreq(float freq)
	{
		return (69 + static_cast<int>(ceil(12.0f * log2f(freq / 440.0f)))) / SEMITONES_PER_TABLE;
	}

	inline float freqFromWaveTableBand(int band)
	{
		return 440.0f * powf(2.0f, (band * SEMITONES_PER_TABLE - 69.0f) / 12.0f);
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
	static float *s_waveTableBandFreqs;
	static sample_t s_waveTables[WaveShapes::NumWaveShapes-1][WAVE_TABLES_PER_WAVEFORM_COUNT][WAVETABLE_LENGTH];
	sample_t m_userAntiAliasWaveTable[WAVE_TABLES_PER_WAVEFORM_COUNT][WAVETABLE_LENGTH] = {0};
	fftwf_plan m_fftPlan;
	fftwf_plan m_ifftPlan;
	fftwf_complex * m_specBuf;
	float m_fftBuffer[WAVETABLE_LENGTH*2];
	float m_sampleBuffer[WAVETABLE_LENGTH];

	void generateSineWaveTable(sample_t * table);
	void generateSawWaveTable(int bands, sample_t * table);
	void generateTriangleWaveTable(int bands, sample_t * table);
	void generateSquareWaveTable(int bands, sample_t * table);
	void generateAntiAliasUserWaveTable();
	void generateFromFFT(int bands, float threshold, sample_t *table);
	void generateWaveTables();
	void createFFTPlans();
	void destroyFFTPlans();

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
