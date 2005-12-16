/*
 * midi_alsa_seq.h - ALSA-sequencer-client
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MIDI_ALSA_SEQ_H
#define _MIDI_ALSA_SEQ_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ALSA_ASOUNDLIB_H

#ifndef ALSA_SUPPORT
#define ALSA_SUPPORT
#endif

#include <alsa/asoundlib.h>

#endif

#include "qt3support.h"

#ifdef QT4

#include <QThread>
#include <QTimer>

#else

#include <qobject.h>
#include <qthread.h>
#include <qtimer.h>

#endif


#include "midi_client.h"


struct pollfd;
class QLineEdit;


class midiALSASeq :
#ifndef QT4
			public QObject,
#endif
			public midiClient, public QThread
{
	Q_OBJECT
public:
	midiALSASeq( void );
	~midiALSASeq();

	static QString probeDevice( void );


	inline static QString name( void )
	{
		return( setupWidget::tr( "ALSA-Sequencer (Advanced Linux Sound "
							"Architecture)" ) );
	}



	virtual void FASTCALL processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port );

	virtual void FASTCALL applyPortMode( midiPort * _port );
	virtual void FASTCALL applyPortName( midiPort * _port );

	virtual void FASTCALL removePort( midiPort * _port );


	// list seq-ports from ALSA 
	inline virtual const QStringList & readablePorts( void ) const
	{
		return( m_readablePorts );
	}

	virtual const QStringList & writeablePorts( void ) const
	{
		return( m_writeablePorts );
	}

	// (un)subscribe given midiPort to/from destination-port 
	virtual void subscribeReadablePort( midiPort * _port,
						const QString & _dest,
						bool _unsubscribe = FALSE );
	virtual void subscribeWriteablePort( midiPort * _port,
						const QString & _dest,
						bool _unsubscribe = FALSE );
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


	class setupWidget : public midiClient::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings( void );

	private:
		QLineEdit * m_device;

	} ;


private slots:
	void changeQueueTempo( int _bpm );
	void updatePortList( void );


private:
	virtual void run( void );
#ifdef ALSA_SUPPORT

	snd_seq_t * m_seqHandle;
	struct ports
	{
		ports() { p[0] = -1; p[1] = -1; }
		int & operator[]( const int _i ) { return( p[_i] ); }
		private: int p[2];
	} ;
	QMap<midiPort *, ports> m_portIDs;

	int m_queueID;

	volatile bool m_quit;
#endif

	QTimer m_portListUpdateTimer;
	QStringList m_readablePorts;
	QStringList m_writeablePorts;


signals:
	void readablePortsChanged( void );
	void writeablePortsChanged( void );

} ;

#endif

