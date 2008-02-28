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


#include "resonate_model.h"


resonateModel::resonateModel( bool _monophonic,
					StkFloat _portamento,
					StkFloat _bend,
					StkFloat _bend_range,
					bool _velocity_sensitive_lpf,
					StkFloat _velocity_sensitive_q,
					StkFloat _volume,
					StkFloat _pan,
					StkFloat _spread,
					StkFloat _resonance_frequency,
					StkFloat _pole_radii,
					StkFloat _notch_frequency,
					StkFloat _zero_radii ):
	stkModel( _monophonic, _portamento, _bend, _bend_range, _velocity_sensitive_lpf, _velocity_sensitive_q, _volume, _pan, _spread ),
	m_resonanceFrequency( new floatModel( _resonance_frequency, 0.0f, 128.0f, 0.1f, this ) ),
	m_poleRadii( new floatModel( _pole_radii, 0.0f, 128.0f, 0.1f, this ) ),
	m_notchFrequency( new floatModel( _notch_frequency, 0.0f, 128.0f, 0.1f, this ) ),
	m_zeroRadii( new floatModel( _zero_radii, 0.0f, 128.0f, 0.1f, this ) )
{
}




resonateModel::~resonateModel()
{
	delete m_resonanceFrequency;
	delete m_poleRadii;
	delete m_notchFrequency;
	delete m_zeroRadii;
}




void FASTCALL resonateModel::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	stkModel::saveSettings( _doc, _parent );
	m_resonanceFrequency->saveSettings( _doc, _parent, "resonancefrequency" );
	m_poleRadii->saveSettings( _doc, _parent, "poleradii" );
	m_notchFrequency->saveSettings( _doc, _parent, "notchfrequency" );
	m_zeroRadii->saveSettings( _doc, _parent, "zeroradii" );
}
	
	
	
	
void FASTCALL resonateModel::loadSettings( const QDomElement & _this )
{
	stkModel::loadSettings( _this );
	m_resonanceFrequency->loadSettings( _this, "resonancefrequency" );
	m_poleRadii->loadSettings( _this, "poleradii" );
	m_notchFrequency->loadSettings( _this, "notchfrequency" );
	m_zeroRadii->loadSettings( _this, "zeroradii" );
}

