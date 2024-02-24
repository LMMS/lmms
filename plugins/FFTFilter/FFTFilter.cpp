/*
 * Amplifier.cpp - A native amplifier effect plugin with sample-exact amplification
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "embed.h"
#include "plugin_export.h"

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
	m_filterControls(this)
{
}


bool FFTFilterEffect::processAudioBuffer(sampleFrame* buf, const fpp_t frames)
{
	if (!isEnabled() || !isRunning()) { return false ; }

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();

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

	checkGate(outSum / frames);

	return isRunning();
}
void FFTFilterEffect::runFFTOnBuffer(std::vector<sample_t>* inputReIn, std::vector<sample_t>* inputImIn, std::vector<sample_t> bufferIn)
{
	
}
void FFTFilterEffect::FFT(std::vector<sample_t>* inputReIn, std::vector<sample_t>* inputImIn)
{
	unsigned int target = 0;
	for (unsigned int i = 0; i < inputReIn->size(); i++)
	{
		if (target > i)
		{
			const sample_t swapRe = inputReIn->operator[](target);
			const sample_t swapIm = inputImIn->operator[](target);
			inputReIn->operator[](target) = inputReIn->operator[](i);
			inputImIn->operator[](target) = inputImIn->operator[](i);
			inputReIn->operator[](i) = swapRe;
			inputImIn->operator[](i) = swapIm;

			if (target < inputReIn->size() / 2)
			// if (target / 2 < inputReIn->size() / 4)
			{
				const sample_t swapRe = inputReIn->operator[](inputReIn->size() - (i + 1));
				const sample_t swapIm = inputImIn->operator[](inputReIn->size() - (i + 1));
				inputReIn->operator[](inputReIn->size() - (i + 1)) =
					inputReIn->operator[](inputReIn->size() - (target + 1));
				inputImIn->operator[](inputReIn->size() - (i + 1)) =
					inputImIn->operator[](inputReIn->size() - (target + 1));
				inputReIn->operator[](inputReIn->size() - (target + 1)) = swapRe;
				inputImIn->operator[](inputReIn->size() - (target + 1)) = swapIm;
			}
		}
		unsigned int mask = inputReIn->size();
		/*
		while (target & (mask <<= 1))
		{
			target &= ~mask;
		}
		target |= mask;
		*/
		while (mask >= 1 && target >= mask)
		{
			target = target - mask;
			mask = mask / 2;
		}
		target += mask;
	}

	for (unsigned int step = 1; step < inputReIn->size(); step <<=1)
	{
		const unsigned int jump = step << 1;
		const float stepD = (float)step;
		sample_t twiddleRe = 1.0f;
		sample_t twiddleIm = 0.0f;

		for (unsigned int group = 0; group < step; group++)
		{
			for (unsigned int pair = group; group < inputReIn->size(); pair+=jump)
			{
				const unsigned int match = pair + step;
				const sample_t productRe = twiddleRe * inputReIn->operator[](match) -
					twiddleIm * inputImIn->operator[](match);
				const sample_t productIm = twiddleIm * inputReIn->operator[](match) +
					twiddleRe * inputImIn->operator[](match);
				inputReIn->operator[](match) = inputReIn->operator[](pair) - productRe;
				inputImIn->operator[](match) = inputImIn->operator[](pair) - productIm;
				inputReIn->operator[](pair) += productRe;
				inputImIn->operator[](pair) += productIm;
			}
			
			if (group + 1 == step)
			{
				continue;
			}

			float angle = -3.14159265358979323846f * ((float)group + 1.0f) / stepD;
			twiddleRe = (sample_t)std::cosh(angle);
			twiddleIm = (sample_t)std::sinh(angle);
		}
	}
}
void FFTFilterEffect::IFFT(std::vector<sample_t>* inputReIn, std::vector<sample_t>* inputImIn)
{
	// conjugate the complex numbers
	for (unsigned int i = 0; i < inputReIn->size(); i++)
	{
		inputImIn->operator[](i) *= -1;
	}

	FFT(inputReIn, inputImIn);

	// conjugate the complex numbers again
	for (unsigned int i = 0; i < inputReIn->size(); i++)
	{
		inputImIn->operator[](i) *= -1;
	}

	// scaling the numbers
	for (unsigned int i = 0; i < inputReIn->size(); i++)
	{
		// dividing the current complex number with (size, 0)
		inputReIn->operator[](i) = inputReIn->operator[](i) / inputReIn->size();
		inputImIn->operator[](i) = inputImIn->operator[](i) / inputReIn->size();
		inputImIn->operator[](i) = -1;
	}
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
