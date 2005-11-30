/*
 * instrument.cpp - base-class for all instrument-plugins (synths, samplers etc)
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "instrument.h"
#include "channel_track.h"
#include "dummy_instrument.h"


instrument::instrument( channelTrack * _channel_track, const QString & _name ) :
	QWidget( _channel_track->tabWidgetParent() ),
	plugin( _name, INSTRUMENT ),
	m_channelTrack( _channel_track ),
	m_valid( TRUE )
{
	setFixedSize( 250, 250 );
}




instrument::~instrument()
{
}




void instrument::play( void )
{
}




void instrument::playNote( notePlayHandle * )
{
}




void instrument::deleteNotePluginData( notePlayHandle * )
{
}




Uint32 instrument::beatLen( notePlayHandle * ) const
{
	return( 0 );
}




instrument * instrument::instantiate( const QString & _plugin_name,
						channelTrack * _channel_track )
{
	plugin * p = plugin::instantiate( _plugin_name, _channel_track );
	// check whether instantiated plugin is an instrument
	if( dynamic_cast<instrument *>( p ) != NULL )
	{
		// everything ok, so return pointer
		return( dynamic_cast<instrument *>( p ) );
	}

	// not quite... so delete plugin and return dummy instrument
	delete p;
	return( new dummyInstrument( _channel_track ) );
}

