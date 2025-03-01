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
 

#include "CrossoverEQControlDialog.h"
#include "CrossoverEQControls.h"
#include "embed.h"
#include "LedCheckBox.h"
#include "Knob.h"
#include "Fader.h"

#include <QPixmap>


namespace lmms::gui
{


CrossoverEQControlDialog::CrossoverEQControlDialog( CrossoverEQControls * controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 167, 178 );
	
	// knobs
	auto xover12 = new Knob(KnobType::Bright26, this);
	xover12->move( 29, 11 );
	xover12->setModel( & controls->m_xover12 );
	xover12->setLabel( "1/2" );
	xover12->setHintText( tr( "Band 1/2 crossover:" ), " Hz" );

	auto xover23 = new Knob(KnobType::Bright26, this);
	xover23->move( 69, 11 );
	xover23->setModel( & controls->m_xover23 );
	xover23->setLabel( "2/3" );
	xover23->setHintText( tr( "Band 2/3 crossover:" ), " Hz" );

	auto xover34 = new Knob(KnobType::Bright26, this);
	xover34->move( 109, 11 );
	xover34->setModel( & controls->m_xover34 );
	xover34->setLabel( "3/4" );
	xover34->setHintText( tr( "Band 3/4 crossover:" ), " Hz" );
	
	QPixmap const fader_knob(PLUGIN_NAME::getIconPixmap("fader_knob2"));
	
	// faders
	auto gain1 = new Fader(&controls->m_gain1, tr("Band 1 gain"), this, fader_knob, false);
	gain1->move( 7, 56 );
	gain1->setDisplayConversion( false );
	gain1->setHintText( tr( "Band 1 gain:" ), " dBFS" );
	gain1->setRenderUnityLine(false);

	auto gain2 = new Fader(&controls->m_gain2, tr("Band 2 gain"), this, fader_knob, false);
	gain2->move( 47, 56 );
	gain2->setDisplayConversion( false );
	gain2->setHintText( tr( "Band 2 gain:" ), " dBFS" );
	gain2->setRenderUnityLine(false);

	auto gain3 = new Fader(&controls->m_gain3, tr("Band 3 gain"), this, fader_knob, false);
	gain3->move( 87, 56 );
	gain3->setDisplayConversion( false );
	gain3->setHintText( tr( "Band 3 gain:" ), " dBFS" );
	gain3->setRenderUnityLine(false);

	auto gain4 = new Fader(&controls->m_gain4, tr("Band 4 gain"), this, fader_knob, false);
	gain4->move( 127, 56 );
	gain4->setDisplayConversion( false );
	gain4->setHintText( tr( "Band 4 gain:" ), " dBFS" );
	gain4->setRenderUnityLine(false);
	
	// leds
	auto mute1 = new LedCheckBox("", this, tr("Band 1 mute"), LedCheckBox::LedColor::Green);
	mute1->move( 15, 154 );
	mute1->setModel( & controls->m_mute1 );
	mute1->setToolTip(tr("Mute band 1"));

	auto mute2 = new LedCheckBox("", this, tr("Band 2 mute"), LedCheckBox::LedColor::Green);
	mute2->move( 55, 154 );
	mute2->setModel( & controls->m_mute2 );
	mute2->setToolTip(tr("Mute band 2"));

	auto mute3 = new LedCheckBox("", this, tr("Band 3 mute"), LedCheckBox::LedColor::Green);
	mute3->move( 95, 154 );
	mute3->setModel( & controls->m_mute3 );
	mute3->setToolTip(tr("Mute band 3"));

	auto mute4 = new LedCheckBox("", this, tr("Band 4 mute"), LedCheckBox::LedColor::Green);
	mute4->move( 135, 154 );
	mute4->setModel( & controls->m_mute4 );
	mute4->setToolTip(tr("Mute band 4"));
}


} // namespace lmms::gui