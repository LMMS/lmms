/*
 * dummy_instrument.h - instrument used as fallback if an instrument couldn't
 *                      be loaded
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _DUMMY_INSTRUMENT_H
#define _DUMMY_INSTRUMENT_H

#include "instrument.h"
#include "instrument_view.h"


class dummyInstrument : public instrument
{
public:
	dummyInstrument( instrumentTrack * _instrument_track ) :
		instrument( _instrument_track, NULL )
	{
	}

	virtual ~dummyInstrument()
	{
	}


	virtual void saveSettings( QDomDocument &, QDomElement & )
	{
	}

	virtual void loadSettings( const QDomElement & )
	{
	}

	virtual QString nodeName( void ) const
	{
		return( "dummyinstrument" );
	}

	virtual pluginView * instantiateView( QWidget * _parent )
	{
		return( new instrumentView( this, _parent ) );
	}
} ;


#endif
