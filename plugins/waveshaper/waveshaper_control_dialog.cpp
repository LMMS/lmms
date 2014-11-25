/*
 * waveshaper_control_dialog.cpp - control-dialog for waveshaper-effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "waveshaper_control_dialog.h"
#include "waveshaper_controls.h"
#include "embed.h"
#include "graph.h"
#include "pixmap_button.h"
#include "tooltip.h"
#include "led_checkbox.h"


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

	graph * waveGraph = new graph( this, graph::LinearNonCyclicStyle, 204, 205 );
	waveGraph -> move( 10, 32 );
	waveGraph -> setModel( &_controls -> m_wavegraphModel );
	waveGraph -> setAutoFillBackground( true );
	pal = QPalette();
	pal.setBrush( backgroundRole(),
			PLUGIN_NAME::getIconPixmap("wavegraph") );
	waveGraph->setPalette( pal );
	waveGraph->setGraphColor( QColor( 170, 255, 255 ) );
	waveGraph -> setMaximumSize( 204, 205 );

	knob * inputKnob = new knob( knobBright_26, this);
	inputKnob -> setVolumeKnob( true );
	inputKnob -> setVolumeRatio( 1.0 );
	inputKnob -> move( 14, 251 );
	inputKnob->setModel( &_controls->m_inputModel );
	inputKnob->setLabel( tr( "INPUT" ) );
	inputKnob->setHintText( tr( "Input gain:" ) + " ", "" );

	knob * outputKnob = new knob( knobBright_26, this );
	outputKnob -> setVolumeKnob( true );
	outputKnob -> setVolumeRatio( 1.0 );
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
	toolTip::add( smoothButton, tr( "Click here to apply smoothing to wavegraph" ) );

	pixmapButton * addOneButton = new pixmapButton( this, tr("Increase graph amplitude by 1dB") );
	addOneButton -> move( 133, 251 );
	addOneButton -> resize( 12, 29 );
	addOneButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "add1_active" ) );
	addOneButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "add1_inactive" ) );
	toolTip::add( addOneButton, tr( "Click here to increase wavegraph amplitude by 1dB" ) );

	pixmapButton * subOneButton = new pixmapButton( this, tr("Decrease graph amplitude by 1dB") );
	subOneButton -> move( 133, 267 );
	subOneButton -> resize( 12, 29 );
	subOneButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sub1_active" ) );
	subOneButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sub1_inactive" ) );
	toolTip::add( subOneButton, tr( "Click here to decrease wavegraph amplitude by 1dB" ) );

	ledCheckBox * clipInputToggle = new ledCheckBox( "Clip input", this,
							tr( "Clip input" ), ledCheckBox::Green );
	clipInputToggle -> move( 133, 283 );
	clipInputToggle -> setModel( &_controls -> m_clipModel );
	toolTip::add( clipInputToggle, tr( "Clip input signal to 0dB" ) );

	connect( resetButton, SIGNAL (clicked () ),
			_controls, SLOT ( resetClicked() ) );
	connect( smoothButton, SIGNAL (clicked () ),
			_controls, SLOT ( smoothClicked() ) );
	connect( addOneButton, SIGNAL( clicked() ),
			_controls, SLOT( addOneClicked() ) );
	connect( subOneButton, SIGNAL( clicked() ),
			_controls, SLOT( subOneClicked() ) );
}

#include "moc_waveshaper_control_dialog.cxx"
