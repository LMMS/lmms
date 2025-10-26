/*
 * MultitapEchoControlDialog.cpp - a multitap echo delay plugin
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "MultitapEchoControlDialog.h"
#include "MultitapEchoControls.h"
#include "embed.h"
#include "FontHelper.h"
#include "Graph.h"
#include "LedCheckBox.h"
#include "Knob.h"
#include "TempoSyncKnob.h"
#include "LcdSpinBox.h"

namespace lmms::gui
{


MultitapEchoControlDialog::MultitapEchoControlDialog( MultitapEchoControls * controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 245, 300 );
	
	// graph widgets

	auto ampGraph = new Graph(this, Graph::Style::Bar, 204, 105);
	auto lpGraph = new Graph(this, Graph::Style::Bar, 204, 105);

	ampGraph->move( 30, 10 );
	lpGraph->move( 30, 125 );
	
	ampGraph->setModel( & controls->m_ampGraph );
	lpGraph->setModel( & controls->m_lpGraph );
	
	pal = QPalette();
	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap("graph_bg") );
	
	ampGraph->setAutoFillBackground( true );
	ampGraph->setPalette( pal );
	ampGraph->setGraphColor( QColor( 11, 213, 86) );
	ampGraph -> setMaximumSize( 204, 105 );
	
	lpGraph->setAutoFillBackground( true );
	lpGraph->setPalette( pal );
	lpGraph->setGraphColor( QColor( 0, 200, 187) );
	lpGraph -> setMaximumSize( 204, 105 );
	
	// steps spinbox

	auto steps = new LcdSpinBox(2, this, "Steps");
	steps->move( 20, 245 );
	steps->setModel( & controls->m_steps );
	
	// knobs

	auto stepLength = new TempoSyncKnob(KnobType::Bright26, tr("Length"), SMALL_FONT_SIZE,  this);
	stepLength->move( 100, 245 );
	stepLength->setModel( & controls->m_stepLength );
	stepLength->setHintText( tr( "Step length:" ) , " ms" );

	auto dryGain = new Knob(KnobType::Bright26, tr("Dry"), SMALL_FONT_SIZE, this);
	dryGain->move( 150, 245 );
	dryGain->setModel( & controls->m_dryGain );
	dryGain->setHintText( tr( "Dry gain:" ) , " dBFS" );

	auto stages = new Knob(KnobType::Bright26, tr("Stages"), SMALL_FONT_SIZE, this);
	stages->move( 200, 245 );
	stages->setModel( & controls->m_stages );
	stages->setHintText( tr( "Low-pass stages:" ) , "x" );
	// switch led

	auto swapInputs = new LedCheckBox("Swap inputs", this, tr("Swap inputs"), LedCheckBox::LedColor::Green);
	swapInputs->move( 20, 275 );
	swapInputs->setModel( & controls->m_swapInputs );
	swapInputs->setToolTip(tr("Swap left and right input channels for reflections"));
}


} // namespace lmms::gui
