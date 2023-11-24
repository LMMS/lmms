/*
 * DispersionControlDialog.cpp
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


#include "DispersionControlDialog.h"
#include "DispersionControls.h"

#include "embed.h"
#include "Knob.h"
#include "LcdSpinBox.h"
#include "PixmapButton.h"


namespace lmms::gui
{


DispersionControlDialog::DispersionControlDialog(DispersionControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(207, 50);

	LcdSpinBox * m_amountBox = new LcdSpinBox(3, this, "Amount");
	m_amountBox->setModel(&controls->m_amountModel);
	m_amountBox->move(5, 10);
	m_amountBox->setLabel(tr("AMOUNT"));
	m_amountBox->setToolTip(tr("Number of all-pass filters"));
	
	Knob * freqKnob = new Knob(KnobType::Bright26, this);
	freqKnob->move(59, 8);
	freqKnob->setModel(&controls->m_freqModel);
	freqKnob->setLabel(tr("FREQ"));
	freqKnob->setHintText(tr("Frequency:") , " Hz");
	
	Knob * resoKnob = new Knob(KnobType::Bright26, this);
	resoKnob->move(99, 8);
	resoKnob->setModel(&controls->m_resoModel);
	resoKnob->setLabel(tr("RESO"));
	resoKnob->setHintText(tr("Resonance:") , " octaves");
	
	Knob * feedbackKnob = new Knob(KnobType::Bright26, this);
	feedbackKnob->move(139, 8);
	feedbackKnob->setModel(&controls->m_feedbackModel);
	feedbackKnob->setLabel(tr("FEED"));
	feedbackKnob->setHintText(tr("Feedback:") , "");
	
	PixmapButton * dcButton = new PixmapButton(this, tr("DC Offset Removal"));
	dcButton->move(176, 16);
	dcButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("dc_active"));
	dcButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("dc_inactive"));
	dcButton->setCheckable(true);
	dcButton->setModel(&controls->m_dcModel);
	dcButton->setToolTip(tr("Remove DC Offset"));
}


} // namespace lmms::gui
