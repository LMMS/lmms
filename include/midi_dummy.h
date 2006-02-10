/*
 * midi_dummy.h - dummy MIDI-driver
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _MIDI_DUMMY_H
#define _MIDI_DUMMY_H


#include "midi_client.h"
#include "midi_port.h"
#include "tab_widget.h"


class midiDummy : public midiClientRaw
{
public:
	midiDummy( engine * _engine ) :
		midiClientRaw( _engine )
	{
	}
	virtual ~midiDummy()
	{
	}

	inline static QString name( void )
	{
		return( setupWidget::tr( "Dummy (no MIDI support)" ) );
	}


	class setupWidget : public midiClient::setupWidget
	{
	public:
		setupWidget( QWidget * _parent ) :
			midiClientRaw::setupWidget( midiDummy::name(), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		virtual void saveSettings( void )
		{
		}

	} ;


protected:
	virtual void FASTCALL sendByte( const Uint8 )
	{
	}

} ;


#endif
