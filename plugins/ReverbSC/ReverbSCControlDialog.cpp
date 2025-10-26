/*
 * ReverbSCControlDialog.cpp - control dialog for ReverbSC
 *
 * Copyright (c) 2017 Paul Batchelor
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


#include "ReverbSCControlDialog.h"

#include "embed.h"
#include "Knob.h"
#include "ReverbSCControls.h"

#include <QHBoxLayout>

namespace lmms::gui
{


ReverbSCControlDialog::ReverbSCControlDialog( ReverbSCControls* controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	auto knobLayout = new QHBoxLayout(this);

	auto inputGainKnob = new Knob(KnobType::Bright26, tr("Input"), this);
	inputGainKnob->setModel( &controls->m_inputGainModel );
	inputGainKnob->setHintText( tr( "Input gain:" ) , "dB" );

	auto sizeKnob = new Knob(KnobType::Bright26, tr("Size"), this);
	sizeKnob->setModel( &controls->m_sizeModel );
	sizeKnob->setHintText( tr( "Size:" ) , "" );

	auto colorKnob = new Knob(KnobType::Bright26, tr("Color"), this);
	colorKnob->setModel( &controls->m_colorModel );
	colorKnob->setHintText( tr( "Color:" ) , "" );

	auto outputGainKnob = new Knob(KnobType::Bright26, tr("Output"), this);
	outputGainKnob->setModel( &controls->m_outputGainModel );
	outputGainKnob->setHintText( tr( "Output gain:" ) , "dB" );

	knobLayout->addWidget(inputGainKnob);
	knobLayout->addWidget(sizeKnob);
	knobLayout->addWidget(colorKnob);
	knobLayout->addWidget(outputGainKnob);
}


} // namespace lmms::gui