/*
 * PredictionDistort.cpp - A native distort effect that tries to predict the next sample
 *
 * Copyright (c) 2025 szeli1
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

#include "PredictionDistort.h"

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT PredictionDistort_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"Prediction Distortion",
	QT_TRANSLATE_NOOP("PluginBrowser", "This effect tries to predict the next sample using second degree polynomials"),
	"szeli1",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
} ;

}


PredictionDistortEffect::PredictionDistortEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&PredictionDistort_plugin_descriptor, parent, key),
	m_effectControls(this)
{
}


Effect::ProcessStatus PredictionDistortEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	size_t range = m_effectControls.m_rangeModel.value();
	float mixVal = m_effectControls.m_mixModel.value();
	size_t decayVal = static_cast<size_t>(m_effectControls.m_decayModel.value());
	float invMix = 1.0f - mixVal;

	size_t decayRange = range + 3;
	size_t decayRangeMid = decayRange / 2;
	size_t decayRangeEnd = decayRange - 1;
	m_retainCount = decayVal + decayRange;
	if (m_inputData.size() - m_retainCount != frames) { m_inputData.resize(frames + m_retainCount); }
	m_inputData.write(buf, frames);

	constexpr float treshold = 0.002f;
	size_t processDecayCount = mixVal <= treshold ? 1 : static_cast<size_t>(std::log(mixVal) / std::log(invMix) + 1.0f);
	processDecayCount = std::min(decayVal, processDecayCount);

	bool isReverse = m_effectControls.m_isReverseModel.value();
	if (isReverse == false)
	{
		size_t processDecayCountX = processDecayCount + 2;
		for (fpp_t i = 0; i < frames; i++)
		{
			for (fpp_t j = 0; j < processDecayCount; j++)
			{
				fpp_t index = i + j;
				// buf[i] is m_retainCount foreward compared to m_inputData[i]
				buf[i][0] = std::clamp(predictNext(m_inputData[index][0], m_inputData[index + decayRangeMid][0], m_inputData[index + decayRangeEnd][0], processDecayCountX - j), -1.0f, 1.0f) * mixVal + buf[i][0] * invMix;
				buf[i][1] = std::clamp(predictNext(m_inputData[index][1], m_inputData[index + decayRangeMid][1], m_inputData[index + decayRangeEnd][1], processDecayCountX - j), -1.0f, 1.0f) * mixVal + buf[i][1] * invMix;
			}
		}
	}
	else
	{
		for (fpp_t i = 0; i < frames; i++)
		{
			for (fpp_t j = processDecayCount - 1; j-- > 0;) //! < thx Lost Robot
			{
				fpp_t index = i + j;
				buf[i][0] = std::clamp(predictNext(m_inputData[index][0], m_inputData[index + decayRangeMid][0], m_inputData[index + decayRangeEnd][0], 3), -1.0f, 1.0f) * mixVal + buf[i][0] * invMix;
				buf[i][1] = std::clamp(predictNext(m_inputData[index][1], m_inputData[index + decayRangeMid][1], m_inputData[index + decayRangeEnd][1], 3), -1.0f, 1.0f) * mixVal + buf[i][1] * invMix;
			}
		}
	}
	return ProcessStatus::ContinueIfNotQuiet;
}

float PredictionDistortEffect::predictNext(float y1, float y2, float y3, float predictX)
{
	// calculating second degree polynomial with 3 input points
	// y = ax^2 + bx + c
	// x1 = 0, x2 = 1, x3 = 2, output: x4 = `predictX`
	float c = y1;
	float b = 2.0f * y2 - (3.0f * y1 + y3) / 2.0f;
	float a = y2 - y1 - b;
	return predictX * predictX * a + predictX * b + c;
}

template<typename T>
T& PredictionDistortEffect::storageBuffer<T>::operator[](size_t index)
{
	return m_data[(m_readIndex + index) % m_data.size()];
}
template<typename T>
void PredictionDistortEffect::storageBuffer<T>::clear()
{
	m_data.clear();
	m_readIndex = 0;
}
template<typename T>
size_t PredictionDistortEffect::storageBuffer<T>::size()
{
	return m_data.size();
}
template<typename T>
void PredictionDistortEffect::storageBuffer<T>::resize(size_t newSize)
{
	m_data.resize(newSize);
	m_readIndex = m_readIndex % m_data.size();
}
template<typename T>
void PredictionDistortEffect::storageBuffer<T>::write(const T* buf, size_t frames)
{
	if (m_data.size() <= 0 || frames <= 0) { return; }
	for (size_t i = 0; i < frames; i++)
	{
		m_data[m_readIndex] = buf[i];
		m_readIndex = m_readIndex + 1 < m_data.size() ? m_readIndex + 1 : 0;
	}
}


void PredictinDistortEffect::getPolinomialCoefficients(const std::vector<float>& helperMatrix,
	const std::vector<float>& samples, std::vector<float>& polinomial, size_t width)
{
	for (size_t y = 0; y < width; ++y)
	{
		polinomial[y] = 0.0f;
		for (size_t x = 0; x < width; ++x)
		{
			polinomial[y] += helperMatrix[y * width + x] * samples[x];
		}
	}
}

void PredictinDistortEffect::getMatrix(std::vector<float>& matrix, size_t width)
{
	std::vector<float> coefficientMatrix(width * width);
	// x coordinates correspond to indexes in the source audio
	// a polinomial consist of increasing whole powers of these x coords:
	// "a*x^2 + b*x + c"
	// w = 3    c  b  a   f(1) f(2) f(3)    ---->    c  b  a      1 <= t <= w [matrix] OUT
	// x = 1 [  1  1  1  |  1    0    0  ]        [  1  0  0  | linear combination of f(t) ]
	// x = 2 [  1  2  4  |  0    1    0  ]        [  0  1  0  | linear combination of f(t) ]
	// x = 3 [  1  3  9  |  0    0    1  ]        [  0  0  1  | linear combination of f(t) ]
	for (size_t i = 0; i < size; ++i)
	{
		float poweredInput{i + 1};
		for (size_t j = 0; j < width; ++j)
		{
			size_t index = i * width + j;
			coefficientMatrix[index] = poweredInput;
			poweredInput = poweredInput * i;
		}
	}
	for (size_t i = 0; i < width; ++i)
	{
		for (size_t j = 0; j < width; ++j)
		{
			matrix[i * width + j] = 0.0f;
		}
		matrix[i * width + i] = 1.0f;
	}

	
	// Gauss elimination
	int x = 0;
	int y = 0;
	for (; y < width; ++y)
	{
		// if 0
		if (std::abs(coefficientMatrix[y * width + x]) < 0.0001f)
		{
			while (x < width)
			{
				bool found = false;
				for (int ny = y + 1; ny < width; ++ny)
				{
					if (std::abs(coefficientMatrix[ny * width + x]) >= 0.0001f)
					{
						// swap y with ny
						for (int nx = x; nx < width; ++nx)
						{
							float swap = coefficientMatrix[ny * width + nx];
							coefficientMatrix[ny * width + nx] = coefficientMatrix[y * width + nx];
							coefficientMatrix[y * width + nx] = swap;
							// mirror
							swap = matrix[ny * width + nx];
							matrix[ny * width + nx] = matrix[y * width + nx];
							matrix[y * width + nx] = swap;
						}
						found = true;
						break;
					}
				}
				// if we managed to move a non 0 row to (x, y)
				if (found) { break; }
				++x;
			}
			if (x >= width) { assert(false); /* 0 row isn't handled (should never happen for this input) */ }
		}
		
		float quotient = 1.0f / coefficientMatrix[y * width + x];
		coefficientMatrix[y * width + x] = 1.0f;
		matrix[y * width + x] *= quotient; // mirror
		for (int nx = x + 1; nx < width; ++nx)
		{
			coefficientMatrix[y * width + nx] *= quotient;
			// mirror
			matrix[y * width + nx] *= quotient;
		}

		for (int ny = y + 1; ny < width; ++ny)
		{
			float multiplier = -coefficientMatrix[ny * width + x];
			coefficientMatrix[ny * width + x] = 0.0f;
			matrix[ny * width + x] += matrix[y * width + x] * multiplier; // mirror
			for (int nx = x + 1; nx < width; ++nx)
			{
				coefficientMatrix[ny * width + nx] += coefficientMatrix[y * width + nx] * multiplier;
				// mirror
				matrix[ny * width + nx] += matrix[y * width + nx] * multiplier;
			}
		}
		++x;
		if (x >= width) { break; }
	}
	assert(x == width); // Gauss elimination didn't finish
	assert(y == width); // Gauss elimination didn't finish
	--x;
	--y;
	for (; y >= 0; --y)
	{
		// finding leading 1
		if (std::abs(coefficientMatrix[y * width + x] - 1.0f) > 0.0001f)
		{
			while (--x >= 0)
			{
				// if we found the leading edge
				if (std::abs(coefficientMatrix[y * width + x] - 1.0f) > 0.0001f) { break; }
			}

			if (x < 0) { assert(false); /* this should never happen */ }
		}
		
		for (size_t ny = y - 1; ny >= 0; --ny)
		{
			float multiplier = -coefficientMatrix[ny * width + x];
			coefficientMatrix[ny * width + x] = 0.0f;
			for (size_t nx = 0; nx < width; ++nx)
			{
				// mirror
				matrix[ny * width + nx] += matrix[y * width + nx] * multiplier;
			}
		}
		--x;
		if (x < 0) { break; }
	}
}

float PredictinDistortEffect::polinomialAt(float x, const std::vector<float>& polinomial, size_t width) const
{
	float factor = polinomial[width - 1];
	for (int i = width - 2; i >= 0; --i)
	{
		factor = factor * x + polinomial[i];
	}
	return factor;
}

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	return new PredictionDistortEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}

}

} // namespace lmms
