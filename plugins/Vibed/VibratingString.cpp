/*
 * VibratingString.cpp - model of a vibrating string lifted from pluckedSynth
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo/com>
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

#include "VibratingString.h"
#include "interpolation.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "LmmsTypes.h"

#include <algorithm>
#include <cstdlib>

namespace lmms
{


VibratingString::VibratingString(float pitch, float pick, float pickup, const float* impulse, int len,
	sample_rate_t sampleRate, int oversample, float randomize, float stringLoss, float detune, bool state) :
	m_oversample{2 * oversample / static_cast<int>(sampleRate / Engine::audioEngine()->baseSampleRate())},
	m_randomize{randomize},
	m_stringLoss{1.0f - stringLoss},
	m_choice{static_cast<int>(m_oversample * static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX))},
	m_state{0.1f},
	m_outsamp{std::make_unique<sample_t[]>(m_oversample)}
{
	int stringLength = static_cast<int>(m_oversample * sampleRate / pitch) + 1;
	stringLength += static_cast<int>(stringLength * -detune);

	const int pickInt = static_cast<int>(std::ceil(stringLength * pick));

	if (!state)
	{
		m_impulse = std::make_unique<float[]>(stringLength);
		resample(impulse, len, stringLength);
	}
	else
	{
		m_impulse = std::make_unique<float[]>(len);
		std::copy_n(impulse, len, m_impulse.get());
	}

	m_toBridge = VibratingString::initDelayLine(stringLength);
	m_fromBridge = VibratingString::initDelayLine(stringLength);

	VibratingString::setDelayLine(m_toBridge.get(), pickInt, m_impulse.get(), len, 0.5f, state);
	VibratingString::setDelayLine(m_fromBridge.get(), pickInt, m_impulse.get(), len, 0.5f, state);

	m_pickupLoc = static_cast<int>(pickup * stringLength);
}

std::unique_ptr<VibratingString::DelayLine> VibratingString::initDelayLine(int len)
{
	auto dl = std::make_unique<VibratingString::DelayLine>();
	dl->length = len;
	if (len > 0)
	{
		dl->data = std::make_unique<sample_t[]>(len);
		for (int i = 0; i < dl->length; ++i)
		{
			float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
			float offset = (m_randomize / 2.0f - m_randomize) * r;
			dl->data[i] = offset;
		}
	}
	else
	{
		dl->data = nullptr;
	}

	dl->pointer = dl->data.get();
	dl->end = dl->data.get() + len - 1;

	return dl;
}

void VibratingString::resample(const float* src, f_cnt_t srcFrames, f_cnt_t dstFrames)
{
	for (f_cnt_t frame = 0; frame < dstFrames; ++frame)
	{
		const float srcFrameFloat = frame * static_cast<float>(srcFrames) / dstFrames;
		const float fracPos = srcFrameFloat - static_cast<f_cnt_t>(srcFrameFloat);
		const f_cnt_t srcFrame = std::clamp(static_cast<f_cnt_t>(srcFrameFloat), f_cnt_t{1}, srcFrames - 3);
		m_impulse[frame] = cubicInterpolate(
			src[srcFrame - 1],
			src[srcFrame + 0],
			src[srcFrame + 1],
			src[srcFrame + 2],
			fracPos);
	}
}


} // namespace lmms
