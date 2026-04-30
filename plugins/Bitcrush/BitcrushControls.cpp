/*
 * BitcrushControls.cpp - A native bitcrusher
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



#include "BitcrushControls.h"
#include "Bitcrush.h"

namespace lmms
{



BitcrushControls::BitcrushControls( BitcrushEffect * eff ) :
	EffectControls( eff ),
	m_effect( eff ),
	m_inGain( 0.0f, -20.0f, 20.0f, 0.1f, this, tr( "Input gain" ) ),
	m_inNoise( 0.0f, 0.0f, 100.0f, 0.1f, this, tr( "Input noise" ) ),
	m_outGain( 0.0f, -20.0f, 20.0f, 0.1f, this, tr( "Output gain" ) ),
	m_outClip( 0.0f, -20.0f, 20.0f, 0.1f, this, tr( "Output clip" ) ),
	m_rate( 44100.f, 20.f, 44100.f, 1.0f, this, tr( "Sample rate" ) ),
	m_stereoDiff( 0.f, -50.f, 50.f, 0.1f, this, tr( "Stereo difference" ) ),
	m_levels( 256.f, 1.f, 256.f, 0.01f, this, tr( "Levels" ) ),
	m_rateEnabled( true, this, tr( "Rate enabled" ) ),
	m_depthEnabled( true, this, tr( "Depth enabled" ) )
{
	m_rate.setStrictStepSize( true );
	m_levels.setStrictStepSize( true );
	
	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( sampleRateChanged() ) );
}


void BitcrushControls::saveSettings( QDomDocument & doc, QDomElement & elem )
{
	m_inGain.saveSettings( doc, elem, "ingain" );
	m_inNoise.saveSettings( doc, elem, "innoise" );
	m_outGain.saveSettings( doc, elem, "outgain" );
	m_outClip.saveSettings( doc, elem, "outclip" );
	m_rate.saveSettings( doc, elem, "rate" );
	m_stereoDiff.saveSettings( doc, elem, "stereodiff" );
	m_levels.saveSettings( doc, elem, "levels" );
	m_rateEnabled.saveSettings( doc, elem, "rateon" );
	m_depthEnabled.saveSettings( doc, elem, "depthon" );
}


void BitcrushControls::loadSettings( const QDomElement & elem )
{
	m_inGain.loadSettings(  elem, "ingain" );
	m_inNoise.loadSettings(  elem, "innoise" );
	m_outGain.loadSettings(  elem, "outgain" );
	m_outClip.loadSettings(  elem, "outclip" );
	m_rate.loadSettings(  elem, "rate" );
	m_stereoDiff.loadSettings(  elem, "stereodiff" );
	m_levels.loadSettings(  elem, "levels" );
	m_rateEnabled.loadSettings(  elem, "rateon" );
	m_depthEnabled.loadSettings(  elem, "depthon" );
	
	m_effect->m_needsUpdate = true;
}

void BitcrushControls::sampleRateChanged()
{
	m_effect->sampleRateChanged();
}


} // namespace lmms