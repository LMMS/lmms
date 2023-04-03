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
	QT_TRANSLATE_NOOP("PluginBrowser", "An allpass filter allowing for extremely high orders."),
	"Lost Robot <r94231/at/gmail/dot/com>",
	0x0100,
	Plugin::Effect,
	new PluginPixmapLoader("logo"),
	nullptr,
	nullptr,
};

}


DispersionEffect::DispersionEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&dispersion_plugin_descriptor, parent, key),
	m_dispersionControls(this),
	m_sampleRate(Engine::audioEngine()->processingSampleRate()),
	m_amountVal(0)
{
}


bool DispersionEffect::processAudioBuffer(sampleFrame* buf, const fpp_t frames)
{
	if (!isEnabled() || !isRunning())
	{
		return false;
	}

	double outSum = 0.0;
	const float d = dryLevel();
	const float w = wetLevel();
	
	const int amount = m_dispersionControls.m_amountModel.value();
	const float freq = m_dispersionControls.m_freqModel.value();
	const float reso = m_dispersionControls.m_resoModel.value();
	float feedback = m_dispersionControls.m_feedbackModel.value();
	const bool dc = m_dispersionControls.m_dcModel.value();
	
	// Allpass coefficient calculation
	const float w0 = (F_2PI / m_sampleRate) * freq;
	const float a0 = 1 + (std::sin(w0) / (reso * 2.f));
	float apCoeff1 = (1 - (a0 - 1)) / a0;
	float apCoeff2 = (-2 * std::cos(w0)) / a0;
	
	float dcCoeff = 0.001 * (44100.f / m_sampleRate);
	
	if (amount != m_amountVal)
	{
		if (amount < m_amountVal)
		{
			// Flush filter buffers when they're no longer in use
			for (int i = amount; i < m_amountVal; ++i)
			{
				m_apX0[i][0] = m_apX0[i][1] = m_apX1[i][0] = m_apX1[i][1] = 0;
				m_apY0[i][0] = m_apY0[i][1] = m_apY1[i][0] = m_apY1[i][1] = 0;
			}
		}
		m_amountVal = amount;
	}
	
	if (amount == 0)
	{
		feedback = 0;
	}

	for (fpp_t f = 0; f < frames; ++f)
	{
		sample_t s[2] = { buf[f][0] + m_feedbackVal[0], buf[f][1] + m_feedbackVal[1] };
		
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
		outSum += buf[f][0] * buf[f][0] + buf[f][1] * buf[f][1];
	}

	checkGate(outSum / frames);
	return isRunning();
}


void DispersionEffect::runDispersionAP(int filtNum, float apCoeff1, float apCoeff2, sample_t* put)
{
	for (int i = 0; i < 2; ++i)
	{
		sample_t filterOutput = put[i];
		for (int j = 0; j < filtNum; ++j)
		{
			const sample_t currentInput = filterOutput;
			filterOutput = apCoeff1 * (currentInput - m_apY1[j][i]) +
				apCoeff2 * (m_apX0[j][i] - m_apY0[j][i]) +
				m_apX1[j][i];
			
			m_apX1[j][i] = m_apX0[j][i];
			m_apX0[j][i] = currentInput;
			m_apY1[j][i] = m_apY0[j][i];
			m_apY0[j][i] = filterOutput;
		}
		put[i] = filterOutput;
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
