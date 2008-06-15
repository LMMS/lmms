/*
 * stk_model.cpp - base class for stk instrument models
 *
 * Copyright (c) 2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * 
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

#include "stk_model.h"


stkModel::stkModel( bool _monophonic,
			StkFloat _portamento,
			StkFloat _bend,
			StkFloat _bend_range,
			bool _velocity_sensitive_lpf,
			StkFloat _velocity_sensitive_q,
			StkFloat _volume,
			StkFloat _pan,
			bool _release_triggered,
			bool _randomize_attack,
			StkFloat _randomize_length,
			StkFloat _randomize_velocity_amount,
			StkFloat _randomize_frequency_amount,
			StkFloat _spread ):
	model( NULL ),
	m_monophonic( new boolModel( _monophonic, this ) ),
	m_portamento( new floatModel( _portamento, 0.0f, 1000.0f, 1.0f, this ) ),
	m_bend( new floatModel( _bend, -4096.0f, 4096.0f, 8.192f, this ) ),
	m_bendRange( new floatModel( _bend_range, 0.0f, 24.0f, 0.24f, this ) ),
	m_velocitySensitiveLPF( new boolModel( _velocity_sensitive_lpf, this ) ),
	m_velocitySensitiveQ( new floatModel( _velocity_sensitive_q, basicFilters<>::minQ(), 10.0f, 0.01f, this ) ),
	m_volume( new floatModel( _volume, 0.0f, 1.0f, 0.01f, this ) ),
	m_pan( new floatModel( _pan, -1.0f, 1.0f, 0.02f, this ) ),
	m_releaseTriggered( new boolModel( _release_triggered, this ) ),
	m_randomizeAttack( new boolModel( _randomize_attack, this ) ),
	m_randomizeLength( new floatModel( _randomize_length, 0.0f, 1000.0f, 1.0f, this ) ),
	m_randomizeVelocityAmount( new floatModel( _randomize_velocity_amount, 0.0f, 1.0f, 0.01f, this ) ),
	m_randomizeFrequencyAmount( new floatModel( _randomize_frequency_amount, 0.0f, 4.0f, 0.04f, this ) ),
	m_spread( new floatModel( _spread, 0.0f, 255.0f, 1.0f, this ) )
{
}




stkModel::~stkModel()
{
	delete m_portamento;
	delete m_bend;
	delete m_bendRange;
	delete m_velocitySensitiveLPF;
	delete m_velocitySensitiveQ;
	delete m_volume;
	delete m_pan;
	delete m_releaseTriggered;
	delete m_randomizeAttack;
	delete m_randomizeVelocityAmount;
	delete m_randomizeFrequencyAmount;
	delete m_spread;
}




void stkModel::saveSettings( QDomDocument & _doc, QDomElement & _parent ) 
{
	m_monophonic->saveSettings( _doc, _parent, "monophonic" );
	m_portamento->saveSettings( _doc, _parent, "portamento" );
	m_bend->saveSettings( _doc, _parent, "bend" );
	m_bendRange->saveSettings( _doc, _parent, "bendrange" );
	m_velocitySensitiveLPF->saveSettings( _doc, _parent, "velocitysensitivelpf" );
	m_velocitySensitiveQ->saveSettings( _doc, _parent, "velocitysensitiveq" );
	m_volume->saveSettings( _doc, _parent, "volume" );
	m_pan->saveSettings( _doc, _parent, "pan" );
	m_releaseTriggered->saveSettings( _doc, _parent, "releasetriggered" );
	m_randomizeAttack->saveSettings( _doc, _parent, "randomizeattack" );
	m_randomizeLength->saveSettings( _doc, _parent, "randomizelength" );
	m_randomizeVelocityAmount->saveSettings( _doc, _parent, "randomizevelocityamount" );
	m_randomizeFrequencyAmount->saveSettings( _doc, _parent, "randomizefrequencyamount" );
	m_spread->saveSettings( _doc, _parent, "spread" );
}




void stkModel::loadSettings( const QDomElement & _this ) 
{
	m_monophonic->loadSettings( _this, "monophonic" );
	m_portamento->loadSettings( _this, "portamento" );
	m_bend->loadSettings( _this, "bend" );
	m_bendRange->loadSettings( _this, "bendrange" );
	m_velocitySensitiveLPF->loadSettings( _this, "velocitysensitivelpf" );
	m_velocitySensitiveQ->loadSettings( _this, "velocitysensitiveq" );
	m_volume->loadSettings( _this, "volume" );
	m_pan->loadSettings( _this, "pan" );
	m_releaseTriggered->loadSettings( _this, "releasetriggered" );
	m_randomizeAttack->loadSettings( _this, "randomizeattack" );
	m_randomizeLength->loadSettings( _this, "randomizelength" );
	m_randomizeVelocityAmount->loadSettings( _this, "randomizevelocityamount" );
	m_randomizeFrequencyAmount->loadSettings( _this, "randomizefrequencyamount" );
	m_spread->loadSettings( _this, "spread" );
}


