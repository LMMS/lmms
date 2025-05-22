/*
 * PredictionDistort.h - PredictionDistort-effect-plugin
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

#ifndef LMMS_PREDICTIONDISTORT_H
#define LMMS_PREDICTIONDISTORT_H

#include "Effect.h"
#include "PredictionDistortControls.h"

#include <cmath>
#include <vector>

namespace lmms
{

class PredictionDistortEffect : public Effect
{
public:
	PredictionDistortEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~PredictionDistortEffect() override = default;

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

	//! @param y1: the sample at relative index [0]
	//! @param y2: the sample at relative index [1]
	//! @param y3: the sample at relative index [2]
	//! @param predictX: prediction x, starts from relative index [0], use 3.0f if you want to predict the [3] sample
	static float predictNext(float y1, float y2, float y3, float predictX);

	EffectControls* controls() override
	{
		return &m_effectControls;
	}

private:
	template<typename T>
	class storageBuffer
	{
	public:
		storageBuffer() = default;
		T& operator [](size_t index);
		void clear();
		size_t size();
		void resize(size_t newSize);
		void write(const T* buf, size_t frames);
	private:
		size_t m_readIndex = 0;
		std::vector<T> m_data = {};
	};

	PredictionDistortControls m_effectControls;
	storageBuffer<SampleFrame> m_inputData;
	//! how much data to store in `m_inputData` from the last input
	size_t m_retainCount = 3;

	friend class PredictionDistortControls;
};

} // namespace lmms

#endif // LMMS_PREDICTIONDISTORT_H
