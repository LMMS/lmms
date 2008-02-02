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


#include "b3_model.h"


b3Model::b3Model( bool _monophonic,
					StkFloat _portamento,
					StkFloat _bend,
					StkFloat _bend_range,
					bool _velocity_sensitive_lpf,
					StkFloat _velocity_sensitive_q,
					StkFloat _volume,
					StkFloat _pan,
					StkFloat _spread,
					StkFloat _operator4,
					StkFloat _operator3,
					StkFloat _lfo_speed,
					StkFloat _lfo_depth,
					StkFloat _adsr_target ):
	stkModel( _monophonic, _portamento, _bend, _bend_range, _velocity_sensitive_lpf, _velocity_sensitive_q, _volume, _pan, _spread ),
	m_operator4( new floatModel( _operator4, 0.0f, 128.0f, 0.1f, this ) ),
	m_operator3( new floatModel( _operator3, 0.0f, 128.0f, 0.1f, this ) ),
	m_lfoSpeed( new floatModel( _lfo_speed, 0.0f, 128.0f, 0.1f, this ) ),
	m_lfoDepth( new floatModel( _lfo_depth, 0.0f, 128.0f, 0.1f, this ) ),
	m_adsrTarget( new floatModel( _adsr_target, 0.0f, 128.0f, 0.1f, this ) )
{
}




b3Model::~b3Model()
{
	delete m_operator4;
	delete m_operator3;
	delete m_lfoSpeed;
	delete m_lfoDepth;
	delete m_adsrTarget;
}




void FASTCALL b3Model::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
	stkModel::saveSettings( _doc, _parent );
	m_operator4->saveSettings( _doc, _parent, "operator4" );
	m_operator3->saveSettings( _doc, _parent, "operator3" );
	m_lfoSpeed->saveSettings( _doc, _parent, "lfospeed" );
	m_lfoDepth->saveSettings( _doc, _parent, "lfodepth" );
	m_adsrTarget->saveSettings( _doc, _parent, "adsrtarget" );
}
	
	
	
	
void FASTCALL b3Model::loadSettings( const QDomElement & _this )
{
	stkModel::loadSettings( _this );
	m_operator4->loadSettings( _this, "operator4" );
	m_operator3->loadSettings( _this, "operator3" );
	m_lfoSpeed->loadSettings( _this, "lfospeed" );
	m_lfoDepth->loadSettings( _this, "lfodepth" );
	m_adsrTarget->loadSettings( _this, "adsrtarget" );
}

