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

#ifndef GRANULARPITCHSHIFTER_H
#define GRANULARPITCHSHIFTER_H

#include "Effect.h"
#include "GranularPitchShifterControls.h"

#include "BasicFilters.h"
#include "interpolation.h"

namespace lmms
{

constexpr float GPS_PREFILTER_BW = 0.96f;// 96% of nyquist
constexpr double GPS_GLIDE_SNAG_RADIUS = 0.001;
constexpr int GPS_SAFETY_LATENCY = 3;

// adapted from signalsmith's crossfade approximation:
// https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade
inline float cosHalfWindowApprox(float x, float k) {
	float A = x * (1 - x);
	float B = A * (1 + k * A);
	float C = (B + x);
	return C * C;
}
// 1-2 fades between equal-gain and equal-power
inline float cosWindowApproxK(float p) {
	return -6.0026608f + p * (6.8773512f - 1.5838104f * p);
}

class PrefilterLowpass
{
public:
	inline void setCoefs(float sampleRate, float cutoff)
	{
		const float g = std::tan(F_PI * cutoff / sampleRate);
		const float ginv = g / (1.0f + g * (g + 1.414213562));
		m_g1 = ginv;
		m_g2 = 2.0f * (g + 1.414213562) * ginv;
		m_g3 = g * ginv;
		m_g4 = 2.0f * ginv;
	}

	inline float process(float input)
	{
		const float v1z = m_v1;
		const float v3 = input + m_v0z - 2.0f * m_v2;
		m_v1 += m_g1 * v3 - m_g2 * v1z;
		m_v2 += m_g3 * v3 + m_g4 * v1z;
		m_v0z = input;
		return m_v2;
	}

private:
	float m_v0z = 0.0f, m_v1 = 0.0f, m_v2 = 0.0f;
	float m_g1, m_g2, m_g3, m_g4;
};

class GranularPitchShifterGrain final {
public:
	GranularPitchShifterGrain(double grainSpeedL, double grainSpeedR, double phaseSpeedL, double phaseSpeedR, double readPointL, double readPointR) :
		m_readPoint({readPointL, readPointR}),
		m_phaseSpeed({phaseSpeedL, phaseSpeedR}),
		m_grainSpeed({grainSpeedL, grainSpeedR}),
		m_phase(0) {}
	std::array<double, 2> m_readPoint;
	std::array<double, 2> m_phaseSpeed;
	std::array<double, 2> m_grainSpeed;
	double m_phase;
};

class GranularPitchShifterEffect : public Effect
{
public:
	GranularPitchShifterEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~GranularPitchShifterEffect() override = default;
	bool processAudioBuffer(sampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_granularpitchshifterControls;
	}
	
	// double index and fraction are required for good quality
	inline float getHermiteSample(double index, int ch) {
		const int index_floor = static_cast<int>(index);
		const double fraction = index - index_floor;
		
		float v0, v1, v2, v3;

		if (index_floor == 0) {
			v0 = m_ringBuf[m_ringBuf.size() - 1][ch];
		} else {
			v0 = m_ringBuf[index_floor - 1][ch];
		}
		
		v1 = m_ringBuf[index_floor][ch];
		
		if(index_floor >= m_ringBuf.size() - 2) {
			v2 = m_ringBuf[(index_floor + 1) % m_ringBuf.size()][ch];
			v3 = m_ringBuf[(index_floor + 2) % m_ringBuf.size()][ch];
		} else {
			v2 = m_ringBuf[index_floor + 1][ch];
			v3 = m_ringBuf[index_floor + 2][ch];
		}
		
		return hermiteInterpolate(v0, v1, v2, v3, static_cast<float>(fraction));
	}
	
	void sampleRateNeedsUpdate() { m_sampleRateNeedsUpdate = true; }
	
	void changeSampleRate();

private:
	GranularPitchShifterControls m_granularpitchshifterControls;
	
	std::vector<std::array<float, 2>> m_ringBuf;
	std::vector<GranularPitchShifterGrain> m_grains;

	std::array<PrefilterLowpass, 2> m_prefilter;
	std::array<double, 2> m_speed = {1, 1};
	std::array<double, 2> m_truePitch = {0, 0};

	float m_sampleRate;
	float m_nyquist;
	float m_nextWaitRandomization = 1;

	int m_ringBufLength = 0;
	int m_writePoint = 0;
	int m_grainCount = 0;
	int m_timeSinceLastGrain = 0;

	double m_oldGlide = -1;
	double m_glideCoef = 0;

	bool m_sampleRateNeedsUpdate = false;
	bool m_updatePitches = true;

	friend class GranularPitchShifterControls;
};


} // namespace lmms

#endif
