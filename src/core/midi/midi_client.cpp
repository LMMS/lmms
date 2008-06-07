#ifndef SINGLE_SOURCE_COMPILE

/*
 * midi_client.cpp - base-class for MIDI-clients like ALSA-sequencer-client
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * This file partly contains code from Fluidsynth, Peter Hanappe
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


#include "midi_client.h"
/*#include "midi_mapper.h"*/
#include "templates.h"
#include "midi_port.h"
#include "note.h"




midiClient::midiClient( void )
{
}




midiClient::~midiClient()
{
	//TODO: noteOffAll(); / clear all ports
}




void midiClient::applyPortMode( midiPort * )
{
}




void midiClient::applyPortName( midiPort * )
{
}




void midiClient::addPort( midiPort * _port )
{
	m_midiPorts.push_back( _port );
}




void midiClient::removePort( midiPort * _port )
{
	QVector<midiPort *>::iterator it = qFind( m_midiPorts.begin(),
							m_midiPorts.end(),
							_port );
	if( it != m_midiPorts.end() )
	{
		m_midiPorts.erase( it );
	}
}




const QStringList & midiClient::readablePorts( void ) const
{
	static QStringList sl;
	return( sl );
}




const QStringList & midiClient::writeablePorts( void ) const
{
	static QStringList sl;
	return( sl );
}




void midiClient::subscribeReadablePort( midiPort *, const QString & , bool )
{
}




void midiClient::subscribeWriteablePort( midiPort * , const QString & , bool)
{
}







midiClientRaw::midiClientRaw( void )
{
}




midiClientRaw::~midiClientRaw()
{
}




void midiClientRaw::parseData( const Uint8 _c )
{
	/*********************************************************************/
	/* 'Process' system real-time messages                               */
	/*********************************************************************/
	/* There are not too many real-time messages that are of interest here.
	 * They can occur anywhere, even in the middle of a noteon message! 
	 * Real-time range: 0xF8 .. 0xFF
	 * Note: Real-time does not affect (running) status.
	 */  
	if( _c >= 0xF8 )
	{
		if( _c == MidiSystemReset )
		{
			m_midiParseData.m_midiEvent.m_type = MidiSystemReset;
			m_midiParseData.m_status = 0;
			processParsedEvent();
		}
		return;
	}

	/*********************************************************************/
	/* 'Process' system common messages (again, just skip them)          */
	/*********************************************************************/
	/* There are no system common messages that are of interest here.
	 * System common range: 0xF0 .. 0xF7 
	 */
	if( _c > 0xF0 )
	{
	/* MIDI spec say: To ignore a non-real-time message, just discard all
	 * data up to the next status byte.  And our parser will ignore data
	 * that is received without a valid status.  
	 * Note: system common cancels running status. */
		m_midiParseData.m_status = 0;
		return;
	}

	/*********************************************************************/
	/* Process voice category messages:                                  */
	/*********************************************************************/
	/* Now that we have handled realtime and system common messages, only
	 * voice messages are left.
	 * Only a status byte has bit # 7 set.
	 * So no matter the status of the parser (in case we have lost sync),
	 * as soon as a byte >= 0x80 comes in, we are dealing with a status byte
	 * and start a new event.
	 */
	if( _c & 0x80 )
	{
		m_midiParseData.m_channel = _c & 0x0F;
		m_midiParseData.m_status = _c & 0xF0;
		/* The event consumes x bytes of data...
					(subtract 1 for the status byte) */
		m_midiParseData.m_bytesTotal = eventLength(
						m_midiParseData.m_status ) - 1;
		/* of which we have read 0 at this time. */
		m_midiParseData.m_bytes = 0;
		return;
	}

	/*********************************************************************/
	/* Process data                                                      */
	/*********************************************************************/
	/* If we made it this far, then the received char belongs to the data
	 * of the last event. */
	if( m_midiParseData.m_status == 0 )
	{
		/* We are not interested in the event currently received.
							Discard the data. */
		return;
	}

	/* Store the first couple of bytes */
	if( m_midiParseData.m_bytes < RAW_MIDI_PARSE_BUF_SIZE )
	{
		m_midiParseData.m_buffer[m_midiParseData.m_bytes] = _c;
	}
	++m_midiParseData.m_bytes;

	/* Do we still need more data to get this event complete? */
	if( m_midiParseData.m_bytes < m_midiParseData.m_bytesTotal )
	{
		return;
	}

	/*********************************************************************/
	/* Send the event                                                    */
	/*********************************************************************/
	/* The event is ready-to-go.  About 'running status': 
	 * 
	 * The MIDI protocol has a built-in compression mechanism. If several
	 * similar events are sent in-a-row, for example note-ons, then the
	 * event type is only sent once. For this case, the last event type
	 * (status) is remembered.
	 * We simply keep the status as it is, just reset the parameter counter.
	 * If another status byte comes in, it will overwrite the status. 
	 */
	m_midiParseData.m_midiEvent.m_type = static_cast<MidiEventTypes>(
						m_midiParseData.m_status );
	m_midiParseData.m_midiEvent.m_channel = m_midiParseData.m_channel;
	m_midiParseData.m_bytes = 0; /* Related to running status! */
	switch( m_midiParseData.m_midiEvent.m_type )
	{
		case MidiNoteOff:
		case MidiNoteOn:
		case MidiKeyPressure:
		case MidiProgramChange:
		case MidiChannelPressure:
			m_midiParseData.m_midiEvent.m_data.m_param[0] =
				m_midiParseData.m_buffer[0] - KeysPerOctave;
			m_midiParseData.m_midiEvent.m_data.m_param[1] =
						m_midiParseData.m_buffer[1];
		case MidiControlChange:
			m_midiParseData.m_midiEvent.m_data.m_param[0] =
				m_midiParseData.m_buffer[0] - KeysPerOctave;
			m_midiParseData.m_midiEvent.m_data.m_param[1] =
						m_midiParseData.m_buffer[1];
			break;

		case MidiPitchBend:
			// Pitch-bend is transmitted with 14-bit precision.
			// Note: '|' does here the same as '+' (no common bits),
			// but might be faster
			m_midiParseData.m_midiEvent.m_data.m_param[0] =
				( ( m_midiParseData.m_buffer[1] * 128 ) |
						m_midiParseData.m_buffer[0] );
			break;

		default: 
			// Unlikely
			return;
	}

	processParsedEvent();
}




void midiClientRaw::processParsedEvent()
{
	for( int i = 0; i < m_midiPorts.size(); ++i )
	{
		m_midiPorts[i]->processInEvent( m_midiParseData.m_midiEvent,
							midiTime() );
	}
}




void midiClientRaw::processOutEvent( const midiEvent & _me,
							const midiTime & ,
							const midiPort * _port )
{
	// TODO: also evaluate _time and queue event if neccessary
	switch( _me.m_type )
	{
		case MidiNoteOn:
		case MidiNoteOff:
		case MidiKeyPressure:
			if( _port->outputChannel() >= 0 )
			{
				sendByte( _me.m_type | _port->outputChannel() );
				sendByte( _me.m_data.m_param[0] +
							KeysPerOctave );
				sendByte( tLimit( (int) _me.m_data.m_param[1],
								0, 127 ) );
			}
			else
			{
				for( Sint8 i = 0; i < MidiChannelCount; ++i )
				{
					sendByte( _me.m_type | i );
					sendByte( _me.m_data.m_param[0] +
							KeysPerOctave );
					sendByte( tLimit( (int)
						_me.m_data.m_param[1],
								0, 127 ) );
				}
			}
			break;

		default:
			printf( "midiClientRaw: unhandled MIDI-event %d\n",
							(int) _me.m_type );
			break;
	}
}






// Taken from Nagano Daisuke's USB-MIDI driver
const Uint8 REMAINS_F0F6[] =
{
	0,	/* 0xF0 */
	2,	/* 0XF1 */
	3,	/* 0XF2 */
	2,	/* 0XF3 */
	2,	/* 0XF4 (Undefined by MIDI Spec, and subject to change) */
	2,	/* 0XF5 (Undefined by MIDI Spec, and subject to change) */
	1	/* 0XF6 */
} ;

const Uint8 REMAINS_80E0[] =
{
	3,	/* 0x8X Note Off */
	3,	/* 0x9X Note On */
	3,	/* 0xAX Poly-key pressure */
	3,	/* 0xBX Control Change */
	2,	/* 0xCX Program Change */
	2,	/* 0xDX Channel pressure */
	3 	/* 0xEX PitchBend Change */
} ;



// Returns the length of the MIDI message starting with _event.
// Taken from Nagano Daisuke's USB-MIDI driver
Uint8 midiClientRaw::eventLength( const Uint8 _event )
{
	if ( _event < 0xF0 )
	{
		return( REMAINS_80E0[( ( _event - 0x80 ) >> 4 ) & 0x0F] );
	}
	else if ( _event < 0xF7 )
	{
		return( REMAINS_F0F6[_event - 0xF0] );
	}
	return( 1 );
}


#endif
