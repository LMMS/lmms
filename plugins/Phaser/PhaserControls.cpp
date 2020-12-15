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
	m_attackModel(200.0f, 0.1f, 2000.0f, 0.1f, this, tr("Attack")),
	m_releaseModel(500.0f, 0.1f, 2000.0f, 0.1f, this, tr("Release")),
	m_distortionModel(0.0f, 0.0f, 100.0f, 0.001f, this, tr("Distortion")),
	m_analogDistModel(1.0f, 0.25f, 8.0f, 0.01f, this, tr("Analog Distortion")),
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
	m_modeModel.addItem(tr("Inane"));
}


void PhaserControls::saveSettings(QDomDocument& doc, QDomElement& elem)
{
	m_cutoffModel.saveSettings(doc, elem, "cutoff");
	m_resonanceModel.saveSettings(doc, elem, "resonance");
	m_feedbackModel.saveSettings(doc, elem, "feedback");
	m_orderModel.saveSettings(doc, elem, "order");
	m_delayModel.saveSettings(doc, elem, "delay");
	m_rateModel.saveSettings(doc, elem, "rate");
	m_enableLFOModel.saveSettings(doc, elem, "enableLFO");
	m_amountModel.saveSettings(doc, elem, "amount");
	m_phaseModel.saveSettings(doc, elem, "phase");
	m_inFollowModel.saveSettings(doc, elem, "inFollow");
	m_attackModel.saveSettings(doc, elem, "attack");
	m_releaseModel.saveSettings(doc, elem, "release");
	m_distortionModel.saveSettings(doc, elem, "distortion");
	m_outGainModel.saveSettings(doc, elem, "outGain");
	m_inGainModel.saveSettings(doc, elem, "inGain");
	m_invertModel.saveSettings(doc, elem, "invert");
	m_wetModel.saveSettings(doc, elem, "wet");
	m_analogModel.saveSettings(doc, elem, "analog");
	m_doubleModel.saveSettings(doc, elem, "double");
	m_aliasModel.saveSettings(doc, elem, "alias");
	m_modeModel.saveSettings(doc, elem, "mode");
	m_analogDistModel.saveSettings(doc, elem, "analogDist");
	m_cutoffControlModel.saveSettings(doc, elem, "cutoffControl");
	m_delayControlModel.saveSettings(doc, elem, "delayControl");
}



void PhaserControls::loadSettings(const QDomElement& elem)
{
	m_cutoffModel.loadSettings(elem, "cutoff");
	m_resonanceModel.loadSettings(elem, "resonance");
	m_feedbackModel.loadSettings(elem, "feedback");
	m_orderModel.loadSettings(elem, "order");
	m_delayModel.loadSettings(elem, "delay");
	m_rateModel.loadSettings(elem, "rate");
	m_amountModel.loadSettings(elem, "amount");
	m_enableLFOModel.loadSettings(elem, "enableLFO");
	m_phaseModel.loadSettings(elem, "phase");
	m_inFollowModel.loadSettings(elem, "inFollow");
	m_attackModel.loadSettings(elem, "attack");
	m_releaseModel.loadSettings(elem, "release");
	m_distortionModel.loadSettings(elem, "distortion");
	m_outGainModel.loadSettings(elem, "outGain");
	m_inGainModel.loadSettings(elem, "inGain");
	m_invertModel.loadSettings(elem, "invert");
	m_wetModel.loadSettings(elem, "wet");
	m_analogModel.loadSettings(elem, "analog");
	m_doubleModel.loadSettings(elem, "double");
	m_aliasModel.loadSettings(elem, "alias");
	m_modeModel.loadSettings(elem, "mode");
	m_analogDistModel.loadSettings(elem, "analogDist");
	m_cutoffControlModel.loadSettings(elem, "cutoffControl");
	m_delayControlModel.loadSettings(elem, "delayControl");
}


