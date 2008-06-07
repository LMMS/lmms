/*
 * midi_client.h - base-class for MIDI-clients like ALSA-sequencer-client
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


#ifndef _MIDI_CLIENT_H
#define _MIDI_CLIENT_H

#include <QtCore/QStringList>
#include <QtCore/QVector>


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
	virtual void processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port ) = 0;

	// inheriting classes can re-implement this for being able to update
	// their internal port-structures etc.
	virtual void applyPortMode( midiPort * _port );
	virtual void applyPortName( midiPort * _port );

	virtual void addPort( midiPort * _port );

	// re-implemented methods HAVE to call removePort() of base-class!!
	virtual void removePort( midiPort * _port );


	// returns whether client works with raw-MIDI, only needs to be
	// re-implemented by midiClientRaw for returning TRUE
	inline virtual bool isRaw( void ) const
	{
		return( FALSE );
	}

	// if not raw-client, return all readable/writeable ports
	virtual const QStringList & readablePorts( void ) const;
	virtual const QStringList & writeablePorts( void ) const;

	// (un)subscribe given midiPort to/from destination-port 
	virtual void subscribeReadablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe = TRUE );
	virtual void subscribeWriteablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe = TRUE );

	// qobject-derived classes can use this for make a slot being
	// connected to signal of non-raw-MIDI-client if port-lists change
	virtual void connectRPChanged( QObject *, const char * )
	{
	}

	virtual void connectWPChanged( QObject *, const char * )
	{
	}

	// tries to open either MIDI-driver from config-file or (if it fails)
	// any other working
	static midiClient * openMidiClient( void );


	class setupWidget : public tabWidget
	{
	public:
		setupWidget( const QString & _caption, QWidget * _parent ) :
			tabWidget( tabWidget::tr( "Settings for %1" ).arg(
					tr( _caption.toAscii() ) ).toUpper(), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		virtual void saveSettings( void ) = 0;


	public slots:
		virtual void show( void )
		{
			parentWidget()->show();
			QWidget::show();
		}

	} ;


protected:
	QVector<midiPort *> m_midiPorts;

} ;




const Uint8 RAW_MIDI_PARSE_BUF_SIZE = 16;


class midiClientRaw : public midiClient
{
public:
	midiClientRaw( void );
	virtual ~midiClientRaw();

	// we are raw-clients for sure!
	inline virtual bool isRaw( void ) const
	{
		return( TRUE );
	}


protected:
	// generic raw-MIDI-parser which generates appropriate MIDI-events
	void parseData( const Uint8 _c );

	// to be implemented by actual client-implementation
	virtual void sendByte( const Uint8 _c ) = 0;


private:
	// this does MIDI-event-process
	void processParsedEvent();
	virtual void processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port );

	// small helper function returning length of a certain event - this
	// is neccessary for parsing raw-MIDI-data
	static Uint8 eventLength( const Uint8 _event );


	// data being used for parsing
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

