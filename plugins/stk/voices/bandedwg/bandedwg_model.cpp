/*
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


#include "bandedwg_model.h"


bandedWGModel::bandedWGModel( bool _monophonic,
					StkFloat _portamento,
					StkFloat _bend,
					StkFloat _bend_range,
					bool _velocity_sensitive_lpf,
					StkFloat _velocity_sensitive_q,
					StkFloat _volume,
					StkFloat _pan,
					StkFloat _spread,
					StkFloat _bow_pressure,
					StkFloat _bow_position,
					StkFloat _vibrato_frequency,
					StkFloat _vibrato_gain,
					StkFloat _bow_velocity,
					StkFloat _set_strike,
					StkFloat _sound ):
	stkModel( _monophonic, _portamento, _bend, _bend_range, _velocity_sensitive_lpf, _velocity_sensitive_q, _volume, _pan, _spread ),
	m_bowPressure( new floatModel( _bow_pressure, 0.0f, 128.0f, 0.1f, this ) ),
	m_bowPosition( new floatModel( _bow_position, 0.0f, 128.0f, 0.1f, this ) ),
	m_vibratoFrequency( new floatModel( _vibrato_frequency, 0.0f, 128.0f, 0.1f, this ) ),
	m_vibratoGain( new floatModel( _vibrato_gain, 0.0f, 128.0f, 0.1f, this ) ),
	m_bowVelocity( new floatModel( _bow_velocity, 0.0f, 128.0f, 0.1f, this ) ),
	m_setStrike( new floatModel( _set_strike, 0.0f, 128.0f, 0.1f, this ) ),
	m_sound( new floatModel( _sound, 0.0f, 3.0f, 1.0f, this ) )
{
}




bandedWGModel::~bandedWGModel()
{
	delete m_bowPressure;
	delete m_bowPosition;
	delete m_vibratoFrequency;
	delete m_vibratoGain;
	delete m_bowVelocity;
	delete m_setStrike;
	delete m_sound;
}




void FASTCALL bandedWGModel::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	stkModel::saveSettings( _doc, _parent );
	m_bowPressure->saveSettings( _doc, _parent, "bowpressure" );
	m_bowPosition->saveSettings( _doc, _parent, "bowgain" );
	m_vibratoFrequency->saveSettings( _doc, _parent, "vibratofrequency" );
	m_vibratoGain->saveSettings( _doc, _parent, "vibratogain" );
	m_bowVelocity->saveSettings( _doc, _parent, "bowvelocity" );
	m_setStrike->saveSettings( _doc, _parent, "setstrike" );
	m_sound->saveSettings( _doc, _parent, "sound" );
}
	
	
	
	
void FASTCALL bandedWGModel::loadSettings( const QDomElement & _this )
{
	stkModel::loadSettings( _this );
	m_bowPressure->loadSettings( _this, "bowpressure" );
	m_bowPosition->loadSettings( _this, "bowposition" );
	m_vibratoFrequency->loadSettings( _this, "vibratofrequency" );
	m_vibratoGain->loadSettings( _this, "vibratogain" );
	m_bowVelocity->loadSettings( _this, "bowvelocity" );
	m_setStrike->loadSettings( _this, "setstrike" );
	m_sound->loadSettings( _this, "sound" );
}

