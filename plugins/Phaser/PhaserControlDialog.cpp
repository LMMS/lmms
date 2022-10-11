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

#include "PhaserControlDialog.h"
#include "PhaserControls.h"
#include "PhaserEffect.h"

#include "../Eq/EqFader.h"
#include "embed.h"
#include "gui_templates.h"
#include "LedCheckBox.h"
#include "lmms_math.h"
#include "PixmapButton.h"
#include "TempoSyncKnob.h"

namespace lmms::gui
{

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
	setFixedSize(370, 200);

	Knob * cutoffKnob = new Knob(knobBright_26, this);
	cutoffKnob -> move(64, 50);
	cutoffKnob->setModel(&controls->m_cutoffModel);
	cutoffKnob->setHintText(tr("Cutoff:"), " Hz");
	cutoffKnob->setToolTip(tr("Center frequency of allpass filters"));

	Knob * resonanceKnob = new Knob(knobBright_26, this);
	resonanceKnob -> move(102, 50);
	resonanceKnob->setModel(&controls->m_resonanceModel);
	resonanceKnob->setHintText(tr("Resonance:"), "");
	resonanceKnob->setToolTip(tr("Resonance of allpass filters"));

	Knob * feedbackKnob = new Knob(knobBright_26, this);
	feedbackKnob -> move(158, 50);
	feedbackKnob->setModel(&controls->m_feedbackModel);
	feedbackKnob->setHintText(tr("Feedback:"), "%");
	feedbackKnob->setToolTip(tr("Feedback amount for allpass filters"));

	LcdSpinBox * m_orderBox = new LcdSpinBox(2, this, "Order");
	m_orderBox->setModel(&controls->m_orderModel);
	m_orderBox->move(247, 61);
	m_orderBox->setToolTip(tr("Number of allpass filters"));

	Knob * delayKnob = new Knob(knobBright_26, this);
	delayKnob -> move(196, 50);
	delayKnob->setModel(&controls->m_delayModel);
	delayKnob->setHintText(tr("Delay:"), " ms");
	delayKnob->setToolTip(tr("Delay length of allpass filter feedback"));

	TempoSyncKnob * rateKnob = new TempoSyncKnob(knobBright_26, this);
	rateKnob -> move(103, 126);
	rateKnob->setModel(&controls->m_rateModel);
	rateKnob->setHintText(tr("Rate:"), " Sec");
	rateKnob->setToolTip(tr("LFO frequency"));

	Knob * amountKnob = new Knob(knobBright_26, this);
	amountKnob -> move(65, 126);
	amountKnob->setModel(&controls->m_amountModel);
	amountKnob->setHintText(tr("Amount:"), " octaves");
	amountKnob->setToolTip(tr("LFO amplitude"));

	Knob * phaseKnob = new Knob(knobBright_26, this);
	phaseKnob -> move(141, 126);
	phaseKnob->setModel(&controls->m_phaseModel);
	phaseKnob->setHintText(tr("Phase:"), " degrees");
	phaseKnob->setToolTip(tr("LFO stereo phase"));

	Knob * inFollowKnob = new Knob(knobBright_26, this);
	inFollowKnob -> move(202, 126);
	inFollowKnob->setModel(&controls->m_inFollowModel);
	inFollowKnob->setHintText(tr("Input Following:"), " octaves");
	inFollowKnob->setToolTip(tr("Input follower amplitude"));

	Knob * attackKnob = new Knob(knobBright_26, this);
	attackKnob -> move(240, 126);
	attackKnob->setModel(&controls->m_attackModel);
	attackKnob->setHintText(tr("Attack:"), " ms");
	attackKnob->setToolTip(tr("Input follower attack"));

	Knob * releaseKnob = new Knob(knobBright_26, this);
	releaseKnob -> move(278, 126);
	releaseKnob->setModel(&controls->m_releaseModel);
	releaseKnob->setHintText(tr("Release:"), " ms");
	releaseKnob->setToolTip(tr("Input follower release"));

	Knob * distortionKnob = new Knob(knobSmall_17, this);
	distortionKnob -> move(292, 63);
	distortionKnob->setModel(&controls->m_distortionModel);
	distortionKnob->setHintText(tr("Distortion:"), "%");
	distortionKnob->setToolTip(tr("Feedback rectifier"));

	Knob * analogDistKnob = new Knob(knobSmall_17, this);
	analogDistKnob -> move(243, 8);
	analogDistKnob->setModel(&controls->m_analogDistModel);
	analogDistKnob->setHintText(tr("Analog Distortion:"), "x");
	analogDistKnob->setToolTip(tr("Analog distortion amount"));

	Knob * delayControlKnob = new Knob(knobSmall_17, this);
	delayControlKnob -> move(344, 8);
	delayControlKnob->setModel(&controls->m_delayControlModel);
	delayControlKnob->setHintText(tr("Delay Control:"), "x");
	delayControlKnob->setToolTip(tr("Delay control amount"));

	Knob * cutoffControlKnob = new Knob(knobSmall_17, this);
	cutoffControlKnob -> move(300, 8);
	cutoffControlKnob->setModel(&controls->m_cutoffControlModel);
	cutoffControlKnob->setHintText(tr("Cutoff Control:"), "x");
	cutoffControlKnob->setToolTip(tr("Cutoff control amount"));

	ComboBox * m_modeBox = new ComboBox(this);
	m_modeBox->setGeometry(6, 6, 74, 22);
	m_modeBox->setFont(pointSize<8>(m_modeBox->font()));
	m_modeBox->setModel(&m_controls->m_modeModel);
	m_modeBox->setToolTip(tr("Change Phaser feedback circuit"));

	QPixmap cutoffDotLeftImg = PLUGIN_NAME::getIconPixmap("cutoffDotLeft");
	m_cutoffDotLeftLabel = new QLabel(this);
	m_cutoffDotLeftLabel->setPixmap(cutoffDotLeftImg);
	m_cutoffDotLeftLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_cutoffDotLeftLabel->move(182, 183);

	QPixmap cutoffDotRightImg = PLUGIN_NAME::getIconPixmap("cutoffDotRight");
	m_cutoffDotRightLabel = new QLabel(this);
	m_cutoffDotRightLabel->setPixmap(cutoffDotRightImg);
	m_cutoffDotRightLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
	m_cutoffDotRightLabel->move(182, 183);

	LedCheckBox * enableLFO = new LedCheckBox("", this, tr("Enable LFO"), LedCheckBox::Green);
	enableLFO->setModel(&controls->m_enableLFOModel);
	enableLFO->move(159, 104);
	enableLFO->setToolTip(tr("Enable LFO"));

	PixmapButton * invertButton = new PixmapButton(this, tr("Invert wet signal"));
	invertButton->move(241, 42);
	invertButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("invert_active"));
	invertButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("invert_inactive"));
	invertButton->setCheckable(true);
	invertButton->setModel(&controls->m_invertModel);
	invertButton->setToolTip(tr("Invert wet signal"));

	PixmapButton * wetButton = new PixmapButton(this, tr("Wet"));
	wetButton->move(287, 42);
	wetButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("wet_active"));
	wetButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("wet_inactive"));
	wetButton->setCheckable(true);
	wetButton->setModel(&controls->m_wetModel);
	wetButton->setToolTip(tr("Isolate wet signal"));

	PixmapButton * analogButton = new PixmapButton(this, tr("Analog"));
	analogButton->move(188, 9);
	analogButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("analog_active"));
	analogButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("analog_inactive"));
	analogButton->setCheckable(true);
	analogButton->setModel(&controls->m_analogModel);
	analogButton->setToolTip(tr("Saturate feedback signal"));

	PixmapButton * doubleButton = new PixmapButton(this, tr("Double"));
	doubleButton->move(88, 9);
	doubleButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("double_active"));
	doubleButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("double_inactive"));
	doubleButton->setCheckable(true);
	doubleButton->setModel(&controls->m_doubleModel);
	doubleButton->setToolTip(tr("Send through allpass filters twice"));

	PixmapButton * aliasButton = new PixmapButton(this, tr("Alias"));
	aliasButton->move(143, 9);
	aliasButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("alias_active"));
	aliasButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("alias_inactive"));
	aliasButton->setCheckable(true);
	aliasButton->setModel(&controls->m_aliasModel);
	aliasButton->setToolTip(tr("Linearly invert spectrum before/after allpass filters"));

	EqFader * outFader = new EqFader(&controls->m_outGainModel,tr("Output gain"),
		this, &controls->m_outPeakL, &controls->m_outPeakR);
	outFader->setMinimumHeight(84);
	outFader->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	outFader->resize(23, 94);
	outFader->move(333, 67);
	outFader->setDisplayConversion(false);
	outFader->setHintText(tr("Gain"), "dBFS");
	outFader->setToolTip(tr("Output gain"));

	EqFader * inFader = new EqFader(&controls->m_inGainModel,tr("Input gain"),
		this, &controls->m_inPeakL, &controls->m_inPeakR);
	inFader->setMinimumHeight(84);
	inFader->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	inFader->resize(23, 94);
	inFader->move(14, 67);
	inFader->setDisplayConversion(false);
	inFader->setHintText(tr("Gain"), "dBFS");
	inFader->setToolTip(tr("Input gain"));

	connect(getGUI()->mainWindow(), SIGNAL(periodicUpdate()), this, SLOT(updateSliders()));
}


void PhaserControlDialog::updateSliders()
{
	// Magic. Do not touch.
	m_cutoffDotLeftLabel->move((int((log2(m_controls->m_effect->getCutoff(0)) - log2(PHA_MIN_FREQ)) /
		log2(PHA_MAX_FREQ / PHA_MIN_FREQ) * PHA_DOT_SLIDER_LENGTH)) + 12, 183);
	m_cutoffDotRightLabel->move((int((log2(m_controls->m_effect->getCutoff(1)) - log2(PHA_MIN_FREQ)) /
		log2(PHA_MAX_FREQ / PHA_MIN_FREQ) * PHA_DOT_SLIDER_LENGTH)) + 12, 183);
}

}

