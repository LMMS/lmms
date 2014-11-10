/*
 * DummyInstrument.h - instrument used as fallback if an instrument couldn't
 *                     be loaded
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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

#include "Instrument.h"
#include "InstrumentView.h"


class DummyInstrument : public Instrument
{
public:
	DummyInstrument( InstrumentTrack * _instrument_track ) :
		Instrument( _instrument_track, NULL )
	{
	}

	virtual ~DummyInstrument()
	{
	}

	virtual void playNote( NotePlayHandle *, sampleFrame * )
	{
	}

	virtual void saveSettings( QDomDocument &, QDomElement & )
	{
	}

	virtual void loadSettings( const QDomElement & )
	{
	}

	virtual QString nodeName() const
	{
		return "dummyinstrument";
	}

	virtual PluginView * instantiateView( QWidget * _parent )
	{
		return new InstrumentView( this, _parent );
	}
} ;


#endif
