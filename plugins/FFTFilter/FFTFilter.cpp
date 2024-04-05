/*
 * FFTFilter.cpp - main FFTFilter class
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

#include <cmath>

#include "FFTFilter.h"
#include "FFTProcessor.h"

#include "embed.h"
#include "plugin_export.h"
#include "fft_helpers.h"
#include "lmms_constants.h"

namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT FFTFilter_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"FFTFilter",
	QT_TRANSLATE_NOOP("PluginBrowser", "TEMP"),
	"TEMP",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
} ;

}


FFTFilterEffect::FFTFilterEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&FFTFilter_plugin_descriptor, parent, key),
	m_filterControls(this),
	m_FFTProcessor(2048, true, FFTWindow::Rectangular),
	m_inputBuffer(2048)
{
	m_FFTProcessor.threadedStartThread(&m_inputBuffer, 0, true, false);
	
	//std::vector<float> graphXArray;
	//m_sampleRate;
	updateSampleRate();
}
FFTFilterEffect::~FFTFilterEffect()
{
	qDebug("FFTFilterEffect destrc 0");
	m_inputBuffer.wakeAll();
	//m_FFTProcessor.~FFTProcessor();
	//m_FFTProcessor.terminate();
	qDebug("FFTFilterEffect destrc 1");
}

bool FFTFilterEffect::processAudioBuffer(sampleFrame* buf, const fpp_t frames)
{
	if (!isEnabled() || !isRunning()) { return false ; }

	if (isNoneInput(buf, frames, 0.01f) == true)
	{
		//qDebug("FFTFilterEffect return false");
		return false;
	}


	qDebug("FFTFilterEffect write");

	m_inputBuffer.write(buf, frames, true);

	qDebug("FFTFilterEffect analyze");

	//m_FFTProcessor.analyze(&m_inputBuffer, &target);
	if (m_FFTProcessor.getOutputSpectrumChanged() == true)
	{
		std::vector<float> FFTSpectrumOutput = m_FFTProcessor.getNormSpectrum();
		
		std::vector<std::pair<float, float>> graphInput(80);
		if (graphInput.size() != graphXArray.size())
		{
			updateGraphXArray(graphInput.size());
		}

		float avgY = 0;
		int avgCount = 0;//FFTSpectrumOutput.size() / graphInput.size();
		if (avgCount <= 0)
		{
			avgCount = 1;
		}
		float avgSum = 0;
		int j = 0;
		for (unsigned int i = 0; i < FFTSpectrumOutput.size(); i++)
		{
			avgY = avgY + FFTSpectrumOutput[i];
			avgCount++;
			if (i + 1 > avgSum + graphXArray[j] * FFTSpectrumOutput.size() / graphXArraySum)
			{
				graphInput[j].second = amplifyY(avgY / static_cast<float>(avgCount), avgCount + 1.0f);
				qDebug("FFTFiterEffect i: %d, %f", i, graphInput[j].second);
				if (j >= graphInput.size())
				{
					break;
				}
				avgY = 0;
				avgSum = avgSum + graphXArray[j] * FFTSpectrumOutput.size() / graphXArraySum;
				avgCount = 0;
				j++;
			}
		}
		// log_10(100) = 2  -> 10^2 = 100
		for (unsigned int i = 0; i < graphInput.size(); i++)
		{
			graphInput[i].first = graphXArray[i];
		}
		// DEBUG
		
		/*
		for (unsigned int i = 0; i < FFTSpectrumOutput.size(); i++)
		{
			qDebug("FFTFilterEffect: [%d], %f", i, FFTSpectrumOutput[i]);
		}
		*/
		
		m_filterControls.setGraph(&graphInput);

		// bin to freq
		//return getNyquistFreq() * bin_index / binCount();
		//return getSampleRate() / 2.0f;
		//return (getSampleRate() / 2.0f) * x / binCount();
	}
	//std::vector<float>* output;
	//for (unsigned int i = 0; i < output->size(); i++)
	//{
		//qDebug("FFTFilterEffect: [%d], %f", i, output->operator[](i));
	//}
	//std::vector<float> testVector = {0.0f, 0.3f, 0.5f, -0.5f, 0.2f};
	//std::vector<float> testVector = {0.0f, 1.3f, 0.5f, -2.5f, 0.2f};
	//std::vector<float> testVector = {0.0f, 0.1f, 0.5f, 0.7f, 0.5f, 0.2f, 0.1f, -1.0f, -0.1f, -0.1f, 0.2f};


	//m_filterControls.setGraph(&testVector);

	/*
	const ValueBuffer* volumeBuf = m_filterControls.m_volumeModel.valueBuffer();
	const ValueBuffer* panBuf = m_filterControls.m_panModel.valueBuffer();
	const ValueBuffer* leftBuf = m_filterControls.m_leftModel.valueBuffer();
	const ValueBuffer* rightBuf = m_filterControls.m_rightModel.valueBuffer();

	for (fpp_t f = 0; f < frames; ++f)
	{
		const float volume = (volumeBuf ? volumeBuf->value(f) : m_filterControls.m_volumeModel.value()) * 0.01f;
		const float pan = (panBuf ? panBuf->value(f) : m_filterControls.m_panModel.value()) * 0.01f;
		const float left = (leftBuf ? leftBuf->value(f) : m_filterControls.m_leftModel.value()) * 0.01f;
		const float right = (rightBuf ? rightBuf->value(f) : m_filterControls.m_rightModel.value()) * 0.01f;

		const float panLeft = std::min(1.0f, 1.0f - pan);
		const float panRight = std::min(1.0f, 1.0f + pan);

		auto s = std::array{buf[f][0], buf[f][1]};

		s[0] *= volume * left * panLeft;
		s[1] *= volume * right * panRight;

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
		outSum += buf[f][0] * buf[f][0] + buf[f][1] * buf[f][1];
	}
	*/

	//checkGate(outSum / frames);

	return isRunning();
}

bool FFTFilterEffect::isNoneInput(sampleFrame* bufferIn, const fpp_t framesIn, const float cutOffIn)
{
	bool output = true;
	//qDebug("FFTFilterEffect check none input");
	for (unsigned int i = 0; i < cutOffIn; i++)
	{
		if (std::abs(bufferIn[i][0]) > cutOffIn)
		{
			output = false;
			break;
		}
		else if (std::abs(bufferIn[i][1]) > cutOffIn)
		{
			output = false;
			break;
		}
	}
	return output;
}
void FFTFilterEffect::updateBufferSize(unsigned int newBufferSizeIn)
{
	m_bufferSize = newBufferSizeIn;
	// TODO
}

void FFTFilterEffect::updateSampleRate()
{
	m_sampleRate = Engine::audioEngine()->processingSampleRate();
}
void FFTFilterEffect::updateGraphXArray(unsigned int sizeIn)
{
	updateSampleRate();
	graphXArray.resize(sizeIn);
	graphXArraySum = 0.0f;
	for (unsigned int i = 0; i < graphXArray.size(); i++)
	{
		graphXArray[i] = getTransformedX(i, graphXArray.size());
		graphXArraySum += graphXArray[i];
		qDebug("updateGraphXArray [%d] %f", i, graphXArray[i]);
	}
}
float FFTFilterEffect::getTransformedX(unsigned int locationIn, unsigned int sizeIn)
{
	float freq = (m_sampleRate / 2.0f) * locationIn / sizeIn;
		// bin to freq
		//return getNyquistFreq() * bin_index / binCount();
		//return getSampleRate() / 2.0f;
		//return (getSampleRate() / 2.0f) * x / binCount();

	float min = std::log10(FRANGE_AUDIBLE_START);
	float range = std::log10(FRANGE_AUDIBLE_END) - min;
	freq = (log10(freq) - min) / range;

	/*
	float min = FRANGE_AUDIBLE_START;
	float range = FRANGE_AUDIBLE_END - min;
	freq = (freq - min) / range;
	*/
	return freq > 0.0f ? freq : 0.0f;
}
float FFTFilterEffect::amplifyY(float yIn, float powerIn)
{
	float power = std::max(powerIn, 2.0f);
	return -std::pow(power, -yIn * power) + 1.0f;
	//return -std::pow(std::abs(yIn - 1.0f), 3.0f) + 1.0f;
}

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	return new FFTFilterEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}

}

} // namespace lmms
