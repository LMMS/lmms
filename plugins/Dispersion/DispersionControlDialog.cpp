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

#include <QHBoxLayout>


namespace lmms::gui
{


DispersionControlDialog::DispersionControlDialog(DispersionControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	auto layout = new QHBoxLayout(this);

	LcdSpinBox * m_amountBox = new LcdSpinBox(3, this, "Amount");
	m_amountBox->setModel(&controls->m_amountModel);
	m_amountBox->setLabel(tr("AMOUNT"));
	m_amountBox->setToolTip(tr("Number of all-pass filters"));
	
	Knob * freqKnob = new Knob(KnobType::Bright26, tr("FREQ"), this);
	freqKnob->setModel(&controls->m_freqModel);
	freqKnob->setHintText(tr("Frequency:") , " Hz");
	
	Knob * resoKnob = new Knob(KnobType::Bright26, tr("RESO"), this);
	resoKnob->setModel(&controls->m_resoModel);
	resoKnob->setHintText(tr("Resonance:") , " octaves");
	
	Knob * feedbackKnob = new Knob(KnobType::Bright26, tr("FEED"), this);
	feedbackKnob->setModel(&controls->m_feedbackModel);
	feedbackKnob->setHintText(tr("Feedback:") , "");
	
	PixmapButton * dcButton = new PixmapButton(this, tr("DC Offset Removal"));
	dcButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap("dc_active"));
	dcButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("dc_inactive"));
	dcButton->setCheckable(true);
	dcButton->setModel(&controls->m_dcModel);
	dcButton->setToolTip(tr("Remove DC Offset"));

	layout->addWidget(m_amountBox);
	layout->addWidget(freqKnob);
	layout->addWidget(resoKnob);
	layout->addWidget(feedbackKnob);
	layout->addWidget(dcButton);
}


} // namespace lmms::gui
