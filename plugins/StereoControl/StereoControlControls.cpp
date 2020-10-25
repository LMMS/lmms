/*
 * StereoControlControls.cpp
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


#include <QDomElement>

#include "Engine.h"
#include "Song.h"
#include "StereoControlControls.h"
#include "StereoControl.h"


StereoControlControls::StereoControlControls(StereoControlEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_gainModel(0.0f, -54.0f, 54.0f, 0.01f, this, tr("gain")),
	m_stereoizerModel(0.0f, 0.0f, 200.0f, 0.01f, this, tr("Stereoize")),
	m_widthModel(100.0f, 0.0f, 200.0f, 0.01f, this, tr("Width")),
	m_panModel(0.0f, -100.0f, 100.0f, 0.01f, this, tr("Pan")),
	m_monoBassFreqModel(80.0f, 0.0f, 500.0f, 0.01f, this, tr("Mono Bass Frequency")),
	m_stereoizerLPModel(20000.0f, 0.0f, 20000.0f, 0.01f, this, tr("Stereoizer Lowpass")),
	m_stereoizerHPModel(0.0f, 0.0f, 20000.0f, 0.01f, this, tr("Stereoizer Highpass")),
	m_panSpectralModel(0.0f, 0.0f, 100.0f, 0.01f, this, tr("Spectral Panning")),
	m_panDelayModel(100.0f, 0.0f, 200.0f, 0.01f, this, tr("Panning Delay")),
	m_panDualLModel(-100.0f, -100.0f, 100.0f, 0.01f, this, tr("Left Pan")),
	m_panDualRModel(100.0f, -100.0f, 100.0f, 0.01f, this, tr("Right Pan")),
	m_panModeModel(0, 0, 2, this, tr("Panning Mode")),
	m_soloChannelModel(this, tr("Solo Channel")),
	m_monoModel(false, this, tr("Mono")),
	m_dcModel(false, this, tr("DC Offset Removal")),
	m_muteModel(false, this, tr("Mute")),
	m_monoBassModel(false, this, tr("Mono Bass")),
	m_auditionModel(false, this, tr("Mono Bass Audition")),
	m_invertLModel(false, this, tr("Invert Left Channel")),
	m_invertRModel(false, this, tr("Invert Right Channel"))
{
	m_monoBassFreqModel.setScaleLogarithmic(true);
	m_stereoizerLPModel.setScaleLogarithmic(true);
	m_stereoizerHPModel.setScaleLogarithmic(true);

	m_soloChannelModel.addItem(tr("Stereo"));
	m_soloChannelModel.addItem(tr("Left"));
	m_soloChannelModel.addItem(tr("Right"));
}


void StereoControlControls::saveSettings(QDomDocument& doc, QDomElement& _this)
{
	m_gainModel.saveSettings(doc, _this, "gain"); 
	m_stereoizerModel.saveSettings(doc, _this, "stereoizer"); 
	m_widthModel.saveSettings(doc, _this, "width");
	m_panModel.saveSettings(doc, _this, "pan");
	m_panModeModel.saveSettings(doc, _this, "panMode");
	m_monoModel.saveSettings(doc, _this, "mono");
	m_dcModel.saveSettings(doc, _this, "dc");
	m_muteModel.saveSettings(doc, _this, "mute");
	m_monoBassModel.saveSettings(doc, _this, "monoBass");
	m_auditionModel.saveSettings(doc, _this, "audition");
	m_invertLModel.saveSettings(doc, _this, "invertL");
	m_invertRModel.saveSettings(doc, _this, "invertR");
	m_soloChannelModel.saveSettings(doc, _this, "soloChannel");
	m_monoBassFreqModel.saveSettings(doc, _this, "monoBassFreq");
	m_stereoizerLPModel.saveSettings(doc, _this, "stereoizerLP");
	m_stereoizerHPModel.saveSettings(doc, _this, "stereoizerHP");
	m_panSpectralModel.saveSettings(doc, _this, "panSpectral");
	m_panDelayModel.saveSettings(doc, _this, "panDelay");
	m_panDualLModel.saveSettings(doc, _this, "panDualL");
	m_panDualRModel.saveSettings(doc, _this, "panDualR");
}



void StereoControlControls::loadSettings(const QDomElement& _this)
{
	m_gainModel.loadSettings(_this, "gain");
	m_stereoizerModel.loadSettings(_this, "stereoizer");
	m_widthModel.loadSettings(_this, "width");
	m_panModel.loadSettings(_this, "pan");
	m_panModeModel.loadSettings(_this, "panMode");
	m_monoModel.loadSettings(_this, "mono");
	m_dcModel.loadSettings(_this, "dc");
	m_muteModel.loadSettings(_this, "mute");
	m_monoBassModel.loadSettings(_this, "monoBass");
	m_auditionModel.loadSettings(_this, "audition");
	m_invertLModel.loadSettings(_this, "invertL");
	m_invertRModel.loadSettings(_this, "invertR");
	m_soloChannelModel.loadSettings(_this, "soloChannel");
	m_monoBassFreqModel.loadSettings(_this, "monoBassFreq");
	m_stereoizerLPModel.loadSettings(_this, "stereoizerLP");
	m_stereoizerHPModel.loadSettings(_this, "stereoizerHP");
	m_panSpectralModel.loadSettings(_this, "panSpectral");
	m_panDelayModel.loadSettings(_this, "panDelay");
	m_panDualLModel.loadSettings(_this, "panDualL");
	m_panDualRModel.loadSettings(_this, "panDualR");
}


