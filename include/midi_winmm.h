/*
 * midi_winmm.h - WinMM MIDI client
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MIDI_WINMM_H
#define _MIDI_WINMM_H

#include "lmmsconfig.h"

#ifdef LMMS_BUILD_WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#include <QtCore/QTimer>

#include "midi_client.h"
#include "midi_port.h"


class QLineEdit;


class midiWinMM : public QObject, public midiClient
{
	Q_OBJECT
public:
	midiWinMM( void );
	virtual ~midiWinMM();

	static QString probeDevice( void );


	inline static QString name( void )
	{
		return( QT_TRANSLATE_NOOP( "setupWidget", "WinMM MIDI" ) );
	}



	virtual void processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port );

	virtual void applyPortMode( midiPort * _port );
	virtual void removePort( midiPort * _port );


#ifdef LMMS_BUILD_WIN32
	// list devices as ports
	virtual QStringList readablePorts( void ) const
	{
		return( m_inputDevices.values() );
	}

	virtual QStringList writeablePorts( void ) const
	{
		return( m_outputDevices.values() );
	}
#endif

	// (un)subscribe given midiPort to/from destination-port 
	virtual void subscribeReadablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe = TRUE );
	virtual void subscribeWriteablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe = TRUE );
	virtual void connectRPChanged( QObject * _receiver,
							const char * _member )
	{
		connect( this, SIGNAL( readablePortsChanged() ),
							_receiver, _member );
	}

	virtual void connectWPChanged( QObject * _receiver,
							const char * _member )
	{
		connect( this, SIGNAL( writeablePortsChanged() ),
							_receiver, _member );
	}

	virtual bool isRaw( void ) const
	{
		return( FALSE );
	}


	class setupWidget : public midiClient::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

	} ;


private slots:
	void updateDeviceList( void );


private:
	void openDevices( void );
	void closeDevices( void );

#ifdef LMMS_BUILD_WIN32
	static void CALLBACK inputCallback( HMIDIIN _hm, UINT _msg,
						DWORD_PTR _inst,
						DWORD_PTR _param1,
							DWORD_PTR _param2 );
	void handleInputEvent( HMIDIIN _hm, DWORD _ev );

	QTimer m_deviceListUpdateTimer;
	QMap<HMIDIIN, QString> m_inputDevices;
	QMap<HMIDIOUT, QString> m_outputDevices;
#endif

	// subscriptions
	typedef QMap<QString, midiPortList> subMap;
	subMap m_inputSubs;
	subMap m_outputSubs;


signals:
	void readablePortsChanged( void );
	void writeablePortsChanged( void );

} ;

#endif

