/*
 * waveshaper_control_dialog.cpp - control-dialog for waveshaper-effect
 *
 * Copyright  * (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright  * (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "waveshaper_control_dialog.h"
#include "waveshaper_controls.h"
#include "embed.h"
#include "graph.h"



waveShaperControlDialog::waveShaperControlDialog(
					waveShaperControls * _controls ) :
	EffectControlDialog( _controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 120, 200 );

	QVBoxLayout * tl = new QVBoxLayout( this );
	tl->addSpacing( 30 );

	graph * waveGraph = new graph( this, graph::LinearStyle );
	waveGraph -> setModel( &_controls -> m_wavegraphModel );
	waveGraph -> setAutoFillBackground( true );
	
	tl -> addWidget( waveGraph );

	QHBoxLayout * l = new QHBoxLayout;

	knob * inputKnob = new knob( knobBright_26, this);
	inputKnob->setModel( &_controls->m_inputModel );
	inputKnob->setLabel( tr( "INPUT" ) );
	inputKnob->setHintText( tr( "Input gain:" ) + " ", "" );

	knob * outputKnob = new knob( knobBright_26, this );
	outputKnob->setModel( &_controls->m_outputModel );
	outputKnob->setLabel( tr( "OUTPUT" ) );
	outputKnob->setHintText( tr( "Output gain:" ) + " ", "" );

	l->addWidget( inputKnob );
	l->addWidget( outputKnob );

	tl->addLayout( l );
	setLayout( tl );
}

