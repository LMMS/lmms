/*
 * stereomatrix_control_dialog.cpp - control dialog for stereoMatrix-effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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



#include <QLayout>
#include <QLabel>

#include "stereomatrix_control_dialog.h"
#include "stereomatrix_controls.h"
#include "embed.h"



stereoMatrixControlDialog::stereoMatrixControlDialog(
	stereoMatrixControls * _controls ) :
	EffectControlDialog( _controls )
{

	setFixedSize( 160, 185 );

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );


	Knob * llKnob = new Knob( knobBright_26, this );
	llKnob->setModel( &_controls->m_llModel );
	llKnob->setHintText( tr( "Left to Left Vol:" ) , "" );
	llKnob->move( 10, 79 );

	Knob * lrKnob = new Knob( knobBright_26, this );
	lrKnob->setModel( &_controls->m_lrModel );
	lrKnob->setHintText( tr( "Left to Right Vol:" ) , "" );
	lrKnob->move( 48, 79 );

	Knob * rlKnob = new Knob( knobBright_26, this );
	rlKnob->setModel( &_controls->m_rlModel );
	rlKnob->setHintText( tr( "Right to Left Vol:" ) , "" );
	rlKnob->move( 85, 79 );

	Knob * rrKnob = new Knob( knobBright_26, this );
	rrKnob->setModel( &_controls->m_rrModel );
	rrKnob->setHintText( tr( "Right to Right Vol:" ) , "" );
	rrKnob->move( 123, 79 );
}
