/*
 * PolynomialExtrapolate.h - PolynomialExtrapolate-effect-plugin
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

#ifndef LMMS_POLYNOMIALEXTRAPOLATE_H
#define LMMS_POLYNOMIALEXTRAPOLATE_H

#include "Effect.h"
#include "PolynomialExtrapolateControls.h"

#include <cmath>
#include <vector>

namespace lmms
{

class PolynomialExtrapolateEffect : public Effect
{
public:
	PolynomialExtrapolateEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~PolynomialExtrapolateEffect() override = default;

	static constexpr size_t MAX_POLYNOMIAL_DEGREE = 8;
	static constexpr size_t MAX_SAMPLING_GAP = 500;
	static constexpr size_t MAX_PREDICTION_COUNT = 20;
	static constexpr size_t MAX_SAMPLES_BETWEEN_PREDICTION = 1;
	static constexpr size_t MAX_BUFFER_SIZE = MAX_POLYNOMIAL_DEGREE * MAX_SAMPLING_GAP + MAX_PREDICTION_COUNT + MAX_SAMPLES_BETWEEN_PREDICTION;

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

	EffectControls* controls() override
	{
		return &m_effectControls;
	}

private:
	template<typename T, size_t maxSize>
	class storageBuffer
	{
	public:
		storageBuffer() = default;
		T& operator [](size_t index);
		void clear();
		size_t size();
		void resize(size_t newSize);
		// pushes front `data`, returns replaced back
		T swap(T data);
	private:
		size_t m_readIndex = 0;
		size_t m_size = maxSize;
		std::array<T, maxSize> m_data = {};
	};

	void processNewFrames(SampleFrame* buf, const fpp_t frames,
		size_t sampesBetweenPredicions, size_t width, size_t gap,
		int predictionCount, float xMultiplier);

	void getPolynomialCoefficients(const std::span<float>& helperMatrix,
		const std::span<float>& samples, std::span<float>& polinomial, size_t width);
	void generateMatrix(std::span<float>& matrix, size_t width);
	float polinomialAt(float x, const std::span<float>& polinomial, size_t width) const;

	//! @param startIdnex the index before the extraploation
	//! @param width the count of combined polynomials, degree + 1
	//! @param gap how many samples to skip while generatin the coefficients
	//! @param predictionCount how many samples to extrapolate from startIndex
	//! @param xMultiplier changes the sampling positions for the extrapolations: x * xMultiplier
	//! @param feedback mixes the extrapolation with the input samples, 0 -> 0% extrapolation, 100% input
	//! @param start changes the sampling positions fot the extrapolations: x + start
	void makeExtrapolation(size_t startIndex, size_t width,
		size_t gap, int predictionCount, float xMultiplier, float feedback, float start);

	void printMatrixDebug(const std::span<float>& matrix, size_t width);

	PolynomialExtrapolateControls m_effectControls;
	//! first: input data and feedback
	//! output data
	storageBuffer<std::pair<SampleFrame, SampleFrame>, MAX_BUFFER_SIZE> m_inputData;
	//! how much data to store in `m_inputData` from the last input
	size_t m_retainCount = 3;
	size_t m_retainCounter = 0;

	//! matrix width, equivalent with polynomial degree
	size_t m_width = 0;
	//! helps generate polynomial coefficients
	std::array<float, MAX_POLYNOMIAL_DEGREE * MAX_POLYNOMIAL_DEGREE>
		m_polynomialMatrix;

	friend class PolynomialExtrapolateControls;
};

} // namespace lmms

#endif // LMMS_POLYNOMIALEXTRAPOLATE_H
