/*
 * stereomatrix_control_dialog.cpp - control dialog for stereoMatrix-effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * 
 * This file is part of LMMS - http://lmms.io
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



#include <QtGui/QLayout>
#include <QtGui/QLabel>

#include "stereomatrix_control_dialog.h"
#include "stereomatrix_controls.h"
#include "embed.h"



stereoMatrixControlDialog::stereoMatrixControlDialog(
	stereoMatrixControls * _controls ) :
	EffectControlDialog( _controls )
{

	setFixedSize( 105, 115);

	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );


	knob * llKnob = new knob( knobSmall_17, this );
	llKnob->setModel( &_controls->m_llModel );
	llKnob->setHintText( tr( "Left to Left Vol:" ) + " ", "" );
	llKnob->move( 40, 60 );

	knob * lrKnob = new knob( knobSmall_17, this );
	lrKnob->setModel( &_controls->m_lrModel );
	lrKnob->setHintText( tr( "Left to Right Vol:" ) + " ", "" );
	lrKnob->move( 40+28, 60);

	knob * rlKnob = new knob( knobSmall_17, this );
	rlKnob->setModel( &_controls->m_rlModel );
	rlKnob->setHintText( tr( "Right to Left Vol:" ) + " ", "" );
	rlKnob->move( 40, 60+28 );

	knob * rrKnob = new knob( knobSmall_17, this );
	rrKnob->setModel( &_controls->m_rrModel );
	rrKnob->setHintText( tr( "Right to Right Vol:" ) + " ", "" );
	rrKnob->move( 40+28, 60+28 );
}

#include "moc_stereomatrix_control_dialog.cxx"
