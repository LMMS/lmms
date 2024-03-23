/*
 * FFTFilter.h - FFTFilter effect for lmms
 *
 * Copyright (c) 2024 Szeli1 </at/gmail/dot/com> TODO
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

#ifndef LMMS_FFTFILTER_H
#define LMMS_FFTFILTER_H

#include "vector"

#include "Effect.h"
#include "FFTFilterControls.h"

namespace lmms
{

class FFTFilterEffect : public Effect
{
public:
	FFTFilterEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~FFTFilterEffect() override = default;
	bool processAudioBuffer(sampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_filterControls;
	}

private:
	void runFFTOnBuffer(std::vector<sample_t>* inputReIn, std::vector<sample_t>* inputImIn, std::vector<sample_t> bufferIn);
	// implementation of fft
	void FFT(std::vector<sample_t>* inputReIn, std::vector<sample_t>* inputImIn);
	// inverse fft
	void IFFT(std::vector<sample_t>* inputReIn, std::vector<sample_t>* inputImIn);

	inline sample_t getCurSample(unsigned int posIn)
	{
		if (m_useSecondBufferOut == true)
		{
			return m_bufferB[posIn];
		}
		else
		{
			return m_bufferA[posIn];
		}
	}
	inline void setCurSample(unsigned int posIn, sample_t sampleIn)
	{
		if (m_useSecondBufferOut == true)
		{
			m_bufferA[posIn] = sampleIn;
		}
		else
		{
			m_bufferB[posIn] = sampleIn;
		}
	}

	FFTFilterControls m_filterControls;

	std::vector<sample_t> m_bufferA;
	std::vector<sample_t> m_bufferB;
	bool m_useSecondBufferOut;

	std::vector<float> m_FFTFilterData;


	friend class FFTFilterControls;
};

} // namespace lmms

#endif // LMMS_FFTFILTER_H
