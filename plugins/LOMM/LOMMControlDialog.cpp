/*
 * LOMMControlDialog.cpp
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
#include "LOMMControlDialog.h"
#include "LOMMControls.h"

#include "embed.h"
#include "Knob.h"
#include "LcdFloatSpinBox.h"
#include "LcdSpinBox.h"


namespace lmms::gui
{

LOMMControlDialog::LOMMControlDialog(LOMMControls* controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(400, 256);

	Knob * depthKnob = new Knob(KnobType::Bright26, this);
	depthKnob->move(10, 4);
	depthKnob->setModel(&controls->m_depthModel);
	depthKnob->setHintText(tr("Depth:") , "");
	
	Knob * timeKnob = new Knob(KnobType::Bright26, this);
	timeKnob->move(10, 41);
	timeKnob->setModel(&controls->m_timeModel);
	timeKnob->setHintText(tr("Time:") , "");
	
	Knob * inVolKnob = new Knob(KnobType::Bright26, this);
	inVolKnob->move(10, 220);
	inVolKnob->setModel(&controls->m_inVolModel);
	inVolKnob->setHintText(tr("Input Volume:") , " dB");
	
	Knob * outVolKnob = new Knob(KnobType::Bright26, this);
	outVolKnob->move(363, 220);
	outVolKnob->setModel(&controls->m_outVolModel);
	outVolKnob->setHintText(tr("Output Volume:") , " dB");
	
	Knob * upwardKnob = new Knob(KnobType::Bright26, this);
	upwardKnob->move(10, 179);
	upwardKnob->setModel(&controls->m_upwardModel);
	upwardKnob->setHintText(tr("Upward Depth:") , "");
	
	Knob * downwardKnob = new Knob(KnobType::Bright26, this);
	downwardKnob->move(363, 179);
	downwardKnob->setModel(&controls->m_downwardModel);
	downwardKnob->setHintText(tr("Downward Depth:") , "");
	
	LcdFloatSpinBox * split1Spin = new LcdFloatSpinBox(5, 2, "11green", tr("High/Mid Crossover"), this);
	split1Spin->move(352, 76);
	split1Spin->setModel(&controls->m_split1Model);
	split1Spin->setToolTip(tr("High/Mid Crossover"));
	split1Spin->setSeamless(true, true);
	
	LcdFloatSpinBox * split2Spin = new LcdFloatSpinBox(5, 2, "11green", tr("Mid/Low Crossover"), this);
	split2Spin->move(352, 156);
	split2Spin->setModel(&controls->m_split2Model);
	split2Spin->setToolTip(tr("Mid/Low Crossover"));
	split2Spin->setSeamless(true, true);
	
	PixmapButton * split1EnabledButton = new PixmapButton(this, tr("High/mid band split"));
	split1EnabledButton->move(369, 104);
	split1EnabledButton->setCheckable(true);
	split1EnabledButton->setModel(&controls->m_split1EnabledModel);
	split1EnabledButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("crossover_led_green"));
	split1EnabledButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("crossover_led_off"));
	split1EnabledButton->setToolTip(tr("High/mid band split"));
	
	PixmapButton * split2EnabledButton = new PixmapButton(this, tr("Mid/low band split"));
	split2EnabledButton->move(369, 126);
	split2EnabledButton->setCheckable(true);
	split2EnabledButton->setModel(&controls->m_split2EnabledModel);
	split2EnabledButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("crossover_led_green"));
	split2EnabledButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("crossover_led_off"));
	split2EnabledButton->setToolTip(tr("Mid/low band split"));
	
	PixmapButton * band1EnabledButton = new PixmapButton(this, tr("Enable High Band"));
	band1EnabledButton->move(143, 66);
	band1EnabledButton->setCheckable(true);
	band1EnabledButton->setModel(&controls->m_band1EnabledModel);
	band1EnabledButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("high_band_active"));
	band1EnabledButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("high_band_inactive"));
	band1EnabledButton->setToolTip(tr("Enable High Band"));
	
	PixmapButton * band2EnabledButton = new PixmapButton(this, tr("Enable Mid Band"));
	band2EnabledButton->move(143, 146);
	band2EnabledButton->setCheckable(true);
	band2EnabledButton->setModel(&controls->m_band2EnabledModel);
	band2EnabledButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("mid_band_active"));
	band2EnabledButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("mid_band_inactive"));
	band2EnabledButton->setToolTip(tr("Enable Mid Band"));
	
	PixmapButton * band3EnabledButton = new PixmapButton(this, tr("Enable Low Band"));
	band3EnabledButton->move(143, 226);
	band3EnabledButton->setCheckable(true);
	band3EnabledButton->setModel(&controls->m_band3EnabledModel);
	band3EnabledButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("low_band_active"));
	band3EnabledButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("low_band_inactive"));
	band3EnabledButton->setToolTip(tr("Enable Low Band"));
	
	Knob * inHighKnob = new Knob(KnobType::Bright26, this);
	inHighKnob->move(53, 43);
	inHighKnob->setModel(&controls->m_inHighModel);
	inHighKnob->setHintText(tr("High Input Volume:") , " dB");
	
	Knob * inMidKnob = new Knob(KnobType::Bright26, this);
	inMidKnob->move(53, 123);
	inMidKnob->setModel(&controls->m_inMidModel);
	inMidKnob->setHintText(tr("Mid Input Volume:") , " dB");
	
	Knob * inLowKnob = new Knob(KnobType::Bright26, this);
	inLowKnob->move(53, 203);
	inLowKnob->setModel(&controls->m_inLowModel);
	inLowKnob->setHintText(tr("Low Input Volume:") , " dB");
	
	Knob * outHighKnob = new Knob(KnobType::Bright26, this);
	outHighKnob->move(320, 43);
	outHighKnob->setModel(&controls->m_outHighModel);
	outHighKnob->setHintText(tr("High Output Volume:") , " dB");
	
	Knob * outMidKnob = new Knob(KnobType::Bright26, this);
	outMidKnob->move(320, 123);
	outMidKnob->setModel(&controls->m_outMidModel);
	outMidKnob->setHintText(tr("Mid Output Volume:") , " dB");
	
	Knob * outLowKnob = new Knob(KnobType::Bright26, this);
	outLowKnob->move(320, 203);
	outLowKnob->setModel(&controls->m_outLowModel);
	outLowKnob->setHintText(tr("Low Output Volume:") , " dB");
	
	LcdFloatSpinBox * aThreshHSpin = new LcdFloatSpinBox(3, 3, "11green", tr("Above Threshold High"), this);
	aThreshHSpin->move(300, 13);
	aThreshHSpin->setModel(&controls->m_aThreshHModel);
	aThreshHSpin->setToolTip(tr("Above Threshold High"));
	aThreshHSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * aThreshMSpin = new LcdFloatSpinBox(3, 3, "11green", tr("Above Threshold Mid"), this);
	aThreshMSpin->move(300, 93);
	aThreshMSpin->setModel(&controls->m_aThreshMModel);
	aThreshMSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * aThreshLSpin = new LcdFloatSpinBox(3, 3, "11green", tr("Above Threshold Low"), this);
	aThreshLSpin->move(300, 173);
	aThreshLSpin->setModel(&controls->m_aThreshLModel);
	aThreshLSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * aRatioHSpin = new LcdFloatSpinBox(2, 2, "11green", tr("Above Ratio High"), this);
	aRatioHSpin->move(284, 44);
	aRatioHSpin->setModel(&controls->m_aRatioHModel);
	aRatioHSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * aRatioMSpin = new LcdFloatSpinBox(2, 2, "11green", tr("Above Ratio Mid"), this);
	aRatioMSpin->move(284, 124);
	aRatioMSpin->setModel(&controls->m_aRatioMModel);
	aRatioMSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * aRatioLSpin = new LcdFloatSpinBox(2, 2, "11green", tr("Above Ratio Low"), this);
	aRatioLSpin->move(284, 204);
	aRatioLSpin->setModel(&controls->m_aRatioLModel);
	aRatioLSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * bThreshHSpin = new LcdFloatSpinBox(3, 3, "11green", tr("Below Threshold High"), this);
	bThreshHSpin->move(59, 13);
	bThreshHSpin->setModel(&controls->m_bThreshHModel);
	bThreshHSpin->setToolTip(tr("Above Threshold High"));
	bThreshHSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * bThreshMSpin = new LcdFloatSpinBox(3, 3, "11green", tr("Below Threshold Mid"), this);
	bThreshMSpin->move(59, 93);
	bThreshMSpin->setModel(&controls->m_bThreshMModel);
	bThreshMSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * bThreshLSpin = new LcdFloatSpinBox(3, 3, "11green", tr("Below Threshold Low"), this);
	bThreshLSpin->move(59, 173);
	bThreshLSpin->setModel(&controls->m_bThreshLModel);
	bThreshLSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * bRatioHSpin = new LcdFloatSpinBox(2, 2, "11green", tr("Below Ratio High"), this);
	bRatioHSpin->move(87, 44);
	bRatioHSpin->setModel(&controls->m_bRatioHModel);
	bRatioHSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * bRatioMSpin = new LcdFloatSpinBox(2, 2, "11green", tr("Below Ratio Mid"), this);
	bRatioMSpin->move(87, 124);
	bRatioMSpin->setModel(&controls->m_bRatioMModel);
	bRatioMSpin->setSeamless(true, true);
	
	LcdFloatSpinBox * bRatioLSpin = new LcdFloatSpinBox(2, 2, "11green", tr("Below Ratio Low"), this);
	bRatioLSpin->move(87, 204);
	bRatioLSpin->setModel(&controls->m_bRatioLModel);
	bRatioLSpin->setSeamless(true, true);
	
	Knob * atkHKnob = new Knob(KnobType::Small17, this);
	atkHKnob->move(120, 61);
	atkHKnob->setModel(&controls->m_atkHModel);
	atkHKnob->setHintText(tr("Attack High:") , " ms");
	
	Knob * atkMKnob = new Knob(KnobType::Small17, this);
	atkMKnob->move(120, 141);
	atkMKnob->setModel(&controls->m_atkMModel);
	atkMKnob->setHintText(tr("Attack Mid:") , " ms");
	
	Knob * atkLKnob = new Knob(KnobType::Small17, this);
	atkLKnob->move(120, 221);
	atkLKnob->setModel(&controls->m_atkLModel);
	atkLKnob->setHintText(tr("Attack Low:") , " ms");
	
	Knob * relHKnob = new Knob(KnobType::Small17, this);
	relHKnob->move(261, 61);
	relHKnob->setModel(&controls->m_relHModel);
	relHKnob->setHintText(tr("Release High:") , " ms");
	
	Knob * relMKnob = new Knob(KnobType::Small17, this);
	relMKnob->move(261, 141);
	relMKnob->setModel(&controls->m_relMModel);
	relMKnob->setHintText(tr("Release Mid:") , " ms");
	
	Knob * relLKnob = new Knob(KnobType::Small17, this);
	relLKnob->move(261, 221);
	relLKnob->setModel(&controls->m_relLModel);
	relLKnob->setHintText(tr("Release Low:") , " ms");
	
	Knob * rmsTimeKnob = new Knob(KnobType::Small17, this);
	rmsTimeKnob->move(380, 42);
	rmsTimeKnob->setModel(&controls->m_rmsTimeModel);
	rmsTimeKnob->setHintText(tr("RMS Time:") , " ms");
	
	Knob * kneeKnob = new Knob(KnobType::Small17, this);
	kneeKnob->move(356, 42);
	kneeKnob->setModel(&controls->m_kneeModel);
	kneeKnob->setHintText(tr("Knee:") , " dB");
	
	Knob * rangeKnob = new Knob(KnobType::Small17, this);
	rangeKnob->move(24, 146);
	rangeKnob->setModel(&controls->m_rangeModel);
	rangeKnob->setHintText(tr("Range:") , " dB");
	
	Knob * balanceKnob = new Knob(KnobType::Small17, this);
	balanceKnob->move(13, 114);
	balanceKnob->setModel(&controls->m_balanceModel);
	balanceKnob->setHintText(tr("Balance:") , " dB");
	
	PixmapButton * depthScalingButton = new PixmapButton(this, tr("Scale output volume with Depth"));
	depthScalingButton->move(51, 0);
	depthScalingButton->setCheckable(true);
	depthScalingButton->setModel(&controls->m_depthScalingModel);
	depthScalingButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("depthScaling_active"));
	depthScalingButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("depthScaling_inactive"));
	depthScalingButton->setToolTip(tr("Scale output volume with Depth"));
	
	PixmapButton * stereoLinkButton = new PixmapButton(this, tr("Stereo Link"));
	stereoLinkButton->move(52, 237);
	stereoLinkButton->setCheckable(true);
	stereoLinkButton->setModel(&controls->m_stereoLinkModel);
	stereoLinkButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("stereoLink_active"));
	stereoLinkButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("stereoLink_inactive"));
	stereoLinkButton->setToolTip(tr("Stereo Link"));
	
	Knob * autoTimeKnob = new Knob(KnobType::Small17, this);
	autoTimeKnob->move(24, 80);
	autoTimeKnob->setModel(&controls->m_autoTimeModel);
	autoTimeKnob->setHintText(tr("Auto Time:") , "");
	
	Knob * mixKnob = new Knob(KnobType::Bright26, this);
	mixKnob->move(363, 4);
	mixKnob->setModel(&controls->m_mixModel);
	mixKnob->setHintText(tr("Mix:") , "");
	
	m_feedbackButton = new PixmapButton(this, tr("Feedback"));
	m_feedbackButton->move(317, 238);
	m_feedbackButton->setCheckable(true);
	m_feedbackButton->setModel(&controls->m_feedbackModel);
	m_feedbackButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("feedback_active"));
	m_feedbackButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("feedback_inactive"));
	m_feedbackButton->setToolTip(tr("Feedback"));
	
	PixmapButton * midsideButton = new PixmapButton(this, tr("Mid/Side"));
	midsideButton->move(285, 238);
	midsideButton->setCheckable(true);
	midsideButton->setModel(&controls->m_midsideModel);
	midsideButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("midside_active"));
	midsideButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("midside_inactive"));
	midsideButton->setToolTip(tr("Mid/Side"));
	
	PixmapButton * lookaheadEnableButton = new PixmapButton(this, tr("Lookahead"));
	lookaheadEnableButton->move(147, 0);
	lookaheadEnableButton->setCheckable(true);
	lookaheadEnableButton->setModel(&controls->m_lookaheadEnableModel);
	lookaheadEnableButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("lookahead_active"));
	lookaheadEnableButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("lookahead_inactive"));
	lookaheadEnableButton->setToolTip(tr("Lookahead"));
	
	LcdFloatSpinBox * lookaheadSpin = new LcdFloatSpinBox(2, 2, "11green", tr("Lookahead"), this);
	lookaheadSpin->move(214, 2);
	lookaheadSpin->setModel(&controls->m_lookaheadModel);
	lookaheadSpin->setSeamless(true, true);
	
	PixmapButton * initButton = new PixmapButton(this, tr("Reset all parameters"));
	initButton->move(84, 237);
	initButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("init_active"));
	initButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("init_inactive"));
	initButton->setToolTip(tr("Reset all parameters"));
	
	connect(initButton, SIGNAL(clicked()), m_controls, SLOT(resetAllParameters()));
	connect(&controls->m_lookaheadEnableModel, SIGNAL(dataChanged()), this, SLOT(updateFeedbackVisibility()));
	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(updateDisplay()));
	
	emit updateFeedbackVisibility();
}

void LOMMControlDialog::updateFeedbackVisibility()
{
	m_feedbackButton->setVisible(!m_controls->m_lookaheadEnableModel.value());
}

void LOMMControlDialog::updateDisplay()
{
	update();
}

void LOMMControlDialog::paintEvent(QPaintEvent *event)
{
	if (!isVisible())
	{
		return;
	}

	QPainter p;
	p.begin(this);

	// Draw threshold lines
	p.setPen(QPen(QColor(255, 255, 0, 255), 1));
	int aThreshHX = dbfsToX(m_controls->m_aThreshHModel.value());
	int aThreshMX = dbfsToX(m_controls->m_aThreshMModel.value());
	int aThreshLX = dbfsToX(m_controls->m_aThreshLModel.value());
	p.fillRect(aThreshHX, LOMM_DISPLAY_Y[0], LOMM_DISPLAY_X + LOMM_DISPLAY_WIDTH - aThreshHX, LOMM_DISPLAY_Y[1] + LOMM_DISPLAY_HEIGHT - LOMM_DISPLAY_Y[0], QColor(255, 255, 0, 31));
	p.fillRect(aThreshMX, LOMM_DISPLAY_Y[2], LOMM_DISPLAY_X + LOMM_DISPLAY_WIDTH - aThreshMX, LOMM_DISPLAY_Y[3] + LOMM_DISPLAY_HEIGHT - LOMM_DISPLAY_Y[2], QColor(255, 255, 0, 31));
	p.fillRect(aThreshLX, LOMM_DISPLAY_Y[4], LOMM_DISPLAY_X + LOMM_DISPLAY_WIDTH - aThreshLX, LOMM_DISPLAY_Y[5] + LOMM_DISPLAY_HEIGHT - LOMM_DISPLAY_Y[4], QColor(255, 255, 0, 31));
	p.drawLine(aThreshHX, LOMM_DISPLAY_Y[0], aThreshHX, LOMM_DISPLAY_Y[1] + LOMM_DISPLAY_HEIGHT);
	p.drawLine(aThreshMX, LOMM_DISPLAY_Y[2], aThreshMX, LOMM_DISPLAY_Y[3] + LOMM_DISPLAY_HEIGHT);
	p.drawLine(aThreshLX, LOMM_DISPLAY_Y[4], aThreshLX, LOMM_DISPLAY_Y[5] + LOMM_DISPLAY_HEIGHT);
	
	p.setPen(QPen(QColor(255, 0, 0, 255), 1));
	int bThreshHX = dbfsToX(m_controls->m_bThreshHModel.value());
	int bThreshMX = dbfsToX(m_controls->m_bThreshMModel.value());
	int bThreshLX = dbfsToX(m_controls->m_bThreshLModel.value());
	p.fillRect(LOMM_DISPLAY_X, LOMM_DISPLAY_Y[0], bThreshHX - LOMM_DISPLAY_X, LOMM_DISPLAY_Y[1] + LOMM_DISPLAY_HEIGHT - LOMM_DISPLAY_Y[0], QColor(255, 0, 0, 31));
	p.fillRect(LOMM_DISPLAY_X, LOMM_DISPLAY_Y[2], bThreshMX - LOMM_DISPLAY_X, LOMM_DISPLAY_Y[3] + LOMM_DISPLAY_HEIGHT - LOMM_DISPLAY_Y[2], QColor(255, 0, 0, 31));
	p.fillRect(LOMM_DISPLAY_X, LOMM_DISPLAY_Y[4], bThreshLX - LOMM_DISPLAY_X, LOMM_DISPLAY_Y[5] + LOMM_DISPLAY_HEIGHT - LOMM_DISPLAY_Y[4], QColor(255, 0, 0, 31));
	p.drawLine(bThreshHX, LOMM_DISPLAY_Y[0], bThreshHX, LOMM_DISPLAY_Y[1] + LOMM_DISPLAY_HEIGHT);
	p.drawLine(bThreshMX, LOMM_DISPLAY_Y[2], bThreshMX, LOMM_DISPLAY_Y[3] + LOMM_DISPLAY_HEIGHT);
	p.drawLine(bThreshLX, LOMM_DISPLAY_Y[4], bThreshLX, LOMM_DISPLAY_Y[5] + LOMM_DISPLAY_HEIGHT);
	
	// Draw input lines
	p.setPen(QPen(QColor(200, 200, 200, 80), 1));
	int inHL = dbfsToX(m_controls->m_effect->m_displayIn[0][0]);
	p.drawLine(inHL, LOMM_DISPLAY_Y[0] + 4, inHL, LOMM_DISPLAY_Y[0] + LOMM_DISPLAY_HEIGHT);
	int inHR = dbfsToX(m_controls->m_effect->m_displayIn[0][1]);
	p.drawLine(inHR, LOMM_DISPLAY_Y[1], inHR, LOMM_DISPLAY_Y[1] + LOMM_DISPLAY_HEIGHT - 4);
	int inML = dbfsToX(m_controls->m_effect->m_displayIn[1][0]);
	p.drawLine(inML, LOMM_DISPLAY_Y[2] + 4, inML, LOMM_DISPLAY_Y[2] + LOMM_DISPLAY_HEIGHT);
	int inMR = dbfsToX(m_controls->m_effect->m_displayIn[1][1]);
	p.drawLine(inMR, LOMM_DISPLAY_Y[3], inMR, LOMM_DISPLAY_Y[3] + LOMM_DISPLAY_HEIGHT - 4);
	int inLL = dbfsToX(m_controls->m_effect->m_displayIn[2][0]);
	p.drawLine(inLL, LOMM_DISPLAY_Y[4] + 4, inLL, LOMM_DISPLAY_Y[4] + LOMM_DISPLAY_HEIGHT);
	int inLR = dbfsToX(m_controls->m_effect->m_displayIn[2][1]);
	p.drawLine(inLR, LOMM_DISPLAY_Y[5], inLR, LOMM_DISPLAY_Y[5] + LOMM_DISPLAY_HEIGHT - 4);
	
	// Draw output lines
	p.setPen(QPen(QColor(255, 255, 255, 255), 1));
	int outHL = dbfsToX(m_controls->m_effect->m_displayOut[0][0]);
	p.drawLine(outHL, LOMM_DISPLAY_Y[0], outHL, LOMM_DISPLAY_Y[0] + LOMM_DISPLAY_HEIGHT);
	int outHR = dbfsToX(m_controls->m_effect->m_displayOut[0][1]);
	p.drawLine(outHR, LOMM_DISPLAY_Y[1], outHR, LOMM_DISPLAY_Y[1] + LOMM_DISPLAY_HEIGHT);
	int outML = dbfsToX(m_controls->m_effect->m_displayOut[1][0]);
	p.drawLine(outML, LOMM_DISPLAY_Y[2], outML, LOMM_DISPLAY_Y[2] + LOMM_DISPLAY_HEIGHT);
	int outMR = dbfsToX(m_controls->m_effect->m_displayOut[1][1]);
	p.drawLine(outMR, LOMM_DISPLAY_Y[3], outMR, LOMM_DISPLAY_Y[3] + LOMM_DISPLAY_HEIGHT);
	int outLL = dbfsToX(m_controls->m_effect->m_displayOut[2][0]);
	p.drawLine(outLL, LOMM_DISPLAY_Y[4], outLL, LOMM_DISPLAY_Y[4] + LOMM_DISPLAY_HEIGHT);
	int outLR = dbfsToX(m_controls->m_effect->m_displayOut[2][1]);
	p.drawLine(outLR, LOMM_DISPLAY_Y[5], outLR, LOMM_DISPLAY_Y[5] + LOMM_DISPLAY_HEIGHT);

	p.end();
}

int LOMMControlDialog::dbfsToX(float dbfs)
{
	float returnX = (dbfs - LOMM_DISPLAY_MIN) / (LOMM_DISPLAY_MAX - LOMM_DISPLAY_MIN);
	returnX = qBound(LOMM_DISPLAY_X, LOMM_DISPLAY_X + returnX * LOMM_DISPLAY_WIDTH, LOMM_DISPLAY_X + LOMM_DISPLAY_WIDTH);
	return returnX;
}

float LOMMControlDialog::xToDbfs(int x)
{
	float xNorm = float(x - LOMM_DISPLAY_X) / LOMM_DISPLAY_WIDTH;
	float dbfs = xNorm * (LOMM_DISPLAY_MAX - LOMM_DISPLAY_MIN) + LOMM_DISPLAY_MIN;
	return dbfs;
}

void LOMMControlDialog::mousePressEvent(QMouseEvent* event)
{
	if ((event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton) && !(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)))
	{
		const QPoint& p = event->pos();

		if (LOMM_DISPLAY_X - 10 <= p.x() && p.x() <= LOMM_DISPLAY_X + LOMM_DISPLAY_WIDTH + 10)
		{
			FloatModel* aThresh[] = {&m_controls->m_aThreshHModel, &m_controls->m_aThreshMModel, &m_controls->m_aThreshLModel};
			FloatModel* bThresh[] = {&m_controls->m_bThreshHModel, &m_controls->m_bThreshMModel, &m_controls->m_bThreshLModel};

			for (int i = 0; i < 3; ++i)
			{
				if (LOMM_DISPLAY_Y[i * 2] <= p.y() && p.y() <= LOMM_DISPLAY_Y[i * 2 + 1] + LOMM_DISPLAY_HEIGHT)
				{
					int behavior = (p.x() < dbfsToX(bThresh[i]->value())) ? 0 : (p.x() > dbfsToX(aThresh[i]->value())) ? 1 : 2;
					if (event->button() == Qt::MiddleButton)
					{
						if (behavior == 0 || behavior == 2) {bThresh[i]->reset();}
						if (behavior == 1 || behavior == 2) {aThresh[i]->reset();}
						return;
					}

					m_bandDrag = i;
					m_lastMousePos = p;
					m_buttonPressed = true;

					m_dragType = behavior;
					return;
				}
			}
		}
	}
}

void LOMMControlDialog::mouseMoveEvent(QMouseEvent * event)
{
	if(m_buttonPressed && event->pos() != m_lastMousePos)
	{
		const float distance = event->pos().x() - m_lastMousePos.x();
		float dbDistance = distance * LOMM_DISPLAY_DB_PER_PIXEL;
		m_lastMousePos = event->pos();
		
		FloatModel* aModel[] = {&m_controls->m_aThreshHModel, &m_controls->m_aThreshMModel, &m_controls->m_aThreshLModel};
		FloatModel* bModel[] = {&m_controls->m_bThreshHModel, &m_controls->m_bThreshMModel, &m_controls->m_bThreshLModel};
		
		float bVal = bModel[m_bandDrag]->value();
		float aVal = aModel[m_bandDrag]->value();
		if (m_dragType == 0)
		{
			bModel[m_bandDrag]->setValue(bVal + dbDistance);
		}
		else if (m_dragType == 1)
		{
			aModel[m_bandDrag]->setValue(aVal + dbDistance);
		}
		else
		{
			dbDistance = qBound(bModel[m_bandDrag]->minValue(), bVal + dbDistance, bModel[m_bandDrag]->maxValue()) - bVal;
			dbDistance = qBound(aModel[m_bandDrag]->minValue(), aVal + dbDistance, aModel[m_bandDrag]->maxValue()) - aVal;
			bModel[m_bandDrag]->setValue(bVal + dbDistance);
			aModel[m_bandDrag]->setValue(aVal + dbDistance);
		}
	}
}

void LOMMControlDialog::mouseReleaseEvent(QMouseEvent* event)
{
	if(event && event->button() == Qt::LeftButton)
	{
		m_buttonPressed = false;
	}
}


} // namespace lmms::gui
