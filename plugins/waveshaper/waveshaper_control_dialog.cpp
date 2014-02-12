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
#include "pixmap_button.h"
#include "tooltip.h"


waveShaperControlDialog::waveShaperControlDialog(
					waveShaperControls * _controls ) :
	EffectControlDialog( _controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 224, 300 );

	graph * waveGraph = new graph( this, graph::LinearNonCyclicStyle, 204, 204 );
	waveGraph -> move( 10, 32 );
	waveGraph -> setModel( &_controls -> m_wavegraphModel );
	waveGraph -> setAutoFillBackground( true );
	pal = QPalette();
	pal.setBrush( backgroundRole(), 
			PLUGIN_NAME::getIconPixmap("wavegraph") );
	waveGraph->setPalette( pal );
	waveGraph->setGraphColor( QColor( 170, 255, 255 ) );
	waveGraph -> setMaximumSize( 204, 204 );
	
	knob * inputKnob = new knob( knobBright_26, this);
	inputKnob -> move( 14, 251 );
	inputKnob->setModel( &_controls->m_inputModel );
	inputKnob->setLabel( tr( "INPUT" ) );
	inputKnob->setHintText( tr( "Input gain:" ) + " ", "" );
	
	knob * outputKnob = new knob( knobBright_26, this );
	outputKnob -> move( 54, 251 );
	outputKnob->setModel( &_controls->m_outputModel );
	outputKnob->setLabel( tr( "OUTPUT" ) );
	outputKnob->setHintText( tr( "Output gain:" ) + " ", "" );

	pixmapButton * resetButton = new pixmapButton( this, tr("Reset waveform") );
	resetButton -> move( 164, 251 );
	resetButton -> resize( 12, 48 );
	resetButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "reset_active" ) );
	resetButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "reset_inactive" ) );
	toolTip::add( resetButton, tr( "Click here to reset the wavegraph back to default" ) );
	
	pixmapButton * smoothButton = new pixmapButton( this, tr("Smooth waveform") );
	smoothButton -> move( 164, 267 );
	smoothButton -> resize( 12, 48 );
	smoothButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "smooth_active" ) );
	smoothButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "smooth_inactive" ) );
	toolTip::add( smoothButton, tr( "Click here to apply smooth to wavegraph" ) );
	
	connect( resetButton, SIGNAL (clicked () ),
			_controls, SLOT ( resetClicked() ) );
	connect( smoothButton, SIGNAL (clicked () ),
			_controls, SLOT ( smoothClicked() ) );
}
