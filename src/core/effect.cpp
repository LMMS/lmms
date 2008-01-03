#ifndef SINGLE_SOURCE_COMPILE

/*
 * effect.cpp - base-class for effects
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QMessageBox>

#include "effect.h"
#include "engine.h"
#include "dummy_effect.h"


effect::effect( const plugin::descriptor * _desc,
			const descriptor::subPluginFeatures::key * _key ) :
	plugin( _desc ),
	m_key( _key ? *_key : descriptor::subPluginFeatures::key()  ),
	m_okay( TRUE ),
	m_noRun( FALSE ),
	m_running( FALSE ),
	m_bypass( FALSE ),
	m_bufferCount( 0 ),
	m_silenceTimeout( 10 ),
	m_wetDry( 1.0f ),
	m_gate( 0.0f )
{
}




effect::~effect()
{
}




bool FASTCALL effect::processAudioBuffer( surroundSampleFrame * _buf, 
							const fpp_t _frames )
{
	return( FALSE );
}




void FASTCALL effect::setGate( float _level )
{
	m_gate = _level * _level * m_processors * 
				engine::getMixer()->framesPerPeriod();
}


effect * effect::instantiate( const QString & _plugin_name,
				descriptor::subPluginFeatures::key * _key )
{
	plugin * p = plugin::instantiate( _plugin_name, _key );
	// check whether instantiated plugin is an instrument
	if( dynamic_cast<effect *>( p ) != NULL )
	{
		// everything ok, so return pointer
		return( dynamic_cast<effect *>( p ) );
	}

	// not quite... so delete plugin and return dummy instrument
	delete p;
	return( new dummyEffect() );
}



#endif
