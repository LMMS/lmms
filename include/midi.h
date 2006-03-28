/*
 * midi.h - constants, structs etc. concerning MIDI
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


#ifndef _MIDI_H
#define _MIDI_H


#include "types.h"



enum midiEventTypes
{
	// messages
	NOTE_OFF = 0x80,
	NOTE_ON = 0x90,
	KEY_PRESSURE = 0xA0,
	CONTROL_CHANGE = 0xB0,
	PROGRAM_CHANGE = 0xC0,
	CHANNEL_PRESSURE = 0xD0,
	PITCH_BEND = 0xE0,
	// system exclusive
	MIDI_SYSEX = 0xF0,
	// system common - never in midi files
	MIDI_TIME_CODE = 0xF1,
	MIDI_SONG_POSITION = 0xF2,
	MIDI_SONG_SELECT = 0xF3,
	MIDI_TUNE_REQUEST = 0xF6,
	MIDI_EOX = 0xF7,
	// system real-time - never in midi files
	MIDI_SYNC = 0xF8,
	MIDI_TICK = 0xF9,
	MIDI_START = 0xFA,
	MIDI_CONTINUE = 0xFB,
	MIDI_STOP = 0xFC,
	MIDI_ACTIVE_SENSING = 0xFE,
	MIDI_SYSTEM_RESET = 0xFF,
	// meta event - for midi files only
	MIDI_META_EVENT = 0xFF
} ;


const Sint8 MIDI_CHANNEL_COUNT = 16;


struct midiEvent
{
	midiEvent( midiEventTypes _type = MIDI_ACTIVE_SENSING,
			Sint8 _channel = 0,
			Uint16 _param1 = 0,
			Uint16 _param2 = 0 ) :
		m_type( _type ),
		m_channel( _channel ),
		m_sysExData( NULL )
	{
		m_data.m_param[0] = _param1;
		m_data.m_param[1] = _param2;
	}
	midiEvent( midiEventTypes _type, const char * _sysex_data,
							int _data_len ) :
		m_type( _type ),
		m_channel( 0 ),
		m_sysExData( _sysex_data )
	{
		m_data.m_sysExDataLen = _data_len;
	}

	inline Uint16 key( void ) const
	{
		return( m_data.m_param[0] );
	}

	inline Uint16 & key( void )
	{
		return( m_data.m_param[0] );
	}

	inline Uint16 velocity( void ) const
	{
		return( m_data.m_param[1] );
	}

	inline Uint16 & velocity( void )
	{
		return( m_data.m_param[1] );
	}

	midiEventTypes m_type;		// MIDI event type
	Sint8 m_channel;		// MIDI channel
	union
	{
		Uint16 m_param[2];	// first/second parameter (key/velocity)
		Uint32 m_sysExDataLen;	// len of m_sysExData
	} m_data;

	const char * m_sysExData;

} ;


#endif
