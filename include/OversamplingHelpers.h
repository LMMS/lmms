/*
 * OversamplingHelpers.h
 *
 * Copyright (c) 2023 Lost Robot <r94231/at/gmail/dot/com>
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

#ifndef OVERSAMPLING_HELPERS_H
#define OVERSAMPLING_HELPERS_H

#include <algorithm>
#include <array>

#include "hiir/PolyphaseIir2Designer.h"
#include "hiir/Downsampler2xFpu.h"
#include "hiir/Upsampler2xFpu.h"

constexpr float HIIR_DEFAULT_PASSBAND = 19600;
constexpr int HIIR_DEFAULT_MAX_COEFS = 8;

namespace lmms
{

template<int MaxStages, int MaxCoefs = 8>
class Upsampler
{
public:
	void reset()
	{
		float bw = 0.5f - m_passband / m_sampleRate;

		// Stage 1
		double coefsFirst[MaxCoefs];
		hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(coefsFirst, MaxCoefs, bw);
		m_upsampleFirst.set_coefs(coefsFirst);
		m_upsampleFirst.clear_buffers();
		bw = (bw + 0.5f) * 0.5f;

		// Stage 2
		constexpr int secondCoefCount = std::max(MaxCoefs / 2, 2);
		double coefsSecond[secondCoefCount];
		hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(coefsSecond, secondCoefCount, bw);
		m_upsampleSecond.set_coefs(coefsSecond);
		m_upsampleSecond.clear_buffers();
		bw = (bw + 0.5f) * 0.5f;

		// Remaining stages
		constexpr int restCoefCount = std::max(MaxCoefs / 4, 2);
		for (int i = 0; i < m_stages - 2; ++i)
		{
			double coefsRest[restCoefCount];
			hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(coefsRest, restCoefCount, bw);
			m_upsampleRest[i].set_coefs(coefsRest);
			m_upsampleRest[i].clear_buffers();
			bw = (bw + 0.5f) * 0.5f;
		}
	}

	void setup(int stages, float sampleRate, float passband = HIIR_DEFAULT_PASSBAND)
	{
		assert(stages <= MaxStages);
		m_stages = stages;
		m_sampleRate = sampleRate;
		m_passband = passband;
		reset();
	}

	void process_sample(float* outSamples, float inSample)
	{
		int total = 1 << m_stages;
		outSamples[0] = inSample;
		int gap1 = total / 2;

		if (m_stages >= 1)
		{
			m_upsampleFirst.process_sample(outSamples[0], outSamples[gap1], outSamples[0]);
		}

		if (m_stages >= 2)
		{
			int gap2 = gap1 / 2;
			m_upsampleSecond.process_sample(outSamples[0], outSamples[gap2], outSamples[0]);
			m_upsampleSecond.process_sample(outSamples[gap1], outSamples[gap1 + gap2], outSamples[gap1]);
		}

		for (int i = 2; i < m_stages; ++i)
		{
			int count = 1 << i;
			int gap = total / count;
			for (int j = 0; j < count; ++j)
			{
				int temp = j * gap;
				m_upsampleRest[i - 2].process_sample(outSamples[temp], outSamples[temp + gap / 2], outSamples[temp]);
			}
		}
	}

	int getStages() const { return m_stages; }
	float getSampleRate() const { return m_sampleRate; }
	float getPassband() const { return m_passband; }

	void setStages(int stages) { m_stages = stages; reset(); }
	void setSampleRate(float sampleRate) { m_sampleRate = sampleRate; reset(); }
	void setPassband(float passband) { m_passband = passband; reset(); }

private:
	hiir::Upsampler2xFpu<MaxCoefs> m_upsampleFirst;
	hiir::Upsampler2xFpu<std::max(MaxCoefs / 2, 2)> m_upsampleSecond;
	std::array<hiir::Upsampler2xFpu<std::max(MaxCoefs / 4, 2)>, MaxStages - 2> m_upsampleRest;

	int m_stages = 0;
	float m_sampleRate = 44100;
	float m_passband = HIIR_DEFAULT_PASSBAND;
};


template<int MaxStages, int MaxCoefs = 8>
class Downsampler
{
public:
	void reset()
	{
		float bw = 0.5f - m_passband / m_sampleRate;

		// Stage 1
		double coefsFirst[MaxCoefs];
		hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(coefsFirst, MaxCoefs, bw);
		m_downsampleFirst.set_coefs(coefsFirst);
		m_downsampleFirst.clear_buffers();
		bw = (bw + 0.5f) * 0.5f;

		// Stage 2
		constexpr int secondCoefCount = std::max(MaxCoefs / 2, 2);
		double coefsSecond[secondCoefCount];
		hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(coefsSecond, secondCoefCount, bw);
		m_downsampleSecond.set_coefs(coefsSecond);
		m_downsampleSecond.clear_buffers();
		bw = (bw + 0.5f) * 0.5f;

		// Remaining stages
		constexpr int restCoefCount = std::max(MaxCoefs / 4, 2);
		for (int i = 0; i < m_stages - 2; ++i)
		{
			double coefsRest[restCoefCount];
			hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw(coefsRest, restCoefCount, bw);
			m_downsampleRest[i].set_coefs(coefsRest);
			m_downsampleRest[i].clear_buffers();
			bw = (bw + 0.5f) * 0.5f;
		}
	}

	void setup(int stages, float sampleRate, float passband = HIIR_DEFAULT_PASSBAND)
	{
		assert(stages <= MaxStages);
		m_stages = stages;
		m_sampleRate = sampleRate;
		m_passband = passband;
		reset();
	}

	float process_sample(float* inSamples)
	{
		for (int i = m_stages - 1; i >= 2; --i)
		{
			for (int j = 0; j < 1 << i; ++j)
			{
				inSamples[j] = m_downsampleRest[i - 2].process_sample(&inSamples[j * 2]);
			}
		}

		if (m_stages >= 2)
		{
			for (int j = 0; j < 2; ++j)
			{
				inSamples[j] = m_downsampleSecond.process_sample(&inSamples[j * 2]);
			}
		}

		if (m_stages >= 1)
		{
			inSamples[0] = m_downsampleFirst.process_sample(&inSamples[0]);
		}

		return inSamples[0];
	}

	int getStages() const { return m_stages; }
	float getSampleRate() const { return m_sampleRate; }
	float getPassband() const { return m_passband; }

	void setStages(int stages) { m_stages = stages; reset(); }
	void setSampleRate(float sampleRate) { m_sampleRate = sampleRate; reset(); }
	void setPassband(float passband) { m_passband = passband; reset(); }

private:
	hiir::Downsampler2xFpu<MaxCoefs> m_downsampleFirst;
	hiir::Downsampler2xFpu<std::max(MaxCoefs / 2, 2)> m_downsampleSecond;
	std::array<hiir::Downsampler2xFpu<std::max(MaxCoefs / 4, 2)>, MaxStages - 2> m_downsampleRest;

	int m_stages = 0;
	float m_sampleRate = 44100;
	float m_passband = HIIR_DEFAULT_PASSBAND;
};


} // namespace lmms

#endif

