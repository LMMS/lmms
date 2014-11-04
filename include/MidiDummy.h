/*
 * MidiDummy.h - dummy MIDI-driver
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _MIDI_DUMMY_H
#define _MIDI_DUMMY_H

#include "MidiClient.h"


class MidiDummy : public MidiClientRaw
{
public:
	MidiDummy()
	{
	}
	virtual ~MidiDummy()
	{
	}

	inline static QString name()
	{
		return( QT_TRANSLATE_NOOP( "setupWidget",
			"Dummy (no MIDI support)" ) );
	}


	class setupWidget : public MidiClient::setupWidget
	{
	public:
		setupWidget( QWidget * _parent ) :
			MidiClientRaw::setupWidget( MidiDummy::name(), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		virtual void saveSettings()
		{
		}

		virtual void show()
		{
			parentWidget()->hide();
			QWidget::show();
		}

	} ;


protected:
	virtual void sendByte( const unsigned char )
	{
	}

} ;


#endif
