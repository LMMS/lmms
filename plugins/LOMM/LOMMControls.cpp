/*
 * LOMMControls.cpp
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


#include "LOMMControls.h"
#include "LOMM.h"

#include <QMessageBox>

namespace lmms
{

LOMMControls::LOMMControls(LOMMEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_depthModel(0.4f, 0, 1, 0.00001f, this, tr("Depth")),
	m_timeModel(1, 0, 10, 0.00001f, this, tr("Time")),
	m_inVolModel(0, -48, 48, 0.00001f, this, tr("Input Volume")),
	m_outVolModel(8, -48, 48, 0.00001f, this, tr("Output Volume")),
	m_upwardModel(1, 0, 2, 0.00001f, this, tr("Upward Depth")),
	m_downwardModel(1, 0, 2, 0.00001f, this, tr("Downward Depth")),
	m_split1Model(2500, 20, 20000, 0.01f, this, tr("High/Mid Split")),
	m_split2Model(88.3f, 20, 20000, 0.01f, this, tr("Mid/Low Split")),
	m_split1EnabledModel(true, this, tr("Enable High/Mid Split")),
	m_split2EnabledModel(true, this, tr("Enable Mid/Low Split")),
	m_band1EnabledModel(true, this, tr("Enable High Band")),
	m_band2EnabledModel(true, this, tr("Enable Mid Band")),
	m_band3EnabledModel(true, this, tr("Enable Low Band")),
	m_inHighModel(0, -48, 48, 0.00001f, this, tr("High Input Volume")),
	m_inMidModel(0, -48, 48, 0.00001f, this, tr("Mid Input Volume")),
	m_inLowModel(0, -48, 48, 0.00001f, this, tr("Low Input Volume")),
	m_outHighModel(4.6f, -48, 48, 0.00001f, this, tr("High Output Volume")),
	m_outMidModel(0.f, -48, 48, 0.00001f, this, tr("Mid Output Volume")),
	m_outLowModel(4.6f, -48, 48, 0.00001f, this, tr("Low Output Volume")),
	m_aThreshHModel(-30.3f, LOMM_DISPLAY_MIN, LOMM_DISPLAY_MAX, 0.001f, this, tr("Above Threshold High")),
	m_aThreshMModel(-25.f, LOMM_DISPLAY_MIN, LOMM_DISPLAY_MAX, 0.001f, this, tr("Above Threshold Mid")),
	m_aThreshLModel(-28.6f, LOMM_DISPLAY_MIN, LOMM_DISPLAY_MAX, 0.001f, this, tr("Above Threshold Low")),
	m_aRatioHModel(99.99f, 1, 99.99f, 0.01f, this, tr("Above Ratio High")),
	m_aRatioMModel(66.7f, 1, 99.99f, 0.01f, this, tr("Above Ratio Mid")),
	m_aRatioLModel(66.7f, 1, 99.99f, 0.01f, this, tr("Above Ratio Low")),
	m_bThreshHModel(-35.6f, LOMM_DISPLAY_MIN, LOMM_DISPLAY_MAX, 0.001f, this, tr("Below Threshold High")),
	m_bThreshMModel(-36.6f, LOMM_DISPLAY_MIN, LOMM_DISPLAY_MAX, 0.001f, this, tr("Below Threshold Mid")),
	m_bThreshLModel(-35.6f, LOMM_DISPLAY_MIN, LOMM_DISPLAY_MAX, 0.001f, this, tr("Below Threshold Low")),
	m_bRatioHModel(4.17f, 1, 99.99f, 0.01f, this, tr("Below Ratio High")),
	m_bRatioMModel(4.17f, 1, 99.99f, 0.01f, this, tr("Below Ratio Mid")),
	m_bRatioLModel(4.17f, 1, 99.99f, 0.01f, this, tr("Below Ratio Low")),
	m_atkHModel(13.5f, 0, 1000, 0.001f, this, tr("Attack High")),
	m_atkMModel(22.4f, 0, 1000, 0.001f, this, tr("Attack Mid")),
	m_atkLModel(47.8f, 0, 1000, 0.001f, this, tr("Attack Low")),
	m_relHModel(132, 0, 1000, 0.001f, this, tr("Release High")),
	m_relMModel(282, 0, 1000, 0.001f, this, tr("Release Mid")),
	m_relLModel(282, 0, 1000, 0.001f, this, tr("Release Low")),
	m_rmsTimeModel(10, 0, 500, 0.001f, this, tr("RMS Time")),
	m_kneeModel(6, 0, 36, 0.00001f, this, tr("Knee")),
	m_rangeModel(36, 0, 96, 0.00001f, this, tr("Range")),
	m_balanceModel(0, -18, 18, 0.00001f, this, tr("Balance")),
	m_depthScalingModel(true, this, tr("Scale output volume with Depth")),
	m_stereoLinkModel(false, this, tr("Stereo Link")),
	m_autoTimeModel(0, 0, 1, 0.00001f, this, tr("Auto Time")),
	m_mixModel(1, 0, 1, 0.00001f, this, tr("Mix")),
	m_feedbackModel(false, this, tr("Feedback")),
	m_midsideModel(false, this, tr("Mid/Side")),
	m_lookaheadEnableModel(false, this, tr("Lookahead")),
	m_lookaheadModel(0.f, 0.f, LOMM_MAX_LOOKAHEAD, 0.01f, this, tr("Lookahead Length")),
	m_lowSideUpwardSuppressModel(false, this, tr("Suppress upward compression for side band"))
{
	auto models = {&m_timeModel, &m_inVolModel, &m_outVolModel, &m_inHighModel, &m_inMidModel,
		&m_inLowModel, &m_outHighModel, &m_outMidModel, &m_outLowModel, &m_aRatioHModel,
		&m_aRatioMModel, &m_aRatioLModel, &m_bRatioHModel, &m_bRatioMModel, &m_bRatioLModel,
		&m_atkHModel, &m_atkMModel, &m_atkLModel, &m_relHModel, &m_relMModel, &m_relLModel,
		&m_rmsTimeModel, &m_balanceModel};
	for (auto model : models) { model->setScaleLogarithmic(true); }
}


void LOMMControls::resetAllParameters()
{
	int choice = QMessageBox::question(m_view, "Clear Plugin Settings", "Are you sure you want to clear all parameters?\n(This wipes LOMM to a clean slate, not the default preset.)", QMessageBox::Yes | QMessageBox::No);
	if (choice != QMessageBox::Yes) { return; }
	
	// give the user a chance to beg LMMS for forgiveness
	addJournalCheckPoint();
	
	// This plugin's normal default values are fairly close to what they'd want in most applications.
	// The Init button is there so the user can start from a clean slate instead.
	// These are those values.
	setInitAndReset(m_depthModel, 1);
	setInitAndReset(m_timeModel, 1);
	setInitAndReset(m_inVolModel, 0);
	setInitAndReset(m_outVolModel, 0);
	setInitAndReset(m_upwardModel, 1);
	setInitAndReset(m_downwardModel, 1);
	setInitAndReset(m_split1Model, 2500);
	setInitAndReset(m_split2Model, 88);
	setInitAndReset(m_split1EnabledModel, true);
	setInitAndReset(m_split2EnabledModel, true);
	setInitAndReset(m_band1EnabledModel, true);
	setInitAndReset(m_band2EnabledModel, true);
	setInitAndReset(m_band3EnabledModel, true);
	setInitAndReset(m_inHighModel, 0);
	setInitAndReset(m_inMidModel, 0);
	setInitAndReset(m_inLowModel, 0);
	setInitAndReset(m_outHighModel, 0);
	setInitAndReset(m_outMidModel, 0);
	setInitAndReset(m_outLowModel, 0);
	setInitAndReset(m_aThreshHModel, m_aThreshHModel.maxValue());
	setInitAndReset(m_aThreshMModel, m_aThreshMModel.maxValue());
	setInitAndReset(m_aThreshLModel, m_aThreshLModel.maxValue());
	setInitAndReset(m_aRatioHModel, 1);
	setInitAndReset(m_aRatioMModel, 1);
	setInitAndReset(m_aRatioLModel, 1);
	setInitAndReset(m_bThreshHModel, m_bThreshHModel.minValue());
	setInitAndReset(m_bThreshMModel, m_bThreshMModel.minValue());
	setInitAndReset(m_bThreshLModel, m_bThreshLModel.minValue());
	setInitAndReset(m_bRatioHModel, 1);
	setInitAndReset(m_bRatioMModel, 1);
	setInitAndReset(m_bRatioLModel, 1);
	setInitAndReset(m_atkHModel, 13.5);
	setInitAndReset(m_atkMModel, 22.4);
	setInitAndReset(m_atkLModel, 47.8);
	setInitAndReset(m_relHModel, 132);
	setInitAndReset(m_relMModel, 282);
	setInitAndReset(m_relLModel, 282);
	setInitAndReset(m_rmsTimeModel, 10);
	setInitAndReset(m_kneeModel, 6);
	setInitAndReset(m_rangeModel, 36);
	setInitAndReset(m_balanceModel, 0);
	setInitAndReset(m_depthScalingModel, true);
	setInitAndReset(m_stereoLinkModel, false);
	setInitAndReset(m_autoTimeModel, 0);
	setInitAndReset(m_mixModel, 1);
	setInitAndReset(m_feedbackModel, false);
	setInitAndReset(m_midsideModel, false);
	setInitAndReset(m_lookaheadEnableModel, false);
	setInitAndReset(m_lookaheadModel, 0.f);
	setInitAndReset(m_lowSideUpwardSuppressModel, false);
}



void LOMMControls::loadSettings(const QDomElement& parent)
{
	m_depthModel.loadSettings(parent, "depth");
	m_timeModel.loadSettings(parent, "time");
	m_inVolModel.loadSettings(parent, "inVol");
	m_outVolModel.loadSettings(parent, "outVol");
	m_upwardModel.loadSettings(parent, "upward");
	m_downwardModel.loadSettings(parent, "downward");
	m_split1Model.loadSettings(parent, "split1");
	m_split2Model.loadSettings(parent, "split2");
	m_split1EnabledModel.loadSettings(parent, "split1Enabled");
	m_split2EnabledModel.loadSettings(parent, "split2Enabled");
	m_band1EnabledModel.loadSettings(parent, "band1Enabled");
	m_band2EnabledModel.loadSettings(parent, "band2Enabled");
	m_band3EnabledModel.loadSettings(parent, "band3Enabled");
	m_inHighModel.loadSettings(parent, "inHigh");
	m_inMidModel.loadSettings(parent, "inMid");
	m_inLowModel.loadSettings(parent, "inLow");
	m_outHighModel.loadSettings(parent, "outHigh");
	m_outMidModel.loadSettings(parent, "outMid");
	m_outLowModel.loadSettings(parent, "outLow");
	m_aThreshHModel.loadSettings(parent, "aThreshH");
	m_aThreshMModel.loadSettings(parent, "aThreshM");
	m_aThreshLModel.loadSettings(parent, "aThreshL");
	m_aRatioHModel.loadSettings(parent, "aRatioH");
	m_aRatioMModel.loadSettings(parent, "aRatioM");
	m_aRatioLModel.loadSettings(parent, "aRatioL");
	m_bThreshHModel.loadSettings(parent, "bThreshH");
	m_bThreshMModel.loadSettings(parent, "bThreshM");
	m_bThreshLModel.loadSettings(parent, "bThreshL");
	m_bRatioHModel.loadSettings(parent, "bRatioH");
	m_bRatioMModel.loadSettings(parent, "bRatioM");
	m_bRatioLModel.loadSettings(parent, "bRatioL");
	m_atkHModel.loadSettings(parent, "atkH");
	m_atkMModel.loadSettings(parent, "atkM");
	m_atkLModel.loadSettings(parent, "atkL");
	m_relHModel.loadSettings(parent, "relH");
	m_relMModel.loadSettings(parent, "relM");
	m_relLModel.loadSettings(parent, "relL");
	m_rmsTimeModel.loadSettings(parent, "rmsTime");
	m_kneeModel.loadSettings(parent, "knee");
	m_rangeModel.loadSettings(parent, "range");
	m_balanceModel.loadSettings(parent, "balance");
	m_depthScalingModel.loadSettings(parent, "depthScaling");
	m_stereoLinkModel.loadSettings(parent, "stereoLink");
	m_autoTimeModel.loadSettings(parent, "autoTime");
	m_mixModel.loadSettings(parent, "mix");
	m_feedbackModel.loadSettings(parent, "feedback");
	m_midsideModel.loadSettings(parent, "midside");
	m_lookaheadEnableModel.loadSettings(parent, "lookaheadEnable");
	m_lookaheadModel.loadSettings(parent, "lookahead");
	m_lowSideUpwardSuppressModel.loadSettings(parent, "lowSideUpwardSuppress");
}




void LOMMControls::saveSettings(QDomDocument& doc, QDomElement& parent)
{
	m_depthModel.saveSettings(doc, parent, "depth");
	m_timeModel.saveSettings(doc, parent, "time");
	m_inVolModel.saveSettings(doc, parent, "inVol");
	m_outVolModel.saveSettings(doc, parent, "outVol");
	m_upwardModel.saveSettings(doc, parent, "upward");
	m_downwardModel.saveSettings(doc, parent, "downward");
	m_split1Model.saveSettings(doc, parent, "split1");
	m_split2Model.saveSettings(doc, parent, "split2");
	m_split1EnabledModel.saveSettings(doc, parent, "split1Enabled");
	m_split2EnabledModel.saveSettings(doc, parent, "split2Enabled");
	m_band1EnabledModel.saveSettings(doc, parent, "band1Enabled");
	m_band2EnabledModel.saveSettings(doc, parent, "band2Enabled");
	m_band3EnabledModel.saveSettings(doc, parent, "band3Enabled");
	m_inHighModel.saveSettings(doc, parent, "inHigh");
	m_inMidModel.saveSettings(doc, parent, "inMid");
	m_inLowModel.saveSettings(doc, parent, "inLow");
	m_outHighModel.saveSettings(doc, parent, "outHigh");
	m_outMidModel.saveSettings(doc, parent, "outMid");
	m_outLowModel.saveSettings(doc, parent, "outLow");
	m_aThreshHModel.saveSettings(doc, parent, "aThreshH");
	m_aThreshMModel.saveSettings(doc, parent, "aThreshM");
	m_aThreshLModel.saveSettings(doc, parent, "aThreshL");
	m_aRatioHModel.saveSettings(doc, parent, "aRatioH");
	m_aRatioMModel.saveSettings(doc, parent, "aRatioM");
	m_aRatioLModel.saveSettings(doc, parent, "aRatioL");
	m_bThreshHModel.saveSettings(doc, parent, "bThreshH");
	m_bThreshMModel.saveSettings(doc, parent, "bThreshM");
	m_bThreshLModel.saveSettings(doc, parent, "bThreshL");
	m_bRatioHModel.saveSettings(doc, parent, "bRatioH");
	m_bRatioMModel.saveSettings(doc, parent, "bRatioM");
	m_bRatioLModel.saveSettings(doc, parent, "bRatioL");
	m_atkHModel.saveSettings(doc, parent, "atkH");
	m_atkMModel.saveSettings(doc, parent, "atkM");
	m_atkLModel.saveSettings(doc, parent, "atkL");
	m_relHModel.saveSettings(doc, parent, "relH");
	m_relMModel.saveSettings(doc, parent, "relM");
	m_relLModel.saveSettings(doc, parent, "relL");
	m_rmsTimeModel.saveSettings(doc, parent, "rmsTime");
	m_kneeModel.saveSettings(doc, parent, "knee");
	m_rangeModel.saveSettings(doc, parent, "range");
	m_balanceModel.saveSettings(doc, parent, "balance");
	m_depthScalingModel.saveSettings(doc, parent, "depthScaling");
	m_stereoLinkModel.saveSettings(doc, parent, "stereoLink");
	m_autoTimeModel.saveSettings(doc, parent, "autoTime");
	m_mixModel.saveSettings(doc, parent, "mix");
	m_feedbackModel.saveSettings(doc, parent, "feedback");
	m_midsideModel.saveSettings(doc, parent, "midside");
	m_lookaheadEnableModel.saveSettings(doc, parent, "lookaheadEnable");
	m_lookaheadModel.saveSettings(doc, parent, "lookahead");
	m_lowSideUpwardSuppressModel.saveSettings(doc, parent, "lowSideUpwardSuppress");
}


} // namespace lmms


