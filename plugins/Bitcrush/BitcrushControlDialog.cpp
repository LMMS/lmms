/*
 * BitcrushControlDialog.cpp - A native bitcrusher
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QLabel>

#include "BitcrushControlDialog.h"
#include "BitcrushControls.h"
#include "embed.h"
#include "ToolTip.h"
#include "LedCheckbox.h"
#include "Knob.h"

BitcrushControlDialog::BitcrushControlDialog( BitcrushControls * controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 215, 120 );
	
	// labels
	QLabel * inLabel = new QLabel( tr( "IN" ), this );
	inLabel->move( 12, 10);
	
	QLabel * outLabel = new QLabel( tr( "OUT" ), this );
	outLabel->move( 176, 10 );
	
	// input knobs
	Knob * inGain = new Knob( knobBright_26, this );
	inGain->move( 12, 25 );
	inGain->setModel( & controls->m_inGain );
	inGain->setLabel( tr( "GAIN" ) );
	inGain->setHintText( tr( "Input Gain:" ) + " ", " dBFS" );
	
	Knob * inNoise = new Knob( knobBright_26, this );
	inNoise->move( 12, 70 );
	inNoise->setModel( & controls->m_inNoise );
	inNoise->setLabel( tr( "NOIS" ) );
	inNoise->setHintText( tr( "Input Noise:" ) + " ", "%" );
	
	
	// output knobs
	Knob * outGain = new Knob( knobBright_26, this );
	outGain->move( 176, 25 );
	outGain->setModel( & controls->m_outGain );
	outGain->setLabel( tr( "GAIN" ) );
	outGain->setHintText( tr( "Output Gain:" ) + " ", " dBFS" );
	
	Knob * outClip = new Knob( knobBright_26, this );
	outClip->move( 176, 70 );
	outClip->setModel( & controls->m_outClip );
	outClip->setLabel( tr( "CLIP" ) );
	outClip->setHintText( tr( "Output Clip:" ) + " ", "%" );
	
	
	// leds
	LedCheckBox * rateEnabled = new LedCheckBox( tr( "Rate" ), this, tr( "Rate Enabled" ), LedCheckBox::Green );
	rateEnabled->move( 50, 30 );
	rateEnabled->setModel( & controls->m_rateEnabled );
	ToolTip::add( rateEnabled, tr( "Enable samplerate-crushing" ) );
	
	LedCheckBox * depthEnabled = new LedCheckBox( tr( "Depth" ), this, tr( "Depth Enabled" ), LedCheckBox::Green );
	depthEnabled->move( 50, 80 );
	depthEnabled->setModel( & controls->m_depthEnabled );
	ToolTip::add( depthEnabled, tr( "Enable bitdepth-crushing" ) );
	
	
	// rate crushing knobs
	Knob * rate = new Knob( knobBright_26, this );
	rate->move( 100, 20 );
	rate->setModel( & controls->m_rate );
	rate->setLabel( tr( "Rate" ) );
	rate->setHintText( tr( "Sample rate:" ) + " ", " Hz" );
	
	Knob * stereoDiff = new Knob( knobBright_26, this );
	stereoDiff->move( 140, 20 );
	stereoDiff->setModel( & controls->m_stereoDiff );
	stereoDiff->setLabel( tr( "STD" ) );
	stereoDiff->setHintText( tr( "Stereo difference:" ) + " ", "%" );
	
	
	// depth crushing knob
	Knob * levels = new Knob( knobBright_26, this );
	levels->move( 140, 70 );
	levels->setModel( & controls->m_levels );
	levels->setLabel( tr( "Levels" ) );
	levels->setHintText( tr( "Levels:" ) + " ", "" );
}
