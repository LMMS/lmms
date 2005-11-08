/*
 * midi_client.h - base-class for MIDI-clients like ALSA-sequencer-client
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

	// to be implemented by sub-classes
	virtual void FASTCALL processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port ) = 0;

	// inheriting classes can re-implement this for being able to update
	// their internal port-structures etc.
	virtual void FASTCALL applyPortMode( midiPort * _port );
	virtual void FASTCALL applyPortName( midiPort * _port );

	// inheriting classes can re-implement this although it's actually not
	// neccessary, because they can catch port-mode-changes and do their
	// stuff as soon as port-mode changes from DUMMY to something else
	// re-implemented methods HAVE to call addPort() of base-class!!
	virtual midiPort * FASTCALL addPort( midiEventProcessor * _mep,
						const QString & _name );

	// re-implemented methods HAVE to call removePort() of base-class!!
	virtual void FASTCALL removePort( midiPort * _port );

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
	vvector<midiPort *> m_midiPorts;

} ;




const Uint8 RAW_MIDI_PARSE_BUF_SIZE = 16;


class midiRawClient : public midiClient
{
public:
	midiRawClient( void );
	~midiRawClient();


protected:
	void FASTCALL parseData( const Uint8 _c );

	virtual void FASTCALL sendByte( const Uint8 _c ) = 0;


private:
	void processParsedEvent();
	virtual void FASTCALL processOutEvent( const midiEvent & _me,
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

