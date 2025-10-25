/*
 * SlewDistortionControls.cpp
 *
 * Copyright (c) 2025 Lost Robot <r94231/at/gmail/dot/com>
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

#include "SlewDistortionControls.h"

#include <QDomElement>

#include "SlewDistortion.h"

namespace lmms
{

SlewDistortionControls::SlewDistortionControls(SlewDistortion* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_distType1Model(this, tr("Type 1")),
	m_distType2Model(this, tr("Type 2")),
	m_drive1Model(0.0f, -24.f, 24.0f, 0.0001f, this, tr("Drive 1")),
	m_drive2Model(0.0f, -24.f, 24.0f, 0.0001f, this, tr("Drive 2")),
	m_slewUp1Model(6.0f, -96.f, 6.0f, 0.0001f, this, tr("Slew Up 1")),
	m_slewUp2Model(6.0f, -96.f, 6.0f, 0.0001f, this, tr("Slew Up 2")),
	m_slewDown1Model(6.0f, -96.f, 6.0f, 0.0001f, this, tr("Slew Down 1")),
	m_slewDown2Model(6.0f, -96.f, 6.0f, 0.0001f, this, tr("Slew Down 2")),
	m_bias1Model(0.0f, -2.0f, 2.0f, 0.0001f, this, tr("Bias 1")),
	m_bias2Model(0.0f, -2.0f, 2.0f, 0.0001f, this, tr("Bias 2")),
	m_warp1Model(0.0f, 0.0f, 0.99f, 0.0001f, this, tr("Warp 1")),
	m_warp2Model(0.0f, 0.0f, 0.99f, 0.0001f, this, tr("Warp 2")),
	m_crush1Model(0.0f, 0.0f, 24.0f, 0.0001f, this, tr("Crush 1")),
	m_crush2Model(0.0f, 0.0f, 24.0f, 0.0001f, this, tr("Crush 2")),
	m_outVol1Model(0.0f, -24.0f, 24.0f, 0.0001f, this, tr("Out Vol 1")),
	m_outVol2Model(0.0f, -24.0f, 24.0f, 0.0001f, this, tr("Out Vol 2")),
	m_attack1Model(2.0f, 0.01f, 200.0f, 0.01f, this, tr("Attack 1")),
	m_attack2Model(2.0f, 0.01f, 200.0f, 0.01f, this, tr("Attack 2")),
	m_release1Model(20.0f, 0.01f, 800.0f, 0.01f, this, tr("Release 1")),
	m_release2Model(20.0f, 0.01f, 800.0f, 0.01f, this, tr("Release 2")),
	m_dynamics1Model(0.0f, 0.0f, 1.0f, 0.0001f, this, tr("Dynamics 1")),
	m_dynamics2Model(0.0f, 0.0f, 1.0f, 0.0001f, this, tr("Dynamics 2")),
	m_dynamicSlew1Model(0.0f, -8.0f, 8.0f, 0.0001f, this, tr("Dynamic Slew 1")),
	m_dynamicSlew2Model(0.0f, -8.0f, 8.0f, 0.0001f, this, tr("Dynamic Slew 2")),
	m_dcRemoveModel(true, this, tr("DC Offset Remover")),
	m_multibandModel(false, this, tr("Multiband")),
	m_oversamplingModel(0, 0, SLEWDIST_MAX_OVERSAMPLE_STAGES, this, tr("Oversample")),
	m_splitModel(200.0f, 100.0f, 20000.0f, 0.1f, this, tr("Split")),
	m_mix1Model(1.0f, 0.0f, 1.0f, 0.0001f, this, tr("Mix 1")),
	m_mix2Model(1.0f, 0.0f, 1.0f, 0.0001f, this, tr("Mix 2")),
	m_slewLink1Model(true, this, tr("Slew Link 1")),
	m_slewLink2Model(true, this, tr("Slew Link 2"))
{
	m_slewUp1Model.setScaleLogarithmic(true);
	m_slewUp2Model.setScaleLogarithmic(true);
	m_slewDown1Model.setScaleLogarithmic(true);
	m_slewDown2Model.setScaleLogarithmic(true);
	m_crush1Model.setScaleLogarithmic(true);
	m_crush2Model.setScaleLogarithmic(true);
	m_attack1Model.setScaleLogarithmic(true);
	m_attack2Model.setScaleLogarithmic(true);
	m_release1Model.setScaleLogarithmic(true);
	m_release2Model.setScaleLogarithmic(true);
	m_dynamicSlew1Model.setScaleLogarithmic(true);
	m_dynamicSlew2Model.setScaleLogarithmic(true);
	m_splitModel.setScaleLogarithmic(true);
	
	m_distType1Model.addItem(tr("Hard Clip"));
	m_distType1Model.addItem(tr("Tanh"));
	m_distType1Model.addItem(tr("Fast Soft Clip 1"));
	m_distType1Model.addItem(tr("Fast Soft Clip 2"));
	m_distType1Model.addItem(tr("Sinusoidal"));
	m_distType1Model.addItem(tr("Foldback"));
	m_distType1Model.addItem(tr("Full Rectify"));
	m_distType1Model.addItem(tr("Half Rectify"));
	m_distType1Model.addItem(tr("Smooth Rectify"));
	m_distType1Model.addItem(tr("Bitcrush"));
	
	m_distType2Model.addItem(tr("Hard Clip"));
	m_distType2Model.addItem(tr("Tanh"));
	m_distType2Model.addItem(tr("Fast Soft Clip 1"));
	m_distType2Model.addItem(tr("Fast Soft Clip 2"));
	m_distType2Model.addItem(tr("Sinusoidal"));
	m_distType2Model.addItem(tr("Foldback"));
	m_distType2Model.addItem(tr("Full Rectify"));
	m_distType2Model.addItem(tr("Half Rectify"));
	m_distType2Model.addItem(tr("Smooth Rectify"));
	m_distType2Model.addItem(tr("Bitcrush"));
}


void SlewDistortionControls::loadSettings(const QDomElement& parent)
{
	m_distType1Model.loadSettings(parent, "distType1");
	m_distType2Model.loadSettings(parent, "distType2");
	m_drive1Model.loadSettings(parent, "drive1");
	m_drive2Model.loadSettings(parent, "drive2");
	m_slewUp1Model.loadSettings(parent, "slewUp1");
	m_slewUp2Model.loadSettings(parent, "slewUp2");
	m_slewDown1Model.loadSettings(parent, "slewDown1");
	m_slewDown2Model.loadSettings(parent, "slewDown2");
	m_bias1Model.loadSettings(parent, "bias1");
	m_bias2Model.loadSettings(parent, "bias2");
	m_warp1Model.loadSettings(parent, "warp1");
	m_warp2Model.loadSettings(parent, "warp2");
	m_crush1Model.loadSettings(parent, "crush1");
	m_crush2Model.loadSettings(parent, "crush2");
	m_outVol1Model.loadSettings(parent, "outVol1");
	m_outVol2Model.loadSettings(parent, "outVol2");
	m_attack1Model.loadSettings(parent, "attack1");
	m_attack2Model.loadSettings(parent, "attack2");
	m_release1Model.loadSettings(parent, "release1");
	m_release2Model.loadSettings(parent, "release2");
	m_dynamics1Model.loadSettings(parent, "dynamics1");
	m_dynamics2Model.loadSettings(parent, "dynamics2");
	m_dynamicSlew1Model.loadSettings(parent, "dynamicSlew1");
	m_dynamicSlew2Model.loadSettings(parent, "dynamicSlew2");
	m_dcRemoveModel.loadSettings(parent, "dcRemove");
	m_multibandModel.loadSettings(parent, "multiband");
	m_oversamplingModel.loadSettings(parent, "oversampling");
	m_splitModel.loadSettings(parent, "split");
	m_mix1Model.loadSettings(parent, "mix1");
	m_mix2Model.loadSettings(parent, "mix2");
	m_slewLink1Model.loadSettings(parent, "slewLink1");
	m_slewLink2Model.loadSettings(parent, "slewLink2");
}



void SlewDistortionControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_distType1Model.saveSettings(doc, parent, "distType1");
	m_distType2Model.saveSettings(doc, parent, "distType2");
	m_drive1Model.saveSettings(doc, parent, "drive1");
	m_drive2Model.saveSettings(doc, parent, "drive2");
	m_slewUp1Model.saveSettings(doc, parent, "slewUp1");
	m_slewUp2Model.saveSettings(doc, parent, "slewUp2");
	m_slewDown1Model.saveSettings(doc, parent, "slewDown1");
	m_slewDown2Model.saveSettings(doc, parent, "slewDown2");
	m_bias1Model.saveSettings(doc, parent, "bias1");
	m_bias2Model.saveSettings(doc, parent, "bias2");
	m_warp1Model.saveSettings(doc, parent, "warp1");
	m_warp2Model.saveSettings(doc, parent, "warp2");
	m_crush1Model.saveSettings(doc, parent, "crush1");
	m_crush2Model.saveSettings(doc, parent, "crush2");
	m_outVol1Model.saveSettings(doc, parent, "outVol1");
	m_outVol2Model.saveSettings(doc, parent, "outVol2");
	m_attack1Model.saveSettings(doc, parent, "attack1");
	m_attack2Model.saveSettings(doc, parent, "attack2");
	m_release1Model.saveSettings(doc, parent, "release1");
	m_release2Model.saveSettings(doc, parent, "release2");
	m_dynamics1Model.saveSettings(doc, parent, "dynamics1");
	m_dynamics2Model.saveSettings(doc, parent, "dynamics2");
	m_dynamicSlew1Model.saveSettings(doc, parent, "dynamicSlew1");
	m_dynamicSlew2Model.saveSettings(doc, parent, "dynamicSlew2");
	m_dcRemoveModel.saveSettings(doc, parent, "dcRemove");
	m_multibandModel.saveSettings(doc, parent, "multiband");
	m_oversamplingModel.saveSettings(doc, parent, "oversampling");
	m_splitModel.saveSettings(doc, parent, "split");
	m_mix1Model.saveSettings(doc, parent, "mix1");
	m_mix2Model.saveSettings(doc, parent, "mix2");
	m_slewLink1Model.saveSettings(doc, parent, "slewLink1");
	m_slewLink2Model.saveSettings(doc, parent, "slewLink2");
}



} // namespace lmms
