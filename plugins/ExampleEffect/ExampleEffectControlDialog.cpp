/*
 * ExampleEffectControlDialog.cpp - Example effect gui boilerplate code
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

#include "ExampleEffectControlDialog.h"
#include "ExampleEffectControls.h"
#include "embed.h"
#include "Knob.h"
#include "Fader.h"
#include "LedCheckBox.h"
#include "LcdSpinBox.h"

#include <QHBoxLayout>

namespace lmms::gui
{

ExampleEffectControlDialog::ExampleEffectControlDialog(ExampleEffectControls* controls) :
	EffectControlDialog(controls)
{
	setAutoFillBackground(true);
	/* Uncomment this if you want to use a background image
	QPalette pal;
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);
	setFixedSize(100, 110);
	*/

	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(20,20,20,20);
	//mainLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

	// Example knob
	Knob* decayKnob = new Knob(KnobType::Bright26, this);
	decayKnob->setModel(&controls->m_decayModel);
	decayKnob->setLabel(tr("Decay"));
	decayKnob->setHintText(tr("Decay per frame:"), "");
	mainLayout->addWidget(decayKnob);

	// In this demo, only the decay knob actually does anything.
	// The rest are to showcase the different types of gui elements.

	// Example volume knob
	Knob* volumeKnob = new Knob(KnobType::Bright26, this);
	volumeKnob->setModel(&controls->m_volumeModel);
	volumeKnob->setLabel(tr("Volume"));
	volumeKnob->setHintText(tr("Volume:"), "%");
	volumeKnob->setVolumeKnob(true);
	mainLayout->addWidget(volumeKnob);

	// Example check box
	LedCheckBox* invertDecay = new LedCheckBox(tr("Toggle"), this);
	invertDecay->setModel(&controls->m_invertModel);
	invertDecay->setToolTip(tr("Explanation of check box"));
	invertDecay->setCheckable(true);
	mainLayout->addWidget(invertDecay);

	// Example Number LCD
	LcdSpinBox * amountBox = new LcdSpinBox(3, this);
	amountBox->setModel(&controls->m_numberModel);
	amountBox->setLabel(tr("Number"));
	amountBox->setToolTip(tr("Number"));
	mainLayout->addWidget(amountBox);

	// Example Fader
	Fader* exampleFader = new Fader(&controls->m_faderModel, "Example Fader", this, false);
	exampleFader->setHintText("Example Fader", "dBFS");
	exampleFader->setDisplayConversion(false);
	exampleFader->setRenderUnityLine(false);
	exampleFader->setFixedSize(20, 100);
	mainLayout->addWidget(exampleFader);
}

} // namespace lmms::gui
