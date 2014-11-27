/*
 * MultitapEchoControlDialog.cpp - a multitap echo delay plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QLayout>

#include "MultitapEchoControlDialog.h"
#include "MultitapEchoControls.h"
#include "embed.h"
#include "Graph.h"
#include "PixmapButton.h"
#include "ToolTip.h"
#include "LedCheckbox.h"
#include "Knob.h"
#include "TempoSyncKnob.h"
#include "LcdSpinBox.h"


MultitapEchoControlDialog::MultitapEchoControlDialog( MultitapEchoControls * controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 245, 300 );
	
	// graph widgets
	
	Graph * ampGraph = new Graph( this, Graph::BarStyle, 204, 105 );
	Graph * lpGraph = new Graph( this, Graph::BarStyle, 204, 105 );
	
	ampGraph->move( 30, 10 );
	lpGraph->move( 30, 125 );
	
	ampGraph->setModel( & controls->m_ampGraph );
	lpGraph->setModel( & controls->m_lpGraph );
	
	pal = QPalette();
	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap("graph_bg") );
	
	ampGraph->setAutoFillBackground( true );
	ampGraph->setPalette( pal );
	ampGraph->setGraphColor( QColor( 48, 255, 117 ) );
	ampGraph -> setMaximumSize( 204, 105 );
	
	lpGraph->setAutoFillBackground( true );
	lpGraph->setPalette( pal );
	lpGraph->setGraphColor( QColor( 48, 255, 117 ) );
	lpGraph -> setMaximumSize( 204, 105 );
	
	// steps spinbox
	
	LcdSpinBox * steps = new LcdSpinBox( 2, this, "Steps" );
	steps->move( 20, 240 );
	steps->setModel( & controls->m_steps );
	
	// knobs

	TempoSyncKnob * stepLength = new TempoSyncKnob( knobBright_26, this );
	stepLength->move( 80, 240 );
	stepLength->setModel( & controls->m_stepLength );
	stepLength->setLabel( tr( "Length" ) );
	stepLength->setHintText( tr( "Step length:" ) + " ", "ms" );
	
	Knob * dryGain = new Knob( knobBright_26, this );
	dryGain->move( 130, 240 );
	dryGain->setModel( & controls->m_dryGain );
	dryGain->setLabel( tr( "Dry" ) );
	dryGain->setHintText( tr( "Dry Gain:" ) + " ", "dBV" );
	
	// switch led
	
	LedCheckBox * swapInputs = new LedCheckBox( "Swap inputs", this, tr( "Swap in" ), LedCheckBox::Green );
	swapInputs->move( 180, 240 );
	swapInputs->setModel( & controls-> m_swapInputs );
	ToolTip::add( swapInputs, tr( "Swap left and right channel for reflections" ) );
}
