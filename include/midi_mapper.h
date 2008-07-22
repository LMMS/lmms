#if 0
/*
 * midi_mapper.h - MIDI-mapper for any midiDevice
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MIDI_MAPPER_H
#define _MIDI_MAPPER_H

#include <QtCore/QPair>
#include <QtCore/QFile>

#include "types.h"
#include "midi.h"


const Uint8 MIDI_PROGRAMS = 128;
const Uint8 MIDI_KEYS     = 128;


class midiMapper
{
public:
	midiMapper( const QString & _map );
	~midiMapper();

	inline const QString & programName( Uint8 _program ) const
	{
		return( m_patchMap[_program].second );
	}

	inline Uint8 mappedProgramNumber( Uint8 _program ) const
	{
		return( m_patchMap[_program].first );
	}

	inline const QString & drumsetKeyName( Uint8 _key ) const
	{
		return( m_drumsetKeyMap[_key].second );
	}
	inline Uint8 keyForDrumName( const QString & _name ) const
	{
		for( Uint8 i = 0; i < MIDI_KEYS; ++i )
		{
			if( m_drumsetKeyMap[i].second == _name )
			{
				return( i );
			}
		}
		return( 0 );
	}

	inline Uint8 drumsetChannel( void ) const
	{
		return( m_drumsetChannel );
	}

	inline Uint8 drumsetPatch( void ) const
	{
		return( m_drumsetPatch );
	}

	inline Uint8 mappedChannel( Uint8 _channel ) const
	{
		return( m_channelMap[_channel] );
	}


private:
	void readPatchMap( QFile & _f );
	void readDrumsetKeyMap( QFile & _f );
	void readChannelMap( QFile & _f );

	QPair<Uint8, QString> m_patchMap[MIDI_PROGRAMS];
	QPair<Uint8, QString> m_drumsetKeyMap[MIDI_KEYS];
	Uint8 m_channelMap[MIDI_CHANNEL_COUNT];
	Uint8 m_drumsetChannel;
	Uint8 m_drumsetPatch;

} ;


#endif
#endif
