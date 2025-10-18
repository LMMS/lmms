/*
 * Dispersion.cpp
 *
 * Copyright (c) 2023 Lost Robot <r94231/at/gmail/dot/com>
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

#include "Dispersion.h"

#include <numbers>

#include "embed.h"
#include "plugin_export.h"

namespace lmms
{

extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT dispersion_plugin_descriptor =
{
	LMMS_STRINGIFY(PLUGIN_NAME),
	"Dispersion",
	QT_TRANSLATE_NOOP("PluginBrowser", "An all-pass filter allowing for extremely high orders."),
	"Lost Robot <r94231/at/gmail/dot/com>",
	0x0100,
	Plugin::Type::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
	nullptr,
};

}


DispersionEffect::DispersionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&dispersion_plugin_descriptor, parent, key),
	m_dispersionControls(this),
	m_sampleRate(Engine::audioEngine()->outputSampleRate()),
	m_amountVal(0)
{
}


Effect::ProcessStatus DispersionEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	const float d = dryLevel();
	const float w = wetLevel();
	
	const int amount = m_dispersionControls.m_amountModel.value();
	const float freq = m_dispersionControls.m_freqModel.value();
	const float reso = m_dispersionControls.m_resoModel.value();
	float feedback = m_dispersionControls.m_feedbackModel.value();
	const bool dc = m_dispersionControls.m_dcModel.value();
	
	// All-pass coefficient calculation
	const float w0 = (2 * std::numbers::pi_v<float> / m_sampleRate) * freq;
	const float a0 = 1 + (std::sin(w0) / (reso * 2.f));
	float apCoeff1 = (1 - (a0 - 1)) / a0;
	float apCoeff2 = (-2 * std::cos(w0)) / a0;
	
	float dcCoeff = 0.001 * (44100.f / m_sampleRate);
	
	if (amount != m_amountVal)
	{
		if (amount < m_amountVal)
		{
			// Flush filter buffers when they're no longer in use
			for (int i = amount * 2; i < m_amountVal * 2; ++i)
			{
				m_state.x0[i] = m_state.x1[i] = m_state.y0[i] = m_state.y1[i] = 0;
			}
		}
		m_amountVal = amount;
	}
	
	if (amount == 0)
	{
		feedback = 0;
		m_feedbackVal[0] = m_feedbackVal[1] = 0;
	}

	for (fpp_t f = 0; f < frames; ++f)
	{
		std::array<sample_t, 2> s = { buf[f][0] + m_feedbackVal[0], buf[f][1] + m_feedbackVal[1] };
		
		runDispersionAP(m_amountVal, apCoeff1, apCoeff2, s);
		m_feedbackVal[0] = s[0] * feedback;
		m_feedbackVal[1] = s[1] * feedback;
		
		if (dc)
		{
			// DC offset removal
			for (int i = 0; i < 2; ++i)
			{
				m_integrator[i] = m_integrator[i] * (1.f - dcCoeff) + s[i] * dcCoeff;
				s[i] -= m_integrator[i];
			}
		}

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
	}

	return ProcessStatus::ContinueIfNotQuiet;
}


void DispersionEffect::runDispersionAP(const int filtNum, const float apCoeff1, const float apCoeff2, std::array<sample_t, 2> &put)
{
	for (int i = 0; i < filtNum * 2; ++i)
	{
		const int channel = i % 2;
		const sample_t currentInput = put[channel];
		const sample_t filterOutput = apCoeff1 * (currentInput - m_state.y1[i])
			+ apCoeff2 * (m_state.x0[i] - m_state.y0[i]) + m_state.x1[i];
		m_state.x1[i] = m_state.x0[i];
		m_state.x0[i] = currentInput;
		m_state.y1[i] = m_state.y0[i];
		m_state.y0[i] = filterOutput;

		put[channel] = filterOutput;
	}
}


extern "C"
{

// necessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin * lmms_plugin_main(Model* parent, void* data)
{
	return new DispersionEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
}

}

} // namespace lmms
