/*
 * PolynomialExtrapolate.cpp - A native distort effect that tries to predict the next sample
 *
 * Copyright (c) 2025 - 2026 szeli1
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

#include "PolynomialExtrapolate.h"

#include "embed.h"
#include "plugin_export.h"

#include "stdio.h" // TODO REMOVE DEBUG

namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT PolynomialExtrapolate_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"Polynomial extrapolate",
	QT_TRANSLATE_NOOP("PluginBrowser", "This effect tries to predict future audio using polynomials"),
	"szeli1",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
} ;

}


PolynomialExtrapolateEffect::PolynomialExtrapolateEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key)
	: Effect(&PolynomialExtrapolate_plugin_descriptor, parent, key)
	, m_effectControls(this)
{
}


Effect::ProcessStatus PolynomialExtrapolateEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	size_t range = m_effectControls.m_rangeModel.value();
	float mixVal = m_effectControls.m_mixModel.value();
	size_t decayVal = static_cast<size_t>(m_effectControls.m_decayModel.value());
	float invMix = 1.0f - mixVal;

	/*
	size_t decayRange = range + 3;
	size_t decayRangeMid = decayRange / 2;
	size_t decayRangeEnd = decayRange - 1;
	m_retainCount = decayVal + decayRange;
	//if (m_inputData.size() - m_retainCount != frames) { m_inputData.resize(frames + m_retainCount); }


	//m_inputData.write(buf, frames);

	constexpr float treshold = 0.002f;
	size_t processDecayCount = mixVal <= treshold ? 1 : static_cast<size_t>(std::log(mixVal) / std::log(invMix) + 1.0f);
	processDecayCount = std::min(decayVal, processDecayCount);

	bool isReverse = m_effectControls.m_isReverseModel.value();
	*/
	/*
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
			for (fpp_t j = processDecayCount - 1; j-- > 0;)
			{
				fpp_t index = i + j;
				buf[i][0] = std::clamp(predictNext(m_inputData[index][0], m_inputData[index + decayRangeMid][0], m_inputData[index + decayRangeEnd][0], 3), -1.0f, 1.0f) * mixVal + buf[i][0] * invMix;
				buf[i][1] = std::clamp(predictNext(m_inputData[index][1], m_inputData[index + decayRangeMid][1], m_inputData[index + decayRangeEnd][1], 3), -1.0f, 1.0f) * mixVal + buf[i][1] * invMix;
			}
		}
	}
	*/


	//! how many samples should we wait before making a prediction
	size_t samplesBetweenPredictions = 1;
	//! how many sample points will be used for the prediction
	size_t width = decayVal;
	//! how many sample gaps should be between the sample points
	size_t gap = range;
	//! how many samples to predict each prediction
	int predictionCount = decayVal * gap;
	float xMultiplier = 1.0f;

	width = std::min(width, MAX_POLYNOMIAL_DEGREE);
	gap = std::min(gap, MAX_SAMPLING_GAP);
	predictionCount = std::min(predictionCount, (int)MAX_PREDICTION_COUNT);
	samplesBetweenPredictions = std::min(samplesBetweenPredictions, MAX_SAMPLES_BETWEEN_PREDICTION);
	if (m_width != width)
	{
		m_width = width;
		// setup matrix so we can calcualte the polynomial coefficients
		std::span<float> matrix{m_polynomialMatrix};
		generateMatrix(matrix, m_width);
	}

	const size_t requiredSize = m_width * gap + predictionCount;
	m_inputData.resize(requiredSize);

	// we have the data
	for (fpp_t j = 0; j < frames; ++j)
	{
		auto pair = m_inputData.swap(std::make_pair(buf[j], SampleFrame{}));
		buf[j] = pair.second * mixVal + pair.first * invMix;

		if (m_retainCounter >= samplesBetweenPredictions)
		{
			m_retainCounter = 0;
			size_t startIndex = predictionCount;

			// do the processing
			makeExtrapolation(startIndex, width, gap, predictionCount, xMultiplier);
		}
		else
		{
			++m_retainCounter;
		}
	}

	return ProcessStatus::ContinueIfNotQuiet;
}


float PolynomialExtrapolateEffect::predictNext(float y1, float y2, float y3, float predictX)
{
	// calculating second degree polynomial with 3 input points
	// y = ax^2 + bx + c
	// x1 = 0, x2 = 1, x3 = 2, output: x4 = `predictX`
	float c = y1;
	float b = 2.0f * y2 - (3.0f * y1 + y3) / 2.0f;
	float a = y2 - y1 - b;
	return predictX * predictX * a + predictX * b + c;
}

template<typename T, size_t maxSize>
T& PolynomialExtrapolateEffect::storageBuffer<T, maxSize>::operator[](size_t index)
{
	return m_data[(m_readIndex + index) % m_size];
}
template<typename T, size_t maxSize>
void PolynomialExtrapolateEffect::storageBuffer<T, maxSize>::clear()
{
	m_data.clear();
	m_readIndex = 0;
}
template<typename T, size_t maxSize>
size_t PolynomialExtrapolateEffect::storageBuffer<T, maxSize>::size()
{
	return m_size;
}
template<typename T, size_t maxSize>
void PolynomialExtrapolateEffect::storageBuffer<T, maxSize>::resize(size_t newSize)
{
	assert(maxSize >= newSize);
	if (newSize != m_size)
	{
		m_size = std::max(newSize, maxSize);
		m_readIndex = m_readIndex % m_size;
	}
}
template<typename T, size_t maxSize>
void PolynomialExtrapolateEffect::storageBuffer<T, maxSize>::write(const T* buf, size_t frames)
{
	if (m_size <= 0 || frames <= 0) { return; }
	for (size_t i = 0; i < frames; i++)
	{
		m_data[m_readIndex] = buf[i];
		m_readIndex = m_readIndex + 1 < m_size ? m_readIndex + 1 : 0;
	}
}
template<typename T, size_t maxSize>
void PolynomialExtrapolateEffect::storageBuffer<T, maxSize>::swap(const T* buf, size_t frames)
{
	if (m_size <= 0 || frames <= 0) { return; }
	for (size_t i = 0; i < frames; i++)
	{
		T temp = m_data[m_readIndex];
		m_data[m_readIndex] = buf[i];
		buf[i] = temp;
		m_readIndex = m_readIndex + 1 < m_size ? m_readIndex + 1 : 0;
	}
}
template<typename T, size_t maxSize>
T PolynomialExtrapolateEffect::storageBuffer<T, maxSize>::swap(T data)
{
	T temp = m_data[m_readIndex];
	m_data[m_readIndex] = data;
	m_readIndex = m_readIndex + 1 < m_size ? m_readIndex + 1 : 0;
	return temp;
}

void PolynomialExtrapolateEffect::makeExtrapolation(size_t startIndex, size_t width,
	size_t gap, int predictionCount, float xMultiplier)
{
	for (size_t channel = 0; channel <= 1; ++channel)
	{
		std::array<float, MAX_POLYNOMIAL_DEGREE> inputSamples;
		std::array<float, MAX_POLYNOMIAL_DEGREE> coefficients; // [0] = c, [1] = b, [2] = a in a*x*x + b*x + c
		for (size_t i = 0; i < width; ++i)
		{
			inputSamples[i] = m_inputData[startIndex + i * gap].first[channel];
		}

		{
			std::span<float> coefficientHelper(coefficients);
			getPolynomialCoefficients(m_polynomialMatrix, inputSamples, coefficientHelper, width);
		}

		//m_inputData[startIndex].second[channel] = m_inputData[startIndex].first[channel];
		for (int i = 1; i <= predictionCount; ++i)
		{
			float predictionAfter = polinomialAt(-i * xMultiplier, coefficients, width);
			float predictionBefore = polinomialAt(i * xMultiplier, coefficients, width);
			float weight = i / static_cast<float>(predictionCount);
			float iWeight = 1.0f - weight;
			weight = 1.0f - (weight * weight);
			iWeight = 1.0f - (iWeight * iWeight);

			//m_inputData[startIndex - i].first[channel] = predictionAfter * weight;
			//m_inputData[startIndex + i].first[channel] = predictionBefore * weight;

			//m_inputData[startIndex - i].second[channel] = predictionBefore * weight;
			m_inputData[startIndex + i].second[channel] = predictionAfter * weight;
		}
	}
}

void PolynomialExtrapolateEffect::getPolynomialCoefficients(const std::span<float>& helperMatrix,
	const std::span<float>& samples, std::span<float>& polinomial, size_t width)
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

void PolynomialExtrapolateEffect::generateMatrix(std::span<float>& matrix, size_t sizeTWidth)
{
	std::array<float, MAX_POLYNOMIAL_DEGREE * MAX_POLYNOMIAL_DEGREE> coefficientMatrix;
	// x coordinates correspond to indexes in the source audio
	// a polinomial consist of increasing whole powers of these x coords:
	// "a*x^2 + b*x + c"
	// w = 3    c  b  a   f(1) f(2) f(3)    ---->    c  b  a      1 <= t <= w [matrix] OUT
	// x = 1 [  1  1  1  |  1    0    0  ]        [  1  0  0  | linear combination of f(t) ]
	// x = 2 [  1  2  4  |  0    1    0  ]        [  0  1  0  | linear combination of f(t) ]
	// x = 3 [  1  3  9  |  0    0    1  ]        [  0  0  1  | linear combination of f(t) ]
	for (size_t i = 0; i < sizeTWidth; ++i)
	{
		float poweredInput{1.0f};
		for (size_t j = 0; j < sizeTWidth; ++j)
		{
			size_t index = i * sizeTWidth + j;
			coefficientMatrix[index] = poweredInput;
			poweredInput = poweredInput * (i + 1);
		}
	}
	printf("init 1.:\n");
	printMatrixDebug(coefficientMatrix, sizeTWidth);
	for (size_t i = 0; i < sizeTWidth; ++i)
	{
		for (size_t j = 0; j < sizeTWidth; ++j)
		{
			matrix[i * sizeTWidth + j] = 0.0f;
		}
		matrix[i * sizeTWidth + i] = 1.0f;
	}
	printf("init 2.:\n");
	printMatrixDebug(matrix, sizeTWidth);

	
	// Gauss elimination
	int x = 0;
	int y = 0;
	int width{static_cast<int>(sizeTWidth)};
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
						printf("swap (%d) row with (%d)\n", y, ny);
						printMatrixDebug(coefficientMatrix, sizeTWidth);
						printf("MIRROR:\n");
						printMatrixDebug(matrix, sizeTWidth);

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
						printMatrixDebug(coefficientMatrix, sizeTWidth);
						printf("MIRROR:\n");
						printMatrixDebug(matrix, sizeTWidth);

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
		printf("leading 1 identified at (%d, %d)\n", x, y);
		
		float quotient = 1.0f / coefficientMatrix[y * width + x];
		coefficientMatrix[y * width + x] = 1.0f;
		for (int nx = x + 1; nx < width; ++nx)
		{
			coefficientMatrix[y * width + nx] *= quotient;
		}
		// mirror
		for (int nx = 0; nx < width; ++nx)
		{
			matrix[y * width + nx] *= quotient;
		}
		printf("(%d) row multiplied by %f\n", y, quotient);
		printMatrixDebug(coefficientMatrix, sizeTWidth);
		printf("MIRROR:\n");
		printMatrixDebug(matrix, sizeTWidth);

		for (int ny = y + 1; ny < width; ++ny)
		{
			float multiplier = -coefficientMatrix[ny * width + x];
			coefficientMatrix[ny * width + x] = 0.0f;
			for (int nx = x + 1; nx < width; ++nx)
			{
				coefficientMatrix[ny * width + nx] += coefficientMatrix[y * width + nx] * multiplier;
			}
			// mirror
			for (int nx = 0; nx < width; ++nx)
			{
				matrix[ny * width + nx] += matrix[y * width + nx] * multiplier;
			}

			printf("substracted (%d) row from (%d)\n", y, ny);
			printMatrixDebug(coefficientMatrix, sizeTWidth);
			printf("MIRROR:\n");
			printMatrixDebug(matrix, sizeTWidth);
		}
		++x;
		if (x >= width)
		{
			// if the algorithm is finished correctly, it will break at x == width any y == width - 1
			++y; // we assert(y == width) but currently y = width - 1 and we break early, so y is increased
			break;
		}
	}
	printf("x = %d, y = %d\n", x, y);
	assert(x == width); // Gauss elimination couldn't finish
	assert(y == width); // Gauss elimination couldn't finish
	--x;
	--y;
	for (; y >= 0; --y)
	{
		// finding leading 1
		if (std::abs(coefficientMatrix[y * width + x] - 1.0f) > 0.0001f)
		{
			while (--x >= 0)
			{
				// if we found the leading 1
				if (std::abs(coefficientMatrix[y * width + x] - 1.0f) > 0.0001f) { break; }
			}

			if (x < 0) { assert(false); /* this should never happen */ }
		}
		printf("leading 1 identified at (%d, %d)\n", x, y);
		
		for (int ny = y - 1; ny >= 0; --ny)
		{
			float multiplier = -coefficientMatrix[ny * width + x];
			coefficientMatrix[ny * width + x] = 0.0f;
			for (int nx = 0; nx < width; ++nx)
			{
				// mirror
				matrix[ny * width + nx] += matrix[y * width + nx] * multiplier;
			}
			printf("substracted (%d) row from (%d)\n", y, ny);
		}
		--x;
		if (x < 0) { break; }
	}

	assert(x < 0); // Gauss elimination couldn't finish
	assert(y <= 0); // Gauss elimination couldn't finish

	printf("final matrix:\n");
	printMatrixDebug(matrix, sizeTWidth);
}

float PolynomialExtrapolateEffect::polinomialAt(float x, const std::span<float>& polinomial, size_t width) const
{
	float factor = polinomial[width - 1];
	for (int i = width - 2; i >= 0; --i)
	{
		factor = factor * x + polinomial[i];
	}
	return factor;
}

void PolynomialExtrapolateEffect::printMatrixDebug(const std::span<float>& matrix, size_t width)
{
	printf("matrix %ld x %ld\n  ", width, width);

	for (size_t x = 0; x < width; ++x)
	{
		printf("%f ", static_cast<float>(x));
	}
	printf("\n");
	for (size_t y = 0; y < width; ++y)
	{
		printf("[ ");
		for (size_t x = 0; x < width; ++x)
		{
			printf("%f ", matrix[y * width + x]);
		}
		printf(" ]\n");
	}
}

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	return new PolynomialExtrapolateEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}

}

} // namespace lmms
