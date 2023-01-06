/*
 * AmplifierControlDialog.cpp - control dialog for amplifier effect
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


#include "AmplifierControlDialog.h"
#include "AmplifierControls.h"
#include "embed.h"
#include "Knob.h"


namespace lmms::gui
{


AmplifierControlDialog::AmplifierControlDialog( AmplifierControls* controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 100, 110 );

	auto volumeKnob = new Knob(knobBright_26, this);
	volumeKnob -> move( 16, 10 );
	volumeKnob -> setVolumeKnob( true );
	volumeKnob->setModel( &controls->m_volumeModel );
	volumeKnob->setLabel( tr( "VOL" ) );
	volumeKnob->setHintText( tr( "Volume:" ) , "%" );

	auto panKnob = new Knob(knobBright_26, this);
	panKnob -> move( 57, 10 );
	panKnob->setModel( &controls->m_panModel );
	panKnob->setLabel( tr( "PAN" ) );
	panKnob->setHintText( tr( "Panning:" ) , "" );

	auto leftKnob = new Knob(knobBright_26, this);
	leftKnob -> move( 16, 65 );
	leftKnob -> setVolumeKnob( true );
	leftKnob->setModel( &controls->m_leftModel );
	leftKnob->setLabel( tr( "LEFT" ) );
	leftKnob->setHintText( tr( "Left gain:" ) , "%" );

	auto rightKnob = new Knob(knobBright_26, this);
	rightKnob -> move( 57, 65 );
	rightKnob -> setVolumeKnob( true );
	rightKnob->setModel( &controls->m_rightModel );
	rightKnob->setLabel( tr( "RIGHT" ) );
	rightKnob->setHintText( tr( "Right gain:" ) , "%" );
}


} // namespace lmms::gui