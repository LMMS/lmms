/*
 * PolynomialExtrapolateControlDialog.cpp - control dialog for PolynomialExtrapolate effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "PolynomialExtrapolateControlDialog.h"
#include "PolynomialExtrapolateControls.h"

#include "embed.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "PixmapButton.h"

namespace lmms::gui
{

PolynomialExtrapolateControlDialog::PolynomialExtrapolateControlDialog(PolynomialExtrapolateControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(500, 60);
	
	Knob* mixKnob = new Knob(KnobType::Bright26, this);
	mixKnob->move(5, 8);
	mixKnob->setModel(&controls->m_mixModel);
	mixKnob->setLabel(tr("MIX"));
	mixKnob->setHintText(tr("Mix:") , "");

	LcdSpinBox* degreeBox = new LcdSpinBox(3, this, "Amount");
	degreeBox->setModel(&controls->m_degreeModel);
	degreeBox->move(55, 10);
	degreeBox->setLabel(tr("DEGREE"));
	degreeBox->setToolTip(tr("What degree of polynomial to use"));

	LcdSpinBox* gapBox = new LcdSpinBox(3, this, "Amount");
	gapBox->setModel(&controls->m_gapModel);
	gapBox->move(105, 10);
	gapBox->setLabel(tr("GAP"));
	gapBox->setToolTip(tr("How many samples to skip between sample points"));
	
	/*
	PixmapButton* isReversedButton = new PixmapButton(this, tr("Reverse"));
	isReversedButton->move(160, 16);
	isReversedButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("reverse_on"));
	isReversedButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("reverse_off"));
	isReversedButton->setCheckable(true);
	isReversedButton->setModel(&controls->m_isReverseModel);
	isReversedButton->setToolTip(tr("Should the reverse value have the biggest weight?"));
	*/

	LcdSpinBox* predictionCountBox = new LcdSpinBox(3, this, "Amount");
	predictionCountBox->setModel(&controls->m_predictionCountModel);
	predictionCountBox->move(155, 10);
	predictionCountBox->setLabel(tr("PREDICT"));
	predictionCountBox->setToolTip(tr("How many samples to predict"));

	Knob* xMultiplierKnob = new Knob(KnobType::Bright26, this);
	xMultiplierKnob->move(190, 10);
	xMultiplierKnob->setModel(&controls->m_xMultiplierModel);
	xMultiplierKnob->setLabel(tr("X"));
	xMultiplierKnob->setHintText(tr("X multiplier:") , "");

	Knob* feedbackKnob = new Knob(KnobType::Bright26, this);
	feedbackKnob->move(260, 10);
	feedbackKnob->setModel(&controls->m_feedbackModel);
	feedbackKnob->setLabel(tr("Feedback"));
	feedbackKnob->setHintText(tr("Feedback") , "%");

	Knob* reuseKnob = new Knob(KnobType::Bright26, this);
	reuseKnob->move(300, 10);
	reuseKnob->setModel(&controls->m_reusePercentModel);
	reuseKnob->setLabel(tr("Reuse"));
	reuseKnob->setHintText(tr("Reuses ") , "%");

	Knob* volumeKnob = new Knob(KnobType::Bright26, this);
	volumeKnob->move(350, 10);
	volumeKnob->setModel(&controls->m_volumeModel);
	volumeKnob->setLabel(tr("Volume"));
	volumeKnob->setHintText(tr("Volume") , "");
}

} // namespace lmms::gui
