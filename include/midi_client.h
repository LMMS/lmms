/*
 * midi_client.h - base-class for MIDI-clients like ALSA-sequencer-client
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifndef _MIDI_CLIENT_H
#define _MIDI_CLIENT_H

#include "qt3support.h"

#ifdef QT4

#include <QVector>

#else

#include <qvaluevector.h>

#endif


#include "midi.h"
#include "midi_event_processor.h"
#include "tab_widget.h"


class midiPort;


// base-class for all MIDI-clients
class midiClient
{
public:
	midiClient( void );
	virtual ~midiClient();

	virtual midiPort * FASTCALL createPort( midiEventProcessor * _mep,
					const QString & _desired_name ) = 0;

	virtual void FASTCALL processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port ) = 0;

	// validate port-name by trying to change port name in underlying MIDI-
	// subsystem
	virtual void FASTCALL validatePortName( midiPort * _port ) = 0;

	void FASTCALL removePort( midiPort * _port );

	static midiClient * openMidiClient( void );


	class setupWidget : public tabWidget
	{
	public:
		setupWidget( const QString & _caption, QWidget * _parent ) :
			tabWidget( tabWidget::tr( "Settings for %1" ).arg(
							_caption ), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		virtual void saveSettings( void ) = 0;

	} ;


protected:
	inline void addPort( midiPort * _port )
	{
		m_midiPorts.push_back( _port );
	}


	vvector<midiPort *> m_midiPorts;

} ;




const Uint8 RAW_MIDI_PARSE_BUF_SIZE = 16;


class midiRawClient : public midiClient
{
public:
	midiRawClient( void );
	~midiRawClient();


protected:
	virtual midiPort * FASTCALL createPort( midiEventProcessor * _mep,
						const QString & _desired_name );

	virtual void FASTCALL validatePortName( midiPort * _port );

	void FASTCALL parseData( const Uint8 _c );

	virtual void FASTCALL sendByte( const Uint8 _c ) = 0;


private:
	void processParsedEvent();
	void FASTCALL processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port );

	static Uint8 FASTCALL eventLength( const Uint8 _event );


	struct midiParserData
	{
		Uint8 m_status;		// identifies the type of event, that
					// is currently received ('Noteon',
					// 'Pitch Bend' etc).
		Uint8 m_channel;	// The channel of the event that is
					// received (in case of a channel event)
		Uint32 m_bytes;		// How many bytes have been read for
					// the current event?
		Uint32 m_bytesTotal;	// How many bytes does the current
					// event type include?
		Uint32 m_buffer[RAW_MIDI_PARSE_BUF_SIZE];
					// buffer for incoming data
		midiEvent m_midiEvent;	// midi-event 
	} m_midiParseData;
} ;



#endif
