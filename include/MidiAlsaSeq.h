/*
 * MidiAlsaSeq.h - ALSA-sequencer-client
 *
 * Copyright (c) 2005-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _MIDI_ALSA_SEQ_H
#define _MIDI_ALSA_SEQ_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_ALSA
#include <alsa/asoundlib.h>
#endif

#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QTimer>


#include "MidiClient.h"


struct pollfd;
class QLineEdit;


class MidiAlsaSeq : public QThread, public MidiClient
{
	Q_OBJECT
public:
	MidiAlsaSeq();
	virtual ~MidiAlsaSeq();

	static QString probeDevice();


	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget",
			"ALSA-Sequencer (Advanced Linux Sound "
							"Architecture)" );
	}



	virtual void processOutEvent( const MidiEvent & _me,
						const MidiTime & _time,
						const MidiPort * _port );

	virtual void applyPortMode( MidiPort * _port );
	virtual void applyPortName( MidiPort * _port );

	virtual void removePort( MidiPort * _port );


	// list seq-ports from ALSA 
	virtual QStringList readablePorts() const
	{
		return m_readablePorts;
	}

	virtual QStringList writablePorts() const
	{
		return m_writablePorts;
	}

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


	class setupWidget : public MidiClient::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings();

	private:
		QLineEdit * m_device;

	} ;


private slots:
	void changeQueueTempo( bpm_t _bpm );
	void updatePortList();


private:
	virtual void run();

#ifdef LMMS_HAVE_ALSA
	QMutex m_seqMutex;
	snd_seq_t * m_seqHandle;
	struct Ports
	{
		Ports() { p[0] = -1; p[1] = -1; }
		int & operator[]( const int _i ) { return p[_i]; }
		private: int p[2];
	} ;
	QMap<MidiPort *, Ports> m_portIDs;
#endif

	int m_queueID;

	volatile bool m_quit;

	QTimer m_portListUpdateTimer;
	QStringList m_readablePorts;
	QStringList m_writablePorts;

	int m_pipe[2];


signals:
	void readablePortsChanged();
	void writablePortsChanged();

} ;

#endif

