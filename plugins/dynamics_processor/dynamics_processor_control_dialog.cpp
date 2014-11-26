/*
 * dynamics_processor_control_dialog.cpp - control-dialog for dynamics_processor-effect
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


#include <QLayout>

#include "dynamics_processor_control_dialog.h"
#include "dynamics_processor_controls.h"
#include "embed.h"
#include "Graph.h"
#include "PixmapButton.h"
#include "ToolTip.h"
#include "LedCheckbox.h"


dynProcControlDialog::dynProcControlDialog(
					dynProcControls * _controls ) :
	EffectControlDialog( _controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 224, 340 );

	Graph * waveGraph = new Graph( this, Graph::LinearNonCyclicStyle, 204, 205 );
	waveGraph -> move( 10, 32 );
	waveGraph -> setModel( &_controls -> m_wavegraphModel );
	waveGraph -> setAutoFillBackground( true );
	pal = QPalette();
	pal.setBrush( backgroundRole(),
			PLUGIN_NAME::getIconPixmap("wavegraph") );
	waveGraph->setPalette( pal );
	waveGraph->setGraphColor( QColor( 170, 255, 255 ) );
	waveGraph -> setMaximumSize( 204, 205 );

	Knob * inputKnob = new Knob( knobBright_26, this);
	inputKnob -> setVolumeKnob( true );
	inputKnob -> setVolumeRatio( 1.0 );
	inputKnob -> move( 14, 251 );
	inputKnob->setModel( &_controls->m_inputModel );
	inputKnob->setLabel( tr( "INPUT" ) );
	inputKnob->setHintText( tr( "Input gain:" ) + " ", "" );

	Knob * outputKnob = new Knob( knobBright_26, this );
	outputKnob -> setVolumeKnob( true );
	outputKnob -> setVolumeRatio( 1.0 );
	outputKnob -> move( 54, 251 );
	outputKnob->setModel( &_controls->m_outputModel );
	outputKnob->setLabel( tr( "OUTPUT" ) );
	outputKnob->setHintText( tr( "Output gain:" ) + " ", "" );
	
	Knob * attackKnob = new Knob( knobBright_26, this);
	attackKnob -> move( 11, 291 );
	attackKnob->setModel( &_controls->m_attackModel );
	attackKnob->setLabel( tr( "ATTACK" ) );
	attackKnob->setHintText( tr( "Peak attack time:" ) + " ", "ms" );

	Knob * releaseKnob = new Knob( knobBright_26, this );
	releaseKnob -> move( 52, 291 );
	releaseKnob->setModel( &_controls->m_releaseModel );
	releaseKnob->setLabel( tr( "RELEASE" ) );
	releaseKnob->setHintText( tr( "Peak release time:" ) + " ", "ms" );

//waveform control buttons

	PixmapButton * resetButton = new PixmapButton( this, tr("Reset waveform") );
	resetButton -> move( 164, 251 );
	resetButton -> resize( 12, 48 );
	resetButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "reset_active" ) );
	resetButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "reset_inactive" ) );
	ToolTip::add( resetButton, tr( "Click here to reset the wavegraph back to default" ) );

	PixmapButton * smoothButton = new PixmapButton( this, tr("Smooth waveform") );
	smoothButton -> move( 164, 267 );
	smoothButton -> resize( 12, 48 );
	smoothButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "smooth_active" ) );
	smoothButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "smooth_inactive" ) );
	ToolTip::add( smoothButton, tr( "Click here to apply smoothing to wavegraph" ) );

	PixmapButton * addOneButton = new PixmapButton( this, tr("Increase wavegraph amplitude by 1dB") );
	addOneButton -> move( 133, 251 );
	addOneButton -> resize( 12, 29 );
	addOneButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "add1_active" ) );
	addOneButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "add1_inactive" ) );
	ToolTip::add( addOneButton, tr( "Click here to increase wavegraph amplitude by 1dB" ) );

	PixmapButton * subOneButton = new PixmapButton( this, tr("Decrease wavegraph amplitude by 1dB") );
	subOneButton -> move( 133, 267 );
	subOneButton -> resize( 12, 29 );
	subOneButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "sub1_active" ) );
	subOneButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "sub1_inactive" ) );
	ToolTip::add( subOneButton, tr( "Click here to decrease wavegraph amplitude by 1dB" ) );

//stereomode switches
	PixmapButton * smMaxButton = new PixmapButton( this, tr( "Stereomode Maximum" ) );
	smMaxButton -> move( 165, 290 );
	smMaxButton -> resize( 48, 13 );
	smMaxButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "max_active" ) );
	smMaxButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "max_inactive" ) );
	ToolTip::add( smMaxButton, tr( "Process based on the maximum of both stereo channels" ) );
	
	PixmapButton * smAvgButton = new PixmapButton( this, tr( "Stereomode Average" ) );
	smAvgButton -> move( 165, 290 + 13 );
	smAvgButton -> resize( 48, 13 );
	smAvgButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "avg_active" ) );
	smAvgButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "avg_inactive" ) );
	ToolTip::add( smAvgButton, tr( "Process based on the average of both stereo channels" ) );

	PixmapButton * smUnlButton = new PixmapButton( this, tr( "Stereomode Unlinked" ) );
	smUnlButton -> move( 165, 290 + (13*2) );
	smUnlButton -> resize( 48, 13 );
	smUnlButton -> setActiveGraphic( PLUGIN_NAME::getIconPixmap( "unl_active" ) );
	smUnlButton -> setInactiveGraphic( PLUGIN_NAME::getIconPixmap( "unl_inactive" ) );
	ToolTip::add( smUnlButton, tr( "Process each stereo channel independently" ) );
	
	automatableButtonGroup * smGroup = new automatableButtonGroup( this );
	smGroup -> addButton( smMaxButton );
	smGroup -> addButton( smAvgButton );
	smGroup -> addButton( smUnlButton );
	smGroup -> setModel( &_controls -> m_stereomodeModel );

	connect( resetButton, SIGNAL (clicked () ),
			_controls, SLOT ( resetClicked() ) );
	connect( smoothButton, SIGNAL (clicked () ),
			_controls, SLOT ( smoothClicked() ) );
	connect( addOneButton, SIGNAL( clicked() ),
			_controls, SLOT( addOneClicked() ) );
	connect( subOneButton, SIGNAL( clicked() ),
			_controls, SLOT( subOneClicked() ) );
}
