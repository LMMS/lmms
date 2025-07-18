/*
 * LOMM.cpp
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

#include "LOMM.h"

#include "lmms_math.h"
#include "embed.h"
#include "plugin_export.h"

namespace lmms
{

extern "C"
{
	Plugin::Descriptor PLUGIN_EXPORT lomm_plugin_descriptor =
	{
		LMMS_STRINGIFY(PLUGIN_NAME),
		"LOMM",
		QT_TRANSLATE_NOOP("PluginBrowser", "Upwards/downwards multiband compression plugin powered by the eldritch elder god LOMMUS."),
		"Lost Robot <r94231/at/gmail/dot/com>",
		0x0100,
		Plugin::Type::Effect,
		new PluginPixmapLoader("logo"),
		nullptr,
		nullptr,
		nullptr,
	};
}


LOMMEffect::LOMMEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&lomm_plugin_descriptor, parent, key),
	m_lommControls(this),
	m_sampleRate(Engine::audioEngine()->outputSampleRate()),
	m_lp1(m_sampleRate),
	m_lp2(m_sampleRate),
	m_hp1(m_sampleRate),
	m_hp2(m_sampleRate),
	m_ap(m_sampleRate),
	m_needsUpdate(true),
	m_coeffPrecalc(-0.05f),
	m_crestTimeConst(0.999f),
	m_lookWrite(0),
	m_lookBufLength(2)
{
	autoQuitModel()->setValue(autoQuitModel()->maxValue());
	
	m_ap.setFilterType(BasicFilters<2>::FilterType::AllPass);
	
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(changeSampleRate()));
	changeSampleRate();
}

void LOMMEffect::changeSampleRate()
{
	m_sampleRate = Engine::audioEngine()->outputSampleRate();
	m_lp1.setSampleRate(m_sampleRate);
	m_lp2.setSampleRate(m_sampleRate);
	m_hp1.setSampleRate(m_sampleRate);
	m_hp2.setSampleRate(m_sampleRate);
	m_ap.setSampleRate(m_sampleRate);
	
	m_coeffPrecalc = -2.2f / (m_sampleRate * 0.001f);
	m_needsUpdate = true;
	
	m_crestTimeConst = std::exp(-1.f / (0.2f * m_sampleRate));
	
	m_lookBufLength = std::ceil((LOMM_MAX_LOOKAHEAD / 1000.f) * m_sampleRate) + 2;
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			m_inLookBuf[j][i].resize(m_lookBufLength);
			m_scLookBuf[j][i].resize(m_lookBufLength, LOMM_MIN_FLOOR);
		}
	}

	std::fill(m_yL.begin(), m_yL.end(), std::array<float, 2>{LOMM_MIN_FLOOR, LOMM_MIN_FLOOR});
	m_rms = m_gainResult = m_displayIn = m_displayOut = m_prevOut = m_yL;
	m_crestPeakVal[0] = m_crestPeakVal[1] = LOMM_MIN_FLOOR;
	m_crestRmsVal = m_crestFactorVal = m_crestPeakVal;
}


Effect::ProcessStatus LOMMEffect::processImpl(SampleFrame* buf, const fpp_t frames)
{
	if (m_needsUpdate || m_lommControls.m_split1Model.isValueChanged())
	{
		m_lp1.setLowpass(m_lommControls.m_split1Model.value());
		m_hp1.setHighpass(m_lommControls.m_split1Model.value());
		m_ap.calcFilterCoeffs(m_lommControls.m_split1Model.value(), 0.70710678118f);
	}
	if (m_needsUpdate || m_lommControls.m_split2Model.isValueChanged())
	{
		m_lp2.setLowpass(m_lommControls.m_split2Model.value());
		m_hp2.setHighpass(m_lommControls.m_split2Model.value());
	}
	m_needsUpdate = false;

	const float d = dryLevel();
	const float w = wetLevel();
	
	const float depth = m_lommControls.m_depthModel.value();
	const float time = m_lommControls.m_timeModel.value();
	const float inVol = dbfsToAmp(m_lommControls.m_inVolModel.value());
	const float outVol = dbfsToAmp(m_lommControls.m_outVolModel.value());
	const float upward = m_lommControls.m_upwardModel.value();
	const float downward = m_lommControls.m_downwardModel.value();
	const bool split1Enabled = m_lommControls.m_split1EnabledModel.value();
	const bool split2Enabled = m_lommControls.m_split2EnabledModel.value();
	const bool band1Enabled = m_lommControls.m_band1EnabledModel.value();
	const bool band2Enabled = m_lommControls.m_band2EnabledModel.value();
	const bool band3Enabled = m_lommControls.m_band3EnabledModel.value();
	const float inHigh = dbfsToAmp(m_lommControls.m_inHighModel.value());
	const float inMid = dbfsToAmp(m_lommControls.m_inMidModel.value());
	const float inLow = dbfsToAmp(m_lommControls.m_inLowModel.value());
	float inBandVol[3] = {inHigh, inMid, inLow};
	const float outHigh = dbfsToAmp(m_lommControls.m_outHighModel.value());
	const float outMid = dbfsToAmp(m_lommControls.m_outMidModel.value());
	const float outLow = dbfsToAmp(m_lommControls.m_outLowModel.value());
	float outBandVol[3] = {outHigh, outMid, outLow};
	const float aThreshH = m_lommControls.m_aThreshHModel.value();
	const float aThreshM = m_lommControls.m_aThreshMModel.value();
	const float aThreshL = m_lommControls.m_aThreshLModel.value();
	float aThresh[3] = {aThreshH, aThreshM, aThreshL};
	const float aRatioH = m_lommControls.m_aRatioHModel.value();
	const float aRatioM = m_lommControls.m_aRatioMModel.value();
	const float aRatioL = m_lommControls.m_aRatioLModel.value();
	float aRatio[3] = {1.f / aRatioH, 1.f / aRatioM, 1.f / aRatioL};
	const float bThreshH = m_lommControls.m_bThreshHModel.value();
	const float bThreshM = m_lommControls.m_bThreshMModel.value();
	const float bThreshL = m_lommControls.m_bThreshLModel.value();
	float bThresh[3] = {bThreshH, bThreshM, bThreshL};
	const float bRatioH = m_lommControls.m_bRatioHModel.value();
	const float bRatioM = m_lommControls.m_bRatioMModel.value();
	const float bRatioL = m_lommControls.m_bRatioLModel.value();
	float bRatio[3] = {1.f / bRatioH, 1.f / bRatioM, 1.f / bRatioL};
	const float atkH = m_lommControls.m_atkHModel.value() * time;
	const float atkM = m_lommControls.m_atkMModel.value() * time;
	const float atkL = m_lommControls.m_atkLModel.value() * time;
	const float atkCoefH = msToCoeff(atkH);
	const float atkCoefM = msToCoeff(atkM);
	const float atkCoefL = msToCoeff(atkL);
	float atk[3] = {atkH, atkM, atkL};
	float atkCoef[3] = {atkCoefH, atkCoefM, atkCoefL};
	const float relH = m_lommControls.m_relHModel.value() * time;
	const float relM = m_lommControls.m_relMModel.value() * time;
	const float relL = m_lommControls.m_relLModel.value() * time;
	const float relCoefH = msToCoeff(relH);
	const float relCoefM = msToCoeff(relM);
	const float relCoefL = msToCoeff(relL);
	float rel[3] = {relH, relM, relL};
	float relCoef[3] = {relCoefH, relCoefM, relCoefL};
	const float rmsTime = m_lommControls.m_rmsTimeModel.value();
	const float rmsTimeConst = (rmsTime == 0) ? 0 : std::exp(-1.f / (rmsTime * 0.001f * m_sampleRate));
	const float knee = m_lommControls.m_kneeModel.value() * 0.5f;
	const float range = m_lommControls.m_rangeModel.value();
	const float rangeAmp = dbfsToAmp(range);
	const float balance = m_lommControls.m_balanceModel.value();
	const float balanceAmpTemp = dbfsToAmp(balance);
	const float balanceAmp[2] = {1.f / balanceAmpTemp, balanceAmpTemp};
	const bool depthScaling = m_lommControls.m_depthScalingModel.value();
	const bool stereoLink = m_lommControls.m_stereoLinkModel.value();
	const float autoTime = m_lommControls.m_autoTimeModel.value() * m_lommControls.m_autoTimeModel.value();
	const float mix = m_lommControls.m_mixModel.value();
	const bool midside = m_lommControls.m_midsideModel.value();
	const bool lookaheadEnable = m_lommControls.m_lookaheadEnableModel.value();
	const int lookahead = std::ceil((m_lommControls.m_lookaheadModel.value() / 1000.f) * m_sampleRate);
	const bool feedback = m_lommControls.m_feedbackModel.value() && !lookaheadEnable;
	const bool lowSideUpwardSuppress = m_lommControls.m_lowSideUpwardSuppressModel.value() && midside;
	
	for (fpp_t f = 0; f < frames; ++f)
	{
		std::array<sample_t, 2> s = {buf[f][0], buf[f][1]};
		
		// Convert left/right to mid/side.  Side channel is intentionally made
		// to be 6 dB louder to bring it into volume ranges comparable to the mid channel.
		if (midside)
		{
			float tempS0 = s[0];
			s[0] = (s[0] + s[1]) * 0.5f;
			s[1] = tempS0 - s[1];
		}
		
		std::array<std::array<float, 2>, 3> bands = {{}};
		std::array<std::array<float, 2>, 3> bandsDry = {{}};
		
		for (int i = 0; i < 2; ++i)// Channels
		{
			// These values are for the Auto time knob.  Higher crest factor allows for faster attack/release.
			float inSquared = s[i] * s[i];
			m_crestPeakVal[i] = std::max(std::max(LOMM_MIN_FLOOR, inSquared), m_crestTimeConst * m_crestPeakVal[i] + (1 - m_crestTimeConst) * (inSquared));
			m_crestRmsVal[i] = std::max(LOMM_MIN_FLOOR, m_crestTimeConst * m_crestRmsVal[i] + ((1 - m_crestTimeConst) * (inSquared)));
			m_crestFactorVal[i] = m_crestPeakVal[i] / m_crestRmsVal[i];
			float crestFactorValTemp = ((m_crestFactorVal[i] - LOMM_AUTO_TIME_ADJUST) * autoTime) + LOMM_AUTO_TIME_ADJUST;
		
			// Crossover filters
			bands[2][i] = m_lp2.update(s[i], i);
			bands[1][i] = m_hp2.update(s[i], i);
			bands[0][i] = m_hp1.update(bands[1][i], i);
			bands[1][i] = m_lp1.update(bands[1][i], i);
			bands[2][i] = m_ap.update(bands[2][i], i);
			
			if (!split1Enabled)
			{
				bands[1][i] += bands[0][i];
				bands[0][i] = 0;
			}
			if (!split2Enabled)
			{
				bands[1][i] += bands[2][i];
				bands[2][i] = 0;
			}
			
			// Mute disabled bands
			bands[0][i] *= band1Enabled;
			bands[1][i] *= band2Enabled;
			bands[2][i] *= band3Enabled;
			
			std::array<float, 3> detect = {0, 0, 0};
			for (int j = 0; j < 3; ++j)// Bands
			{
				bandsDry[j][i] = bands[j][i];
				
				if (feedback && !lookaheadEnable)
				{
					bands[j][i] = m_prevOut[j][i];
				}
				
				bands[j][i] *= inBandVol[j] * inVol * balanceAmp[i];
				
				if (rmsTime > 0)// RMS
				{
					m_rms[j][i] = rmsTimeConst * m_rms[j][i] + ((1 - rmsTimeConst) * (bands[j][i] * bands[j][i]));
					detect[j] = std::max(LOMM_MIN_FLOOR, std::sqrt(m_rms[j][i]));
				}
				else// Peak
				{
					detect[j] = std::max(LOMM_MIN_FLOOR, std::abs(bands[j][i]));
				}
				
				if (detect[j] > m_yL[j][i])// Attack phase
				{
					// Calculate attack value depending on crest factor
					const float currentAttack = autoTime
						? msToCoeff(LOMM_AUTO_TIME_ADJUST * atk[j] / crestFactorValTemp)
						: atkCoef[j];
					
					m_yL[j][i] = m_yL[j][i] * currentAttack + (1 - currentAttack) * detect[j];
				}
				else// Release phase
				{
					// Calculate release value depending on crest factor
					const float currentRelease = autoTime
						? msToCoeff(LOMM_AUTO_TIME_ADJUST * rel[j] / crestFactorValTemp)
						: relCoef[j];
					
					m_yL[j][i] = m_yL[j][i] * currentRelease + (1 - currentRelease) * detect[j];
				}
				
				m_yL[j][i] = std::max(LOMM_MIN_FLOOR, m_yL[j][i]);
				
				float yAmp = m_yL[j][i];
				if (lookaheadEnable)
				{
					float temp = yAmp;
					// Lookahead is calculated by picking the largest value between
					// the current sidechain signal and the delayed sidechain signal.
					yAmp = std::max(m_scLookBuf[j][i][m_lookWrite], m_scLookBuf[j][i][(m_lookWrite + m_lookBufLength - lookahead) % m_lookBufLength]);
					m_scLookBuf[j][i][m_lookWrite] = temp;
				}
				
				const float yDbfs = ampToDbfs(yAmp);
				
				float aboveGain = 0;
				float belowGain = 0;
				
				// Downward compression
				if (yDbfs - aThresh[j] < -knee)// Below knee
				{
					aboveGain = yDbfs;
				}
				else if (yDbfs - aThresh[j] < knee)// Within knee
				{
					const float temp = yDbfs - aThresh[j] + knee;
					aboveGain = yDbfs + (aRatio[j] - 1) * temp * temp / (4 * knee);
				}
				else// Above knee
				{
					aboveGain = aThresh[j] + (yDbfs - aThresh[j]) * aRatio[j];
				}
				if (aboveGain < yDbfs)
				{
					if (downward * depth <= 1)
					{
						aboveGain = std::lerp(yDbfs, aboveGain, downward * depth);
					}
					else
					{
						aboveGain = std::lerp(aboveGain, aThresh[j], downward * depth - 1);
					}
				}
				
				// Upward compression
				if (yDbfs - bThresh[j] > knee)// Above knee
				{
					belowGain = yDbfs;
				}
				else if (bThresh[j] - yDbfs < knee)// Within knee
				{
					const float temp = bThresh[j] - yDbfs + knee;
					belowGain = yDbfs + (1 - bRatio[j]) * temp * temp / (4 * knee);
				}
				else// Below knee
				{
					belowGain = bThresh[j] + (yDbfs - bThresh[j]) * bRatio[j];
				}
				if (belowGain > yDbfs)
				{
					if (upward * depth <= 1)
					{
						belowGain = std::lerp(yDbfs, belowGain, upward * depth);
					}
					else
					{
						belowGain = std::lerp(belowGain, bThresh[j], upward * depth - 1);
					}
				}
				
				m_displayIn[j][i] = yDbfs;
				m_gainResult[j][i] = (dbfsToAmp(aboveGain) / yAmp) * (dbfsToAmp(belowGain) / yAmp);
				if (lowSideUpwardSuppress && m_gainResult[j][i] > 1 && j == 2 && i == 1) //undo upward compression if low side band
				{
					m_gainResult[j][i] = 1;
				}
				m_gainResult[j][i] = std::min(m_gainResult[j][i], rangeAmp);
				m_displayOut[j][i] = ampToDbfs(std::max(LOMM_MIN_FLOOR, yAmp * m_gainResult[j][i]));
				
				// Apply the same gain reduction to both channels if stereo link is enabled.
				if (stereoLink && i == 1)
				{
					if (m_gainResult[j][1] < m_gainResult[j][0])
					{
						m_gainResult[j][0] = m_gainResult[j][1];
						m_displayOut[j][0] = m_displayIn[j][0] - (m_displayIn[j][1] - m_displayOut[j][1]);
					}
					else
					{
						m_gainResult[j][1] = m_gainResult[j][0];
						m_displayOut[j][1] = m_displayIn[j][1] - (m_displayIn[j][0] - m_displayOut[j][0]);
					}
				}
			}
		}
		
		for (int i = 0; i < 2; ++i)// Channels
		{
			for (int j = 0; j < 3; ++j)// Bands
			{
				if (lookaheadEnable)
				{
					float temp = bands[j][i];
					bands[j][i] = m_inLookBuf[j][i][m_lookWrite];
					m_inLookBuf[j][i][m_lookWrite] = temp;
					bandsDry[j][i] = bands[j][i];
				}
				else if (feedback)
				{
					bands[j][i] = bandsDry[j][i] * inBandVol[j] * inVol * balanceAmp[i];
				}
			
				// Apply gain reduction
				bands[j][i] *= m_gainResult[j][i];
				
				// Store for Feedback
				m_prevOut[j][i] = bands[j][i];
				
				bands[j][i] *= outBandVol[j];
				
				bands[j][i] = std::lerp(bandsDry[j][i], bands[j][i], mix);
			}
			
			s[i] = bands[0][i] + bands[1][i] + bands[2][i];
			
			s[i] *= std::lerp(1.f, outVol, mix * (depthScaling ? depth : 1));
		}
		
		// Convert mid/side back to left/right.
		// Note that the side channel was intentionally made to be 6 dB louder prior to compression.
		if (midside)
		{
			float tempS0 = s[0];
			s[0] = s[0] + (s[1] * 0.5f);
			s[1] = tempS0 - (s[1] * 0.5f);
		}
		
		if (--m_lookWrite < 0) { m_lookWrite = m_lookBufLength - 1; }

		buf[f][0] = d * buf[f][0] + w * s[0];
		buf[f][1] = d * buf[f][1] + w * s[1];
	}

	return ProcessStatus::ContinueIfNotQuiet;
}

extern "C"
{
	// necessary for getting instance out of shared lib
	PLUGIN_EXPORT Plugin * lmms_plugin_main(Model* parent, void* data)
	{
		return new LOMMEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key *>(data));
	}
}

} // namespace lmms
