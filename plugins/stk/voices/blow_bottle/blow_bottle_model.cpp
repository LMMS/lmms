/*
 * blow_bottle_model.cpp - data storage for blown bottle noises
 *
 * Copyright (c) 2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include "blow_bottle_model.h"


blowBottleModel::blowBottleModel( bool _monophonic,
					StkFloat _portamento,
					StkFloat _bend,
					StkFloat _bend_range,
					bool _velocity_sensitive_lpf,
					StkFloat _velocity_sensitive_q,
					StkFloat _volume,
					StkFloat _pan,
					StkFloat _spread,
					StkFloat _noise_gain,
					StkFloat _vibrato_frequency,
					StkFloat _vibrato_gain ):
	stkModel( _monophonic, _portamento, _bend, _bend_range, _velocity_sensitive_lpf, _velocity_sensitive_q, _volume, _pan, _spread ),
	m_noiseGain( new floatModel( _noise_gain, 0.0f, 128.0f, 0.1f, this ) ),
	m_vibratoFrequency( new floatModel( _vibrato_frequency, 0.0f, 128.0f, 0.1f, this ) ),
	m_vibratoGain( new floatModel( _vibrato_gain, 0.0f, 128.0f, 0.1f, this ) )
{
}




blowBottleModel::~blowBottleModel()
{
	delete m_noiseGain;
	delete m_vibratoFrequency;
	delete m_vibratoGain;
}




void FASTCALL blowBottleModel::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	stkModel::saveSettings( _doc, _parent );
	m_noiseGain->saveSettings( _doc, _parent, "noisegain" );
	m_vibratoFrequency->saveSettings( _doc, _parent, "vibratofrequency" );
	m_vibratoGain->saveSettings( _doc, _parent, "vibratogain" );
}
	
	
	
	
void FASTCALL blowBottleModel::loadSettings( const QDomElement & _this )
{
	stkModel::loadSettings( _this );
	m_noiseGain->loadSettings( _this, "noisegain" );
	m_vibratoFrequency->loadSettings( _this, "vibratofrequency" );
	m_vibratoGain->loadSettings( _this, "vibratogain" );
}

