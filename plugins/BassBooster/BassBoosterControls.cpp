/*
 * BassBoosterControls.cpp - controls for bassbooster effect
 *
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



#include "BassBoosterControls.h"
#include "BassBooster.h"

namespace lmms
{


BassBoosterControls::BassBoosterControls( BassBoosterEffect* effect ) :
	EffectControls( effect ),
	m_effect( effect ),
	m_freqModel( 100.0f, 50.0f, 200.0f, 1.0f, this, tr( "Frequency" ) ),
	m_gainModel( 1.0f, 0.1f, 5.0f, 0.05f, this, tr( "Gain" ) ),
	m_ratioModel( 2.0f, 0.1f, 10.0f, 0.1f, this, tr( "Ratio" ) )
{
	connect( Engine::audioEngine(), SIGNAL( sampleRateChanged() ), this, SLOT( changeFrequency() ) );
}


void BassBoosterControls::changeFrequency()
{
	m_effect->m_frequencyChangeNeeded = true;
}



void BassBoosterControls::loadSettings( const QDomElement& _this )
{
	m_freqModel.loadSettings( _this, "freq" );
	m_gainModel.loadSettings( _this, "gain" );
	m_ratioModel.loadSettings( _this, "ratio");
}




void BassBoosterControls::saveSettings( QDomDocument& doc, QDomElement& _this )
{
	m_freqModel.saveSettings( doc, _this, "freq" );
	m_gainModel.saveSettings( doc, _this, "gain" );
	m_ratioModel.saveSettings( doc, _this, "ratio");
}


} // namespace lmms
