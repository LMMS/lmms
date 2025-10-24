/*
 * PhaserControls.cpp
 *
 * Copyright (c) 2019 Lost Robot <r94231@gmail.com>
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


#include <QDomElement>

#include "PhaserControls.h"
#include "PhaserEffect.h"

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Song.h"

namespace lmms
{

constexpr int PHA_DOT_SLIDER_LENGTH = 338;
constexpr float PHA_MIN_FREQ = 20;
constexpr float PHA_MAX_FREQ = 20000;


PhaserControls::PhaserControls(PhaserEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_cutoffModel(640.0f, 20.0f, 20000.0f, 0.01f, this, tr("Cutoff")),
	m_resonanceModel(0.05f, 0.05f, 2.0f, 0.001f, this, tr("Resonance")),
	m_feedbackModel(0.0f, -100.0f, 100.0f, 0.01f, this, tr("Feedback")),
	m_orderModel(8, 0, 32, this, tr("Order")),
	m_delayModel(0.0f, 0.0f, 20.0f, 0.001f, this, tr("Delay")),
	m_rateModel(9.0f, 0.01f, 60.0f, 0.001f, 60000.0, this, tr("Rate")),
	m_amountModel(4.5f, 0.f, 5.0f, 0.01f, this, tr("Amount")),
	m_enableLFOModel(true , this, tr("Enable LFO")),
	m_phaseModel(180.0f, 0.f, 360.0f, 0.01f, this, tr("Phase")),
	m_inFollowModel(0.f, -30.f, 30.0f, 0.01f, this, tr("Input Follow")),
	m_attackModel(200.0f, 0.0f, 2000.0f, 1.0f, this, tr("Attack")),
	m_releaseModel(500.0f, 0.0f, 2000.0f, 1.0f, this, tr("Release")),
	m_distortionModel(0.0f, 0.0f, 100.0f, 0.001f, this, tr("Distortion")),
	m_analogDistModel(1.0f, 0.125f, 8.0f, 0.01f, this, tr("Analog Distortion")),
	m_cutoffControlModel(1.0f, -4.0f, 4.0f, 0.01f, this, tr("Delay Control")),
	m_delayControlModel(0.0f, -4.0f, 4.0f, 0.01f, this, tr("Delay Control")),
	m_outGainModel(0.0, -60.0, 20.0, 0.01, this, tr("Output gain")),
	m_inGainModel(0.0, -60.0, 20.0, 0.01, this, tr("Input gain")),
	m_invertModel(false, this, tr("Invert")),
	m_wetModel(false, this, tr("Wet")),
	m_analogModel(false, this, tr("Analog")),
	m_doubleModel(false, this, tr("Double")),
	m_aliasModel(false, this, tr("Alias")),
	m_modeModel(this, tr("Feedback Circuit Mode"))
{
	m_cutoffModel.setScaleLogarithmic(true);
	m_resonanceModel.setScaleLogarithmic(true);
	m_delayModel.setScaleLogarithmic(true);
	m_rateModel.setScaleLogarithmic(true);
	m_analogDistModel.setScaleLogarithmic(true);
	m_cutoffControlModel.setScaleLogarithmic(true);
	m_delayControlModel.setScaleLogarithmic(true);

	m_modeModel.addItem(tr("Default"));
	m_modeModel.addItem(tr("Standard"));
	m_modeModel.addItem(tr("Nested"));
	m_modeModel.addItem(tr("Yeet Mode"));
}


void PhaserControls::saveSettings(QDomDocument& doc, QDomElement& _this)
{
	m_cutoffModel.saveSettings(doc, _this, "cutoff");
	m_resonanceModel.saveSettings(doc, _this, "resonance");
	m_feedbackModel.saveSettings(doc, _this, "feedback");
	m_orderModel.saveSettings(doc, _this, "order");
	m_delayModel.saveSettings(doc, _this, "delay");
	m_rateModel.saveSettings(doc, _this, "rate");
	m_enableLFOModel.saveSettings(doc, _this, "enableLFO");
	m_amountModel.saveSettings(doc, _this, "amount");
	m_phaseModel.saveSettings(doc, _this, "phase");
	m_inFollowModel.saveSettings(doc, _this, "inFollow");
	m_attackModel.saveSettings(doc, _this, "attack");
	m_releaseModel.saveSettings(doc, _this, "release");
	m_distortionModel.saveSettings(doc, _this, "distortion");
	m_outGainModel.saveSettings(doc, _this, "outGain");
	m_inGainModel.saveSettings(doc, _this, "inGain");
	m_invertModel.saveSettings(doc, _this, "invert");
	m_wetModel.saveSettings(doc, _this, "wet");
	m_analogModel.saveSettings(doc, _this, "analog");
	m_doubleModel.saveSettings(doc, _this, "double");
	m_aliasModel.saveSettings(doc, _this, "alias");
	m_modeModel.saveSettings(doc, _this, "mode");
	m_analogDistModel.saveSettings(doc, _this, "analogDist");
	m_cutoffControlModel.saveSettings(doc, _this, "cutoffControl");
	m_delayControlModel.saveSettings(doc, _this, "delayControl");
}



void PhaserControls::loadSettings(const QDomElement& _this)
{
	m_cutoffModel.loadSettings(_this, "cutoff");
	m_resonanceModel.loadSettings(_this, "resonance");
	m_feedbackModel.loadSettings(_this, "feedback");
	m_orderModel.loadSettings(_this, "order");
	m_delayModel.loadSettings(_this, "delay");
	m_rateModel.loadSettings(_this, "rate");
	m_amountModel.loadSettings(_this, "amount");
	m_enableLFOModel.loadSettings(_this, "enableLFO");
	m_phaseModel.loadSettings(_this, "phase");
	m_inFollowModel.loadSettings(_this, "inFollow");
	m_attackModel.loadSettings(_this, "attack");
	m_releaseModel.loadSettings(_this, "release");
	m_distortionModel.loadSettings(_this, "distortion");
	m_outGainModel.loadSettings(_this, "outGain");
	m_inGainModel.loadSettings(_this, "inGain");
	m_invertModel.loadSettings(_this, "invert");
	m_wetModel.loadSettings(_this, "wet");
	m_analogModel.loadSettings(_this, "analog");
	m_doubleModel.loadSettings(_this, "double");
	m_aliasModel.loadSettings(_this, "alias");
	m_modeModel.loadSettings(_this, "mode");
	m_analogDistModel.loadSettings(_this, "analogDist");
	m_cutoffControlModel.loadSettings(_this, "cutoffControl");
	m_delayControlModel.loadSettings(_this, "delayControl");
}


} // namespace lmms

