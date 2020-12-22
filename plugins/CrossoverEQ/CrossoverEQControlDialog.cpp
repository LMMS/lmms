/*
 * CrossoverEQControlDialog.cpp - A native 4-band Crossover Equalizer 
 * good for simulating tonestacks or simple peakless (flat-band) equalization
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "CrossoverEQControlDialog.h"
#include "CrossoverEQControls.h"
#include "embed.h"
#include "ToolTip.h"
#include "LedCheckbox.h"
#include "Knob.h"
#include "Fader.h"

CrossoverEQControlDialog::CrossoverEQControlDialog( CrossoverEQControls * controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 167, 178 );
	
	// knobs
	Knob * xover12 = new Knob( knobBright_26, this );
	xover12->move( 29, 11 );
	xover12->setModel( & controls->m_xover12 );
	xover12->setLabel( "1/2" );
	xover12->setHintText( tr( "Band 1/2 crossover:" ), " Hz" );
	
	Knob * xover23 = new Knob( knobBright_26, this );
	xover23->move( 69, 11 );
	xover23->setModel( & controls->m_xover23 );
	xover23->setLabel( "2/3" );
	xover23->setHintText( tr( "Band 2/3 crossover:" ), " Hz" );
	
	Knob * xover34 = new Knob( knobBright_26, this );
	xover34->move( 109, 11 );
	xover34->setModel( & controls->m_xover34 );
	xover34->setLabel( "3/4" );
	xover34->setHintText( tr( "Band 3/4 crossover:" ), " Hz" );
	
	m_fader_bg = QPixmap( PLUGIN_NAME::getIconPixmap( "fader_bg" ) );
	m_fader_empty = QPixmap( PLUGIN_NAME::getIconPixmap( "fader_empty" ) );
	m_fader_knob = QPixmap( PLUGIN_NAME::getIconPixmap( "fader_knob2" ) );
	
	// faders
	Fader * gain1 = new Fader( &controls->m_gain1, tr( "Band 1 gain" ), this,
		&m_fader_bg, &m_fader_empty, &m_fader_knob );
	gain1->move( 7, 56 );
	gain1->setDisplayConversion( false );
	gain1->setHintText( tr( "Band 1 gain:" ), " dBFS" );
	
	Fader * gain2 = new Fader( &controls->m_gain2, tr( "Band 2 gain" ), this,
		&m_fader_bg, &m_fader_empty, &m_fader_knob );
	gain2->move( 47, 56 );
	gain2->setDisplayConversion( false );
	gain2->setHintText( tr( "Band 2 gain:" ), " dBFS" );
	
	Fader * gain3 = new Fader( &controls->m_gain3, tr( "Band 3 gain" ), this,
		&m_fader_bg, &m_fader_empty, &m_fader_knob );
	gain3->move( 87, 56 );
	gain3->setDisplayConversion( false );
	gain3->setHintText( tr( "Band 3 gain:" ), " dBFS" );
	
	Fader * gain4 = new Fader( &controls->m_gain4, tr( "Band 4 gain" ), this,
		&m_fader_bg, &m_fader_empty, &m_fader_knob );
	gain4->move( 127, 56 );
	gain4->setDisplayConversion( false );
	gain4->setHintText( tr( "Band 4 gain:" ), " dBFS" );
	
	// leds
	LedCheckBox * mute1 = new LedCheckBox( "", this, tr( "Band 1 mute" ), LedCheckBox::Green );
	mute1->move( 15, 154 );
	mute1->setModel( & controls->m_mute1 );
	ToolTip::add( mute1, tr( "Mute band 1" ) );
	
	LedCheckBox * mute2 = new LedCheckBox( "", this, tr( "Band 2 mute" ), LedCheckBox::Green );
	mute2->move( 55, 154 );
	mute2->setModel( & controls->m_mute2 );
	ToolTip::add( mute2, tr( "Mute band 2" ) );
	
	LedCheckBox * mute3 = new LedCheckBox( "", this, tr( "Band 3 mute" ), LedCheckBox::Green );
	mute3->move( 95, 154 );
	mute3->setModel( & controls->m_mute3 );
	ToolTip::add( mute3, tr( "Mute band 3" ) );
	
	LedCheckBox * mute4 = new LedCheckBox( "", this, tr( "Band 4 mute" ), LedCheckBox::Green );
	mute4->move( 135, 154 );
	mute4->setModel( & controls->m_mute4 );
	ToolTip::add( mute4, tr( "Mute band 4" ) );
}
