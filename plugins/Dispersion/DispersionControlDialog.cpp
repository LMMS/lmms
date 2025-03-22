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
#include <QBoxLayout>
#include <qnamespace.h>
#include "AutomatableButton.h"
#include "DispersionControls.h"

#include "embed.h"
#include "Knob.h"
#include "LcdSpinBox.h"


namespace lmms::gui
{


DispersionControlDialog::DispersionControlDialog(DispersionControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);

	auto layout = new QVBoxLayout(this);
	auto topRow = new QHBoxLayout();
	auto bottomRow = new QHBoxLayout();
	layout->addLayout(topRow);
	bottomRow->setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	layout->addLayout(bottomRow);
	bottomRow->setAlignment(Qt::AlignJustify | Qt::AlignVCenter);

	auto amtBox = new LcdSpinBox(3, this, "Amount");
	amtBox->setModel(&controls->m_amountModel);
	amtBox->setLabel(tr("AMOUNT"));
	amtBox->setToolTip(tr("Number of all-pass filters"));
	bottomRow->addWidget(amtBox);
	bottomRow->addStretch(2);
	
	auto freqKnob = new Knob(KnobType::Bright26, this);
	freqKnob->setModel(&controls->m_freqModel);
	freqKnob->setLabel(tr("FREQ"));
	freqKnob->setHintText(tr("Frequency:") , "Hz");
	topRow->addWidget(freqKnob);
	
	auto resoKnob = new Knob(KnobType::Bright26, this);
	resoKnob->setModel(&controls->m_resoModel);
	resoKnob->setLabel(tr("RESO"));
	resoKnob->setHintText(tr("Resonance:") , "octaves");
	topRow->addWidget(resoKnob);
	
	auto feedbackKnob = new Knob(KnobType::Bright26, this);
	feedbackKnob->setModel(&controls->m_feedbackModel);
	feedbackKnob->setLabel(tr("FEED"));
	feedbackKnob->setHintText(tr("Feedback:") , "");
	topRow->addWidget(feedbackKnob);
	
	auto dcButton = new AutomatableButton(this, tr("DC Offset Removal"));
	dcButton->setText(tr("DC"));
	dcButton->setCheckable(true);
	dcButton->setModel(&controls->m_dcModel);
	dcButton->setToolTip(tr("Remove DC Offset"));
	dcButton->setObjectName("btn");
	bottomRow->addWidget(dcButton);
}


} // namespace lmms::gui
