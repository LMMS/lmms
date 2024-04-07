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

#include <vector>

#include "Effect.h"
#include "FFTFilterControls.h"
#include "FFTProcessor.h"
#include "LocklessRingBuffer.h"

namespace lmms
{

class FFTFilterEffect : public Effect
{
public:
	FFTFilterEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~FFTFilterEffect() override;
	bool processAudioBuffer(sampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_filterControls;
	}

private:

	bool isNoneInput(sampleFrame* bufferIn, const fpp_t framesIn, const float cutOffIn);

	void updateBufferSize(unsigned int newBufferSizeIn);

	void updateSampleRate();
	void updateGraphXArray(unsigned int sizeIn);
	float getTransformedX(unsigned int locationIn, unsigned int sizeIn);
	float amplifyY(float yIn, float powerIn);

	void setOutputSamples(sampleFrame* sampleStartIn, unsigned int sizeIn);
	void updateSampleArray(std::vector<float>* newSamplesIn, unsigned int sampleLocIn);

	FFTFilterControls m_filterControls;
	FFTProcessor m_FFTProcessor;


	//std::vector<sample_t> m_bufferA;
	//std::vector<sample_t> m_bufferB;
	//bool m_useSecondBufferOut;

	LocklessRingBuffer<sampleFrame> m_inputBuffer;

	std::vector<sampleFrame> m_outSampleBufferA;
	std::vector<sampleFrame> m_outSampleBufferB;
	unsigned int m_outSampleUsedUp;
	bool m_outSampleBufferAUsed;

	std::vector<float> graphXArray;
	float graphXArraySum;
	unsigned int m_sampleRate;

	unsigned int m_bufferSize;

	sampleFrame debugLastSample;

	std::vector<float> m_complexMultiplier;

	friend class FFTFilterControls;
};

} // namespace lmms

#endif // LMMS_FFTFILTER_H
