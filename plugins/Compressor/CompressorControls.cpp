/*
 * CompressorControls.cpp
 *
 * Copyright (c) 2020 Lost Robot <r94231@gmail.com>
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


#include "CompressorControls.h"
#include "Compressor.h"



namespace lmms
{

CompressorControls::CompressorControls(CompressorEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_thresholdModel(-8.0f, -60.0f, 0.0f, 0.001f, this, tr("Threshold")),
	m_ratioModel(1.8f, 1.0f, 20.0f, 0.001f, this, tr("Ratio")),
	m_attackModel(10.0f, 0.005f, 250.f, 0.001f, this, tr("Attack")),
	m_releaseModel(100.0f, 1.f, 2500.f, 0.001f, this, tr("Release")),
	m_kneeModel(12.0f, 0.0f, 96.0f, 0.01f, this, tr("Knee")),
	m_holdModel(0.0f, 0.0f, 500.0f, 0.01f, this, tr("Hold")),
	m_rangeModel(-240.0f, -240.0f, 0.0f, 0.01f, this, tr("Range")),
	m_rmsModel(1.0f, 0.0f, 250.0f, 0.01f, this, tr("RMS Size")),
	m_midsideModel(0.0f, 0.0f, 1.0f, this, tr("Mid/Side")),
	m_peakmodeModel(0.0f, 0.0f, 1.0f, this, tr("Peak Mode")),
	m_lookaheadLengthModel(0.0f, 0.0f, 20.0f, 0.0001f, this, tr("Lookahead Length")),
	m_inBalanceModel(0.0f, -1.0f, 1.0f, 0.0001f, this, tr("Input Balance")),
	m_outBalanceModel(0.0f, -1.0f, 1.0f, 0.0001f, this, tr("Output Balance")),
	m_limiterModel(0.f, 0.f, 1.0f, this, tr("Limiter")),
	m_outGainModel(0.f, -60.f, 30.f, 0.01f, this, tr("Output Gain")),
	m_inGainModel(0.f, -60.f, 30.f, 0.01f, this, tr("Input Gain")),
	m_blendModel(1.f, 0.f, 3.f, 0.0001f, this, tr("Blend")),
	m_stereoBalanceModel(0.0f, -1.0f, 1.0f, 0.0001f, this, tr("Stereo Balance")),
	m_autoMakeupModel(false, this, tr("Auto Makeup Gain")),
	m_auditionModel(false, this, tr("Audition")),
	m_feedbackModel(false, this, tr("Feedback")),
	m_autoAttackModel(0.0f, 0.f, 100.0f, 0.01f, this, tr("Auto Attack")),
	m_autoReleaseModel(0.0f, 0.f, 100.0f, 0.01f, this, tr("Auto Release")),
	m_lookaheadModel(false, this, tr("Lookahead")),
	m_tiltModel(0.0f, -6.0f, 6.0f, 0.0001f, this, tr("Tilt")),
	m_tiltFreqModel(150.0f, 20.0f, 20000.0f, 0.1f, this, tr("Tilt Frequency")),
	m_stereoLinkModel(1.0f, 0.0f, 4.0f, this, tr("Stereo Link")),
	m_mixModel(100.0f, 0.f, 100.0f, 0.01f, this, tr("Mix"))
{
	m_ratioModel.setScaleLogarithmic(true);
	m_holdModel.setScaleLogarithmic(true);
	m_attackModel.setScaleLogarithmic(true);
	m_releaseModel.setScaleLogarithmic(true);
	m_thresholdModel.setScaleLogarithmic(true);
	m_rangeModel.setScaleLogarithmic(true);
	m_lookaheadLengthModel.setScaleLogarithmic(true);
	m_rmsModel.setScaleLogarithmic(true);
	m_kneeModel.setScaleLogarithmic(true);
	m_tiltFreqModel.setScaleLogarithmic(true);
	m_rangeModel.setScaleLogarithmic(true);
}


void CompressorControls::saveSettings(QDomDocument& doc, QDomElement& _this)
{
	m_thresholdModel.saveSettings(doc, _this, "threshold"); 
	m_ratioModel.saveSettings(doc, _this, "ratio");
	m_attackModel.saveSettings(doc, _this, "attack");
	m_releaseModel.saveSettings(doc, _this, "release");
	m_kneeModel.saveSettings(doc, _this, "knee");
	m_holdModel.saveSettings(doc, _this, "hold");
	m_rangeModel.saveSettings(doc, _this, "range");
	m_rmsModel.saveSettings(doc, _this, "rms");
	m_midsideModel.saveSettings(doc, _this, "midside");
	m_peakmodeModel.saveSettings(doc, _this, "peakmode");
	m_lookaheadLengthModel.saveSettings(doc, _this, "lookaheadLength");
	m_inBalanceModel.saveSettings(doc, _this, "inBalance");
	m_outBalanceModel.saveSettings(doc, _this, "outBalance");
	m_limiterModel.saveSettings(doc, _this, "limiter");
	m_outGainModel.saveSettings(doc, _this, "outGain");
	m_inGainModel.saveSettings(doc, _this, "inGain");
	m_blendModel.saveSettings(doc, _this, "blend");
	m_stereoBalanceModel.saveSettings(doc, _this, "stereoBalance");
	m_autoMakeupModel.saveSettings(doc, _this, "autoMakeup");
	m_auditionModel.saveSettings(doc, _this, "audition");
	m_feedbackModel.saveSettings(doc, _this, "feedback");
	m_autoAttackModel.saveSettings(doc, _this, "autoAttack");
	m_autoReleaseModel.saveSettings(doc, _this, "autoRelease");
	m_lookaheadModel.saveSettings(doc, _this, "lookahead");
	m_tiltModel.saveSettings(doc, _this, "tilt");
	m_tiltFreqModel.saveSettings(doc, _this, "tiltFreq");
	m_stereoLinkModel.saveSettings(doc, _this, "stereoLink");
	m_mixModel.saveSettings(doc, _this, "mix");
}



void CompressorControls::loadSettings(const QDomElement& _this)
{
	m_thresholdModel.loadSettings(_this, "threshold");
	m_ratioModel.loadSettings(_this, "ratio");
	m_attackModel.loadSettings(_this, "attack");
	m_releaseModel.loadSettings(_this, "release");
	m_kneeModel.loadSettings(_this, "knee");
	m_holdModel.loadSettings(_this, "hold");
	m_rangeModel.loadSettings(_this, "range");
	m_rmsModel.loadSettings(_this, "rms");
	m_midsideModel.loadSettings(_this, "midside");
	m_peakmodeModel.loadSettings(_this, "peakmode");
	m_lookaheadLengthModel.loadSettings(_this, "lookaheadLength");
	m_inBalanceModel.loadSettings(_this, "inBalance");
	m_outBalanceModel.loadSettings(_this, "outBalance");
	m_limiterModel.loadSettings(_this, "limiter");
	m_outGainModel.loadSettings(_this, "outGain");
	m_inGainModel.loadSettings(_this, "inGain");
	m_blendModel.loadSettings(_this, "blend");
	m_stereoBalanceModel.loadSettings(_this, "stereoBalance");
	m_autoMakeupModel.loadSettings(_this, "autoMakeup");
	m_auditionModel.loadSettings(_this, "audition");
	m_feedbackModel.loadSettings(_this, "feedback");
	m_autoAttackModel.loadSettings(_this, "autoAttack");
	m_autoReleaseModel.loadSettings(_this, "autoRelease");
	m_lookaheadModel.loadSettings(_this, "lookahead");
	m_tiltModel.loadSettings(_this, "tilt");
	m_tiltFreqModel.loadSettings(_this, "tiltFreq");
	m_stereoLinkModel.loadSettings(_this, "stereoLink");
	m_mixModel.loadSettings(_this, "mix");
}


} // namespace lmms