/*
 * ReverbControlDialog.cpp - control dialog for Reverb
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


#include "ReverbControlDialog.h"

#include "embed.h"
#include "Knob.h"
#include "ReverbControls.h"
#include "../Eq/EqFader.h"

namespace lmms::gui
{


ReverbControlDialog::ReverbControlDialog( ReverbControls* controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 144, 144);

	auto inFader
		= new EqFader(&controls->m_inputGainModel, tr("Input gain"), this, &controls->m_inPeakL, &controls->m_inPeakR);
	inFader->setMaximumHeight( 32 );
	inFader->move( 15, 16 );
	inFader->setDisplayConversion( false );
	inFader->setHintText( tr( "Gain" ), "dBFS" );

	auto sizeKnob = new Knob(knobBright_26, this);
	sizeKnob -> move( 58, 30 );
	sizeKnob->setModel( &controls->m_sizeModel );
	sizeKnob->setLabel( tr( "Size" ) );
	sizeKnob->setHintText( tr( "Size:" ) , "" );

	auto colorKnob = new Knob(knobBright_26, this);
	colorKnob -> move( 58, 76 );
	colorKnob->setModel( &controls->m_colorModel );
	colorKnob->setLabel( tr( "Color" ) );
	colorKnob->setHintText( tr( "Color:" ) , "" );

	auto outFader
		= new EqFader(&controls->m_outputGainModel, tr("Out gain"), this, &controls->m_outPeakL, &controls->m_outPeakR);
	outFader->setMaximumHeight( 196 );
	outFader->move( 106, 16 );
	outFader->setDisplayConversion( false );
	outFader->setHintText( tr( "Gain" ), "dBFS" );
	
}


} // namespace lmms::gui