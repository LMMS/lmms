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


#include "blow_hole_model.h"


blowHoleModel::blowHoleModel( bool _monophonic,
					StkFloat _portamento,
					StkFloat _bend,
					StkFloat _bend_range,
					bool _velocity_sensitive_lpf,
					StkFloat _velocity_sensitive_q,
					StkFloat _volume,
					StkFloat _pan,
					StkFloat _spread,
					StkFloat _reed_stiffness,
					StkFloat _noise_gain,
					StkFloat _tonehole_state,
					StkFloat _register_state,
					StkFloat _breath_pressure ):
	stkModel( _monophonic, _portamento, _bend, _bend_range, _velocity_sensitive_lpf, _velocity_sensitive_q, _volume, _pan, _spread ),
	m_reedStiffness( new floatModel( _reed_stiffness, 0.0f, 128.0f, 0.1f, this ) ),
	m_noiseGain( new floatModel( _noise_gain, 0.0f, 128.0f, 0.1f, this ) ),
	m_toneholeState( new floatModel( _tonehole_state, 0.0f, 128.0f, 0.1f, this ) ),
	m_registerState( new floatModel( _register_state, 0.0f, 128.0f, 0.1f, this ) ),
	m_breathPressure( new floatModel( _breath_pressure, 0.0f, 128.0f, 0.1f, this ) )
{
}




blowHoleModel::~blowHoleModel()
{
	delete m_reedStiffness;
	delete m_noiseGain;
	delete m_toneholeState;
	delete m_registerState;
	delete m_breathPressure;
}




void blowHoleModel::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	stkModel::saveSettings( _doc, _parent );
	m_reedStiffness->saveSettings( _doc, _parent, "reedstiffness" );
	m_noiseGain->saveSettings( _doc, _parent, "noisegain" );
	m_toneholeState->saveSettings( _doc, _parent, "toneholestate" );
	m_registerState->saveSettings( _doc, _parent, "registerstate" );
	m_breathPressure->saveSettings( _doc, _parent, "breathpressure" );
}
	
	
	
	
void blowHoleModel::loadSettings( const QDomElement & _this )
{
	stkModel::loadSettings( _this );
	m_reedStiffness->loadSettings( _this, "reedstiffness" );
	m_noiseGain->loadSettings( _this, "noisegain" );
	m_toneholeState->loadSettings( _this, "toneholestate" );
	m_registerState->loadSettings( _this, "registerstate" );
	m_breathPressure->loadSettings( _this, "breathpressure" );
}

