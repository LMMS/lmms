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

extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data)
{
	return new PredictionDistortEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}

}

} // namespace lmms
