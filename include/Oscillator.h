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

#ifndef LMMS_OSCILLATOR_H
#define LMMS_OSCILLATOR_H

#include <cassert>
#include <fftw3.h>
#include <memory>
#include <cstdlib>
#include <cmath>

#include "Engine.h"
#include "lmms_math.h"
#include "AudioEngine.h"
#include "OscillatorConstants.h"
#include "SampleBuffer.h"

namespace lmms
{


class IntModel;


class LMMS_EXPORT Oscillator
{
public:
	enum class WaveShape
	{
		Sine,
		Triangle,
		Saw,
		Square,
		MoogSaw,
		Exponential,
		WhiteNoise,
		UserDefined,
		Count //!< Number of all available wave shapes
	};
	constexpr static auto NumWaveShapes = static_cast<std::size_t>(WaveShape::Count);
	//! First wave shape that has a pre-generated table
	constexpr static auto FirstWaveShapeTable = static_cast<std::size_t>(WaveShape::Triangle);
	//! Number of band-limited wave shapes to be generated
	constexpr static auto NumWaveShapeTables = static_cast<std::size_t>(WaveShape::WhiteNoise) - FirstWaveShapeTable;

	enum class ModulationAlgo
	{
		PhaseModulation,
		AmplitudeModulation,
		SignalMix,
		SynchronizedBySubOsc,
		FrequencyModulation,
		Count
	} ;
	constexpr static auto NumModulationAlgos = static_cast<std::size_t>(ModulationAlgo::Count);

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
	static std::unique_ptr<OscillatorConstants::waveform_t> generateAntiAliasUserWaveTable(const SampleBuffer* sampleBuffer);

	inline void setUseWaveTable(bool n)
	{
		m_useWaveTable = n;
	}

	void setUserWave(std::shared_ptr<const SampleBuffer> _wave)
	{
		m_userWave = _wave;
	}

	void setUserAntiAliasWaveTable(std::shared_ptr<const OscillatorConstants::waveform_t> waveform)
	{
		m_userAntiAliasWaveTable = waveform;
	}

	void update(SampleFrame* ab, const fpp_t frames, const ch_cnt_t chnl, bool modulator = false);

	// now follow the wave-shape-routines...
	static inline sample_t sinSample( const float _sample )
	{
		return std::sin(_sample * 2 * std::numbers::pi_v<float>);
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
		return 1.0f - rand() * 2.0f / static_cast<float>(RAND_MAX);
	}

	static sample_t userWaveSample(const SampleBuffer* buffer, const float sample)
	{
		if (buffer == nullptr || buffer->size() == 0) { return 0; }
		const auto frames = buffer->size();
		const auto frame = absFraction(sample) * frames;
		const auto f1 = static_cast<f_cnt_t>(frame);

		return std::lerp(buffer->data()[f1][0], buffer->data()[(f1 + 1) % frames][0], fraction(frame));
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
		control.frame = absFraction(sample) * OscillatorConstants::WAVETABLE_LENGTH;
		control.f1 = static_cast<f_cnt_t>(control.frame);
		control.f2 = control.f1 < OscillatorConstants::WAVETABLE_LENGTH - 1 ?
					control.f1 + 1 :
					0;
		control.band = waveTableBandFromFreq(
			m_freq * m_detuning_div_samplerate * Engine::audioEngine()->outputSampleRate());
		return control;
	}

	inline sample_t wtSample(
		const sample_t table[OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT][OscillatorConstants::WAVETABLE_LENGTH],
		const float sample) const
	{
		assert(table != nullptr);
		wtSampleControl control = getWtSampleControl(sample);
		return std::lerp(table[control.band][control.f1], table[control.band][control.f2], fraction(control.frame));
	}

	sample_t wtSample(const OscillatorConstants::waveform_t* table, const float sample) const
	{
		assert(table != nullptr);
		wtSampleControl control = getWtSampleControl(sample);
		return std::lerp(
			(*table)[control.band][control.f1],
			(*table)[control.band][control.f2],
			fraction(control.frame)
		);
	}

	inline sample_t wtSample(sample_t **table, const float sample) const
	{
		assert(table != nullptr);
		wtSampleControl control = getWtSampleControl(sample);
		return std::lerp(table[control.band][control.f1], table[control.band][control.f2], fraction(control.frame));
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
		return 440.0f * std::exp2((band * OscillatorConstants::SEMITONES_PER_TABLE - 69.0f) / 12.0f);
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
	std::shared_ptr<const SampleBuffer> m_userWave = SampleBuffer::emptyBuffer();
	std::shared_ptr<const OscillatorConstants::waveform_t> m_userAntiAliasWaveTable;
	bool m_useWaveTable;
	// There are many update*() variants; the modulator flag is stored as a member variable to avoid
	// adding more explicit parameters to all of them. Can be converted to a parameter if needed.
	bool m_isModulator;

	/* Multiband WaveTable */
	static sample_t s_waveTables[NumWaveShapeTables][OscillatorConstants::WAVE_TABLES_PER_WAVEFORM_COUNT][OscillatorConstants::WAVETABLE_LENGTH];
	static fftwf_plan s_fftPlan;
	static fftwf_plan s_ifftPlan;
	static fftwf_complex * s_specBuf;
	static std::array<float, OscillatorConstants::WAVETABLE_LENGTH> s_sampleBuffer;

	static void generateSawWaveTable(int bands, sample_t* table, int firstBand = 1);
	static void generateTriangleWaveTable(int bands, sample_t* table, int firstBand = 1);
	static void generateSquareWaveTable(int bands, sample_t* table, int firstBand = 1);
	static void generateFromFFT(int bands, sample_t* table);
	static void generateWaveTables();
	static void createFFTPlans();

	/* End Multiband wavetable */


	void updateNoSub( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updatePM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updateAM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updateMix( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updateSync( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	void updateFM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );

	float syncInit( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	inline bool syncOk( float _osc_coeff );

	template<WaveShape W>
	void updateNoSub( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShape W>
	void updatePM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShape W>
	void updateAM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShape W>
	void updateMix( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShape W>
	void updateSync( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	template<WaveShape W>
	void updateFM( SampleFrame* _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );

	template<WaveShape W>
	inline sample_t getSample( const float _sample );

	inline void recalcPhase();

} ;


} // namespace lmms

#endif // LMMS_OSCILLATOR_H
