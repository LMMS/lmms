/*
 * GranularPitchShifter.h
 *
 * Copyright (c) 2024 Lost Robot <r94231/at/gmail/dot/com>
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

#ifndef LMMS_GRANULAR_PITCH_SHIFTER_EFFECT_H
#define LMMS_GRANULAR_PITCH_SHIFTER_EFFECT_H

#include <numbers>

#include "Effect.h"
#include "GranularPitchShifterControls.h"

#include "interpolation.h"

namespace lmms
{

constexpr float PrefilterBandwidth = 0.96f;// 96% of nyquist
constexpr double GlideSnagRadius = 0.001;
constexpr int SafetyLatency = 3;
constexpr float RangeSeconds[5] = {5, 10, 40, 40, 120};
constexpr float DcRemovalHz = 7.f;
constexpr float SatuSafeVol = 16.f;
constexpr float SatuStrength = 0.001f;


class GranularPitchShifterEffect : public Effect
{
public:
	GranularPitchShifterEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~GranularPitchShifterEffect() override = default;

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_granularpitchshifterControls;
	}
	
	// double index and fraction are required for good quality
	float getHermiteSample(double index, int ch)
	{
		const auto index_floor = static_cast<std::size_t>(index);
		const double fraction = index - index_floor;
		
		float v0, v1, v2, v3;

		if (index_floor == 0) { v0 = m_ringBuf[m_ringBuf.size() - 1][ch]; }
		else { v0 = m_ringBuf[index_floor - 1][ch]; }
		
		v1 = m_ringBuf[index_floor][ch];
		
		if(index_floor >= m_ringBuf.size() - 2)
		{
			v2 = m_ringBuf[(index_floor + 1) % m_ringBuf.size()][ch];
			v3 = m_ringBuf[(index_floor + 2) % m_ringBuf.size()][ch];
		}
		else
		{
			v2 = m_ringBuf[index_floor + 1][ch];
			v3 = m_ringBuf[index_floor + 2][ch];
		}
		
		return hermiteInterpolate(v0, v1, v2, v3, static_cast<float>(fraction));
	}
	
	// adapted from signalsmith's crossfade approximation:
	// https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade
	float cosHalfWindowApprox(float x, float k)
	{
		float A = x * (1 - x);
		float B = A * (1 + k * A);
		float C = (B + x);
		return C * C;
	}
	// 1-2 fades between equal-gain and equal-power
	float cosWindowApproxK(float p)
	{
		return -6.0026608f + p * (6.8773512f - 1.5838104f * p);
	}
	
	// designed to use minimal CPU if the input isn't loud
	float safetySaturate(float input)
	{
		float absInput = std::abs(input);
		return absInput <= SatuSafeVol ? input :
			std::copysign((absInput - SatuSafeVol) / (1 + (absInput - SatuSafeVol) * SatuStrength) + SatuSafeVol, input);
	}
	
	void sampleRateNeedsUpdate() { m_sampleRateNeedsUpdate = true; }
	
	void changeSampleRate();

private:
	struct PrefilterLowpass
	{
		float m_v0z = 0.f, m_v1 = 0.f, m_v2 = 0.f;
		float m_g1, m_g2, m_g3, m_g4;

		void setCoefs(float sampleRate, float cutoff)
		{
			using namespace std::numbers;
			const float g = std::tan(pi_v<float> * cutoff / sampleRate);
			const float ginv = g / (1.f + g * (g + sqrt2_v<float>));
			m_g1 = ginv;
			m_g2 = 2 * (g + sqrt2_v<float>) * ginv;
			m_g3 = g * ginv;
			m_g4 = 2 * ginv;
		}

		float process(float input)
		{
		    const float v1z = m_v1;
		    const float v3 = input + m_v0z - 2.f * m_v2;
		    m_v1 += m_g1 * v3 - m_g2 * v1z;
		    m_v2 += m_g3 * v3 + m_g4 * v1z;
		    m_v0z = input;
		    return m_v2;
		}
	};

	struct Grain
	{
		Grain(double grainSpeedL, double grainSpeedR, double phaseSpeedL, double phaseSpeedR, double readPointL, double readPointR) :
			readPoint{readPointL, readPointR},
			phaseSpeed{phaseSpeedL, phaseSpeedR},
			grainSpeed{grainSpeedL, grainSpeedR},
			phase{0}
		{}
		std::array<double, 2> readPoint;
		std::array<double, 2> phaseSpeed;
		std::array<double, 2> grainSpeed;
		double phase;
	};
	
	GranularPitchShifterControls m_granularpitchshifterControls;
	
	std::vector<std::array<float, 2>> m_ringBuf;
	std::vector<Grain> m_grains;

	std::array<PrefilterLowpass, 2> m_prefilter;
	std::array<double, 2> m_speed = {1, 1};
	std::array<double, 2> m_truePitch = {0, 0};
	std::array<float, 2> m_dcVal = {0, 0};

	float m_sampleRate;
	float m_nyquist;
	float m_nextWaitRandomization = 1;
	float m_dcCoeff;

	int m_ringBufLength = 0;
	int m_writePoint = 0;
	int m_grainCount = 0;
	int m_timeSinceLastGrain = 999999999;

	double m_oldGlide = -1;
	double m_glideCoef = 0;

	bool m_sampleRateNeedsUpdate = false;
	bool m_updatePitches = true;

	friend class GranularPitchShifterControls;
};


} // namespace lmms

#endif // LMMS_GRANULAR_PITCH_SHIFTER_EFFECT_H
