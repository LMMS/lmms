#ifndef SINGLE_SOURCE_COMPILE

/*
 * instrument.cpp - base-class for all instrument-plugins (synths, samplers etc)
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "instrument.h"
#include "instrument_track.h"
#include "dummy_instrument.h"


instrument::instrument( instrumentTrack * _instrument_track,
					const descriptor * _descriptor ) :
	QWidget( _instrument_track->tabWidgetParent() ),
	plugin( _descriptor, _instrument_track->eng() ),
	m_instrumentTrack( _instrument_track ),
	m_valid( TRUE )
{
	setFixedSize( 250, 250 );
	m_instrumentTrack->setWindowIcon( *getDescriptor()->logo );
}




instrument::~instrument()
{
}




void instrument::play( bool )
{
}




void instrument::playNote( notePlayHandle *, bool )
{
}




void instrument::deleteNotePluginData( notePlayHandle * )
{
}




f_cnt_t instrument::beatLen( notePlayHandle * ) const
{
	return( 0 );
}




instrument * instrument::instantiate( const QString & _plugin_name,
					instrumentTrack * _instrument_track )
{
	plugin * p = plugin::instantiate( _plugin_name, _instrument_track );
	// check whether instantiated plugin is an instrument
	if( dynamic_cast<instrument *>( p ) != NULL )
	{
		// everything ok, so return pointer
		return( dynamic_cast<instrument *>( p ) );
	}

	// not quite... so delete plugin and return dummy instrument
	delete p;
	return( new dummyInstrument( _instrument_track ) );
}


#endif
