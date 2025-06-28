/*
 * PredictionDistortControlDialog.cpp - control dialog for PredictionDistort effect
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

#include "PredictionDistortControlDialog.h"
#include "PredictionDistortControls.h"

#include "embed.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "PixmapButton.h"

namespace lmms::gui
{

PredictionDistortControlDialog::PredictionDistortControlDialog(PredictionDistortControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(225, 60);
	
	Knob* mixKnob = new Knob(KnobType::Bright26, this);
	mixKnob->move(5, 8);
	mixKnob->setModel(&controls->m_mixModel);
	mixKnob->setLabel(tr("MIX"));
	mixKnob->setHintText(tr("Mix:") , "");

	LcdSpinBox* decayBox = new LcdSpinBox(3, this, "Amount");
	decayBox->setModel(&controls->m_decayModel);
	decayBox->move(55, 10);
	decayBox->setLabel(tr("DECAY"));
	decayBox->setToolTip(tr("How many passes to do with this filter"));

	LcdSpinBox* rangeBox = new LcdSpinBox(3, this, "Amount");
	rangeBox->setModel(&controls->m_rangeModel);
	rangeBox->move(105, 10);
	rangeBox->setLabel(tr("RANGE"));
	rangeBox->setToolTip(tr("How wide should the samples come from"));
	
	PixmapButton* isReversedButton = new PixmapButton(this, tr("Reverse"));
	isReversedButton->move(160, 16);
	isReversedButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("reverse_on"));
	isReversedButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("reverse_off"));
	isReversedButton->setCheckable(true);
	isReversedButton->setModel(&controls->m_isReverseModel);
	isReversedButton->setToolTip(tr("Should the reverse value have the biggest weight?"));
}

} // namespace lmms::gui
