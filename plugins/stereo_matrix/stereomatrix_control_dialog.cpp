/*
 * stereomatrix_control_dialog.cpp - control dialog for stereoMatrix-effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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



stereoMatrixControlDialog::stereoMatrixControlDialog(
	stereoMatrixControls * _controls ) :
	effectControlDialog( _controls )
{

	setFixedSize( 92, 92 );

	QGridLayout * l = new QGridLayout( this );

	knob * llKnob = new knob( knobBright_26, this, tr( "Left to Left" ) );
	llKnob->setModel( &_controls->m_llModel );
	llKnob->setHintText( tr( "Volume:" ) + " ", "" );

	knob * lrKnob = new knob( knobBright_26, this, tr( "Left to Right" ) );
	lrKnob->setModel( &_controls->m_lrModel );
	lrKnob->setHintText( tr( "Volume:" ) + " ", "" );

	knob * rlKnob = new knob( knobBright_26, this, tr( "Right to Left" ) );
	rlKnob->setModel( &_controls->m_rlModel );
	rlKnob->setHintText( tr( "Volume:" ) + " ", "" );

	knob * rrKnob = new knob( knobBright_26, this, tr( "Right to Right" ) );
	rrKnob->setModel( &_controls->m_rrModel );
	rrKnob->setHintText( tr( "Volume:" ) + " ", "" );

	l->addWidget( new QLabel( "L", this ), 1, 0);
	l->addWidget( new QLabel( "R", this ), 2, 0);
	l->addWidget( new QLabel( "L", this ), 0, 1);
	l->addWidget( new QLabel( "R", this ), 0, 2);

	l->addWidget( llKnob, 1, 1 );
	l->addWidget( lrKnob, 1, 2 );
	l->addWidget( rlKnob, 2, 1 );
	l->addWidget( rrKnob, 2, 2 );

	this->setLayout(l);
}

