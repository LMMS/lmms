/*
 * PhaserControlDialog.cpp
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

#include <QLayout>

#include "PhaserEffect.h"
#include "PhaserControlDialog.h"
#include "PhaserControls.h"
#include "embed.h"
#include "LedCheckbox.h"
#include "TempoSyncKnob.h"
#include "PixmapButton.h"
#include "../Eq/EqFader.h"
#include "lmms_math.h"
#include "ToolTip.h"


constexpr int PHA_DOT_SLIDER_LENGTH = 338;
constexpr float PHA_MIN_FREQ = 20;
constexpr float PHA_MAX_FREQ = 20000;


PhaserControlDialog::PhaserControlDialog(PhaserControls* controls) :
	EffectControlDialog(controls),
	m_controls(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(371, 155);

	Knob * cutoffKnob = new Knob(knobBright_26, this);
	cutoffKnob -> move(65, 13);
	cutoffKnob->setModel(&controls->m_cutoffModel);
	cutoffKnob->setHintText(tr("Cutoff:"), " Hz");
	ToolTip::add(cutoffKnob, tr("Center frequency of allpass filters"));

	Knob * resonanceKnob = new Knob(knobBright_26, this);
	resonanceKnob -> move(103, 13);
	resonanceKnob->setModel(&controls->m_resonanceModel);
	resonanceKnob->setHintText(tr("Resonance:"), "");
	ToolTip::add(resonanceKnob, tr("Resonance of allpass filters"));

	Knob * feedbackKnob = new Knob(knobBright_26, this);
	feedbackKnob -> move(159, 13);
	feedbackKnob->setModel(&controls->m_feedbackModel);
	feedbackKnob->setHintText(tr("Feedback:"), "%");
	ToolTip::add(feedbackKnob, tr("Feedback amount for allpass filters"));

	LcdSpinBox * m_orderBox = new LcdSpinBox(2, this, "Order");
	m_orderBox->setModel(&controls->m_orderModel);
	m_orderBox->move(241, 20);
	ToolTip::add(m_orderBox, tr("Number of allpass filters"));

	Knob * delayKnob = new Knob(knobBright_26, this);
	delayKnob -> move(197, 13);
	delayKnob->setModel(&controls->m_delayModel);
	delayKnob->setHintText(tr("Delay:"), " ms");
	ToolTip::add(delayKnob, tr("Delay length of allpass filter feedback"));

	TempoSyncKnob * rateKnob = new TempoSyncKnob(knobBright_26, this);
	rateKnob -> move(104, 84);
	rateKnob->setModel(&controls->m_rateModel);
	rateKnob->setHintText(tr("Rate:"), " Sec");
	ToolTip::add(rateKnob, tr("LFO frequency"));

	Knob * amountKnob = new Knob(knobBright_26, this);
	amountKnob -> move(66, 84);
	amountKnob->setModel(&controls->m_amountModel);
	amountKnob->setHintText(tr("Amount:"), " octaves");
	ToolTip::add(amountKnob, tr("LFO amplitude"));

	Knob * phaseKnob = new Knob(knobBright_26, this);
	phaseKnob -> move(142, 84);
	phaseKnob->setModel(&controls->m_phaseModel);
	phaseKnob->setHintText(tr("Phase:"), " degrees");
	ToolTip::add(phaseKnob, tr("LFO stereo phase"));

	Knob * inFollowKnob = new Knob(knobBright_26, this);
	inFollowKnob -> move(203, 84);
	inFollowKnob->setModel(&controls->m_inFollowModel);
	inFollowKnob->setHintText(tr("Input Following:"), " octaves");
	ToolTip::add(inFollowKnob, tr("Input follower amplitude"));

	Knob * attackKnob = new Knob(knobBright_26, this);
	attackKnob -> move(241, 84);
	attackKnob->setModel(&controls->m_attackModel);
	attackKnob->setHintText(tr("Attack:"), " ms");
	ToolTip::add(attackKnob, tr("Input follower attack"));

	Knob * releaseKnob = new Knob(knobBright_26, this);
	releaseKnob -> move(279, 84);
	releaseKnob->setModel(&controls->m_releaseModel);
	releaseKnob->setHintText(tr("Release:"), " ms");
	ToolTip::add(releaseKnob, tr("Input follower release"));

	Knob * distortionKnob = new Knob(knobSmall_17, this);
	distortionKnob -> move(292, 26);
	distortionKnob->setModel(&controls->m_distortionModel);
	distortionKnob->setHintText(tr("Distortion:"), "%");
	ToolTip::add(distortionKnob, tr("Feedback rectifier"));

	QPixmap cutoffDotLeftImg = PLUGIN_NAME::getIconPixmap("cutoffDotLeft");
	m_cutoffDotLeftLabel = new QLabel(this);
	m_cutoffDotLeftLabel->setPixmap(cutoffDotLeftImg);
	m_cutoffDotLeftLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_cutoffDotLeftLabel->move(182, 139);

	QPixmap cutoffDotRightImg = PLUGIN_NAME::getIconPixmap("cutoffDotRight");
	m_cutoffDotRightLabel = new QLabel(this);
	m_cutoffDotRightLabel->setPixmap(cutoffDotRightImg);
	m_cutoffDotRightLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_cutoffDotRightLabel->move(182, 139);

	LedCheckBox * enableLFO = new LedCheckBox("", this, tr("Enable LFO"), LedCheckBox::Green);
	enableLFO->setModel(&controls->m_enableLFOModel);
	enableLFO->move(160, 63);
	ToolTip::add(enableLFO, tr("Enable LFO"));

	PixmapButton * invertButton = new PixmapButton(this, tr("Invert wet signal"));
	invertButton->move(276, 6);
	invertButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("invert_active"));
	invertButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("invert_inactive"));
	invertButton->setCheckable(true);
	invertButton->setModel(&controls->m_invertModel);
	ToolTip::add(invertButton, tr("Invert wet signal"));

	PixmapButton * wetButton = new PixmapButton(this, tr("Wet"));
	wetButton->move(242, 6);
	wetButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("wet_active"));
	wetButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("wet_inactive"));
	wetButton->setCheckable(true);
	wetButton->setModel(&controls->m_wetModel);
	ToolTip::add(wetButton, tr("Isolate wet signal"));

	EqFader * outFader = new EqFader(&controls->m_outGainModel,tr("Output gain"),
		this, &controls->m_outPeakL, &controls->m_outPeakR);
	outFader->setMinimumHeight(84);
	outFader->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	outFader->resize(23, 84);
	outFader->move(336, 33);
	outFader->setDisplayConversion(false);
	outFader->setHintText(tr("Gain"), "dBFS");
	ToolTip::add(outFader, tr("Output gain"));

	EqFader * inFader = new EqFader(&controls->m_inGainModel,tr("Input gain"),
		this, &controls->m_inPeakL, &controls->m_inPeakR);
	inFader->setMinimumHeight(84);
	inFader->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	inFader->resize(23, 84);
	inFader->move(13, 33);
	inFader->setDisplayConversion(false);
	inFader->setHintText(tr("Gain"), "dBFS");
	ToolTip::add(inFader, tr("Input gain"));

	connect(gui->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(updateSliders()));
}


void PhaserControlDialog::updateSliders()
{
	// Magic. Do not touch.
	m_cutoffDotLeftLabel->move((int((log2(m_controls->m_effect->getCutoff(0)) - log2(PHA_MIN_FREQ)) /
		log2(PHA_MAX_FREQ / PHA_MIN_FREQ) * PHA_DOT_SLIDER_LENGTH)) + 12, 139);
	m_cutoffDotRightLabel->move((int((log2(m_controls->m_effect->getCutoff(1)) - log2(PHA_MIN_FREQ)) /
		log2(PHA_MAX_FREQ / PHA_MIN_FREQ) * PHA_DOT_SLIDER_LENGTH)) + 12, 139);
}

