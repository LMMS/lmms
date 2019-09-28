/*
 * waveshaper_control_dialog.cpp - control-dialog for waveshaper-effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "waveshaper2_control_dialog.h"
#include "waveshaper2_controls.h"
#include "embed.h"
//#include "Graph.h"
#include "VectorGraph.h"
#include "PixmapButton.h"
#include "ToolTip.h"
#include "LedCheckbox.h"


waveShaper2ControlDialog::waveShaper2ControlDialog(
					waveShaper2Controls * _controls ) :
	EffectControlDialog( _controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 224, 274 );

	VectorGraph * graph = new VectorGraph( this, 204, 205 );
	graph->move( 10, 6 );
	graph->setMaximumSize( 204, 205 );
	graph -> setModel( &_controls -> m_vectorGraphModel );



	Knob * inputKnob = new Knob( knobBright_26, this);
	inputKnob -> setVolumeKnob( true );
	inputKnob -> setVolumeRatio( 1.0 );
	inputKnob -> move( 26, 225 );
	inputKnob->setModel( &_controls->m_inputModel );
	inputKnob->setLabel( tr( "INPUT" ) );
	inputKnob->setHintText( tr( "Input gain:" ) , "" );

	Knob * outputKnob = new Knob( knobBright_26, this );
	outputKnob -> setVolumeKnob( true );
	outputKnob -> setVolumeRatio( 1.0 );
	outputKnob -> move( 76, 225 );
	outputKnob->setModel( &_controls->m_outputModel );
	outputKnob->setLabel( tr( "OUTPUT" ) );
	outputKnob->setHintText( tr( "Output gain:" ), "" );

	LedCheckBox * clipInputToggle = new LedCheckBox( "Clip input", this,
							tr( "Clip input" ), LedCheckBox::Green );
	clipInputToggle -> move( 131, 252 );
	clipInputToggle -> setModel( &_controls -> m_clipModel );
	ToolTip::add( clipInputToggle, tr( "Clip input signal to 0dB" ) );
}

