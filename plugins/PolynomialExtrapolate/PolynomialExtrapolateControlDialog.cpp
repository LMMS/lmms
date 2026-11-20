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

#include <QHBoxLayout>

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
	setFixedSize(400, 60);
	
	QHBoxLayout *layout = new QHBoxLayout(this);

	Knob* mixKnob = new Knob(KnobType::Bright26, nullptr);
	mixKnob->setModel(&controls->m_mixModel);
	mixKnob->setLabel(tr("MIX"));
	mixKnob->setHintText(tr("Mix:") , "");
	layout->addWidget(mixKnob);

	LcdSpinBox* degreeBox = new LcdSpinBox(3, nullptr, "Amount");
	degreeBox->setModel(&controls->m_degreeModel);
	degreeBox->setLabel(tr("DEGREE"));
	degreeBox->setToolTip(tr("What degree of polynomial to use"));
	layout->addWidget(degreeBox);

	LcdSpinBox* gapBox = new LcdSpinBox(3, nullptr, "Amount");
	gapBox->setModel(&controls->m_gapModel);
	gapBox->setLabel(tr("GAP"));
	gapBox->setToolTip(tr("How many samples to skip between sample points"));
	layout->addWidget(gapBox);
	
	LcdSpinBox* predictionCountBox = new LcdSpinBox(3, nullptr, "Amount");
	predictionCountBox->setModel(&controls->m_predictionCountModel);
	predictionCountBox->setLabel(tr("PREDICT"));
	predictionCountBox->setToolTip(tr("How many samples to predict"));
	layout->addWidget(predictionCountBox);

	Knob* xMultiplierKnob = new Knob(KnobType::Bright26, nullptr);
	xMultiplierKnob->setModel(&controls->m_xMultiplierModel);
	xMultiplierKnob->setLabel(tr("X"));
	xMultiplierKnob->setHintText(tr("X multiplier:") , "");
	layout->addWidget(xMultiplierKnob);

	Knob* feedbackKnob = new Knob(KnobType::Bright26, nullptr);
	feedbackKnob->setModel(&controls->m_feedbackModel);
	feedbackKnob->setLabel(tr("Feedback"));
	feedbackKnob->setHintText(tr("Feedback") , "%");
	layout->addWidget(feedbackKnob);

	Knob* reuseKnob = new Knob(KnobType::Bright26, nullptr);
	reuseKnob->setModel(&controls->m_reusePercentModel);
	reuseKnob->setLabel(tr("Offset"));
	reuseKnob->setHintText(tr("Offsets the sampling points to past samples, interpolates instead of exterpolates") , "%");
	layout->addWidget(reuseKnob);

	Knob* volumeKnob = new Knob(KnobType::Bright26, nullptr);
	volumeKnob->move(355, 10);
	volumeKnob->setModel(&controls->m_volumeModel);
	volumeKnob->setLabel(tr("Volume"));
	volumeKnob->setHintText(tr("Volume") , "");
	layout->addWidget(volumeKnob);
}

} // namespace lmms::gui
