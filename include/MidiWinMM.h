/*
 * MidiWinMM.h - WinMM MIDI client
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _MIDI_WINMM_H
#define _MIDI_WINMM_H

#include "lmmsconfig.h"

#ifdef LMMS_BUILD_WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#include "MidiClient.h"
#include "MidiPort.h"


class QLineEdit;


class MidiWinMM : public QObject, public MidiClient
{
	Q_OBJECT
public:
	MidiWinMM();
	virtual ~MidiWinMM();

	static QString probeDevice();


	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget", "WinMM MIDI" );
	}



	virtual void processOutEvent( const MidiEvent & _me,
						const MidiTime & _time,
						const MidiPort * _port );

	virtual void applyPortMode( MidiPort * _port );
	virtual void removePort( MidiPort * _port );


#ifdef LMMS_BUILD_WIN32
	// list devices as ports
	virtual QStringList readablePorts() const
	{
		return m_inputDevices.values();
	}

	virtual QStringList writablePorts() const
	{
		return m_outputDevices.values();
	}
#endif

	// return name of port which specified MIDI event came from
	virtual QString sourcePortName( const MidiEvent & ) const;

	// (un)subscribe given MidiPort to/from destination-port 
	virtual void subscribeReadablePort( MidiPort * _port,
						const QString & _dest,
						bool _subscribe = true );
	virtual void subscribeWritablePort( MidiPort * _port,
						const QString & _dest,
						bool _subscribe = true );
	virtual void connectRPChanged( QObject * _receiver,
							const char * _member )
	{
		connect( this, SIGNAL( readablePortsChanged() ),
							_receiver, _member );
	}

	virtual void connectWPChanged( QObject * _receiver,
							const char * _member )
	{
		connect( this, SIGNAL( writablePortsChanged() ),
							_receiver, _member );
	}

	virtual bool isRaw() const
	{
		return false;
	}


	class setupWidget : public MidiClient::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings()
		{
		}

	} ;


private:// slots:
	void updateDeviceList();


private:
	void openDevices();
	void closeDevices();

#ifdef LMMS_BUILD_WIN32
	static void WINAPI CALLBACK inputCallback( HMIDIIN _hm, UINT _msg,
						DWORD_PTR _inst,
						DWORD_PTR _param1,
							DWORD_PTR _param2 );
	void handleInputEvent( HMIDIIN _hm, DWORD _ev );

	QMap<HMIDIIN, QString> m_inputDevices;
	QMap<HMIDIOUT, QString> m_outputDevices;
#endif

	// subscriptions
	typedef QMap<QString, MidiPortList> SubMap;
	SubMap m_inputSubs;
	SubMap m_outputSubs;


signals:
	void readablePortsChanged();
	void writablePortsChanged();

} ;

#endif

