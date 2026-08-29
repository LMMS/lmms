/*
 * CrossoverEQControls.cpp - A native 4-band Crossover Equalizer 
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
 
#include "CrossoverEQControls.h"
#include "CrossoverEQ.h"

namespace lmms
{


CrossoverEQControls::CrossoverEQControls( CrossoverEQEffect * eff ) :
	EffectControls( eff ),
	m_effect( eff ),
	m_xover12( 125.f, 50.f, 10000.f, 1.0f, this, "Band 1/2 Crossover" ),
	m_xover23( 1250.f, 50.f, 20000.f, 1.0f, this, "Band 2/3 Crossover" ),
	m_xover34( 5000.f, 50.f, 20000.f, 1.0f, this, "Band 3/4 Crossover" ),
	m_gain1( 0.f, -60.f, 30.f, 0.1f, this, "Band 1 Gain" ),
	m_gain2( 0.f, -60.f, 30.f, 0.1f, this, "Band 2 Gain" ),
	m_gain3( 0.f, -60.f, 30.f, 0.1f, this, "Band 3 Gain" ),
	m_gain4( 0.f, -60.f, 30.f, 0.1f, this, "Band 4 Gain" ),
	m_mute1( true, this, "Mute Band 1" ),
	m_mute2( true, this, "Mute Band 2" ),
	m_mute3( true, this, "Mute Band 3" ),
	m_mute4( true, this, "Mute Band 4" )
{
	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( sampleRateChanged() ) );
	connect( &m_xover12, SIGNAL( dataChanged() ), this, SLOT( xover12Changed() ) );
	connect( &m_xover23, SIGNAL( dataChanged() ), this, SLOT( xover23Changed() ) );
	connect( &m_xover34, SIGNAL( dataChanged() ), this, SLOT( xover34Changed() ) );
	
	m_xover12.setScaleLogarithmic( true );
	m_xover23.setScaleLogarithmic( true );
	m_xover34.setScaleLogarithmic( true );
}

void CrossoverEQControls::saveSettings( QDomDocument & doc, QDomElement & elem )
{
	m_xover12.saveSettings( doc, elem, "xover12" );
	m_xover23.saveSettings( doc, elem, "xover23" );
	m_xover34.saveSettings( doc, elem, "xover34" );
	
	m_gain1.saveSettings( doc, elem, "gain1" );
	m_gain2.saveSettings( doc, elem, "gain2" );
	m_gain3.saveSettings( doc, elem, "gain3" );
	m_gain4.saveSettings( doc, elem, "gain4" );
	
	m_mute1.saveSettings( doc, elem, "mute1" );
	m_mute2.saveSettings( doc, elem, "mute2" );
	m_mute3.saveSettings( doc, elem, "mute3" );
	m_mute4.saveSettings( doc, elem, "mute4" );
}

void CrossoverEQControls::loadSettings( const QDomElement & elem )
{
	m_xover12.loadSettings( elem, "xover12" );
	m_xover23.loadSettings( elem, "xover23" );
	m_xover34.loadSettings( elem, "xover34" );
	
	m_gain1.loadSettings( elem, "gain1" );
	m_gain2.loadSettings( elem, "gain2" );
	m_gain3.loadSettings( elem, "gain3" );
	m_gain4.loadSettings( elem, "gain4" );
	
	m_mute1.loadSettings( elem, "mute1" );
	m_mute2.loadSettings( elem, "mute2" );
	m_mute3.loadSettings( elem, "mute3" );
	m_mute4.loadSettings( elem, "mute4" );
	
	m_effect->m_needsUpdate = true;
	m_effect->clearFilterHistories();
}

void CrossoverEQControls::xover12Changed()
{
	float v = m_xover12.value();
	if( m_xover23.value() < v ) { m_xover23.setValue( v ); }
	if( m_xover34.value() < v ) { m_xover34.setValue( v ); }
}

void CrossoverEQControls::xover23Changed()
{
	float v = m_xover23.value();
	if( m_xover12.value() > v ) { m_xover12.setValue( v ); }
	if( m_xover34.value() < v ) { m_xover34.setValue( v ); }
}

void CrossoverEQControls::xover34Changed()
{
	float v = m_xover34.value();
	if( m_xover12.value() > v ) { m_xover12.setValue( v ); }
	if( m_xover23.value() > v ) { m_xover23.setValue( v ); }
}


void CrossoverEQControls::sampleRateChanged()
{
	m_effect->sampleRateChanged();
}


} // namespace lmms
