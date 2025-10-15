/*
 * MidiAlsaSeq.h - ALSA-sequencer-client
 *
 * Copyright (c) 2005-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef LMMS_MIDI_ALSA_SEQ_H
#define LMMS_MIDI_ALSA_SEQ_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_ALSA
#include <alsa/asoundlib.h>

#include <QMap>
#include <QMutex>
#include <QThread>
#include <QTimer>


#include "MidiClient.h"




namespace lmms
{


class MidiAlsaSeq : public QThread, public MidiClient
{
	Q_OBJECT
public:
	MidiAlsaSeq();
	~MidiAlsaSeq() override;

	static QString probeDevice();


	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "MidiSetupWidget",
			"ALSA-Sequencer (Advanced Linux Sound "
							"Architecture)" );
	}

	inline static QString configSection()
	{
		return "Midialsaseq";
	}



	void processOutEvent( const MidiEvent & _me,
						const TimePos & _time,
						const MidiPort * _port ) override;

	void applyPortMode( MidiPort * _port ) override;
	void applyPortName( MidiPort * _port ) override;

	void removePort( MidiPort * _port ) override;


	// list seq-ports from ALSA
	QStringList readablePorts() const override
	{
		return m_readablePortMap.keys();
	}

	QStringList writablePorts() const override
	{
		return m_writablePortMap.keys();
	}

	QStringList friendlyReadablePorts() const override
	{
		return m_readablePortMap.values();
	}

	QStringList friendlyWritablePorts() const override
	{
		return m_writablePortMap.values();
	}

	QString toFriendly(const QString& port) const
	{
		return m_readablePortMap.value(port, m_writablePortMap.value(port, port));
	}

	QString fromFriendly(const QString& friendlyPort) const
	{
		QString key = m_readablePortMap.key(friendlyPort);
		if (!key.isEmpty())
			return key;

		key = m_writablePortMap.key(friendlyPort);
		if (!key.isEmpty())
			return key;

		return friendlyPort;
	}

	static int findDeviceIndex(const QStringList& portList, const QString& portName)
	{
		int index = portList.indexOf(portName);
		if (index >= 0)
		{
			return index;
		}

		QStringList portNameSections = portName.split("::");

		if (portNameSections.isEmpty())
		{
			return -1;
		}

		// if item isn't found, try searching without the Client ID (Client ID changes on system restart sometimes)
		portNameSections.pop_front(); // remove Client ID which isn't persistent
		for (QString _port : portList)
		{
			index++;

			QStringList _portSections = _port.split("::");
			_portSections.pop_front();

			if (portNameSections == _portSections)
			{
				return index;
			}
		}

		return -1;
	}


	// return name of port which specified MIDI event came from
	QString sourcePortName( const MidiEvent & ) const override;

	// (un)subscribe given MidiPort to/from destination-port
	void subscribeReadablePort( MidiPort * _port,
						const QString & _dest,
						bool _subscribe = true ) override;
	void subscribeWritablePort( MidiPort * _port,
						const QString & _dest,
						bool _subscribe = true ) override;
	void connectRPChanged( QObject * _receiver,
							const char * _member ) override
	{
		connect( this, SIGNAL( readablePortsChanged() ),
							_receiver, _member );
	}

	void connectWPChanged( QObject * _receiver,
							const char * _member ) override
	{
		connect( this, SIGNAL( writablePortsChanged() ),
							_receiver, _member );
	}


private slots:
	void changeQueueTempo( lmms::bpm_t _bpm );
	void updatePortList();


private:
	void run() override;

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

	//! key: unique name
	//! value: friendly name
	QMap<QString, QString> m_readablePortMap = QMap<QString, QString>();

	//! key: unique name
	//! value: friendly name
	QMap<QString, QString> m_writablePortMap = QMap<QString, QString>();

	int m_pipe[2];


signals:
	void readablePortsChanged();
	void writablePortsChanged();

} ;


} // namespace lmms

#endif // LMMS_HAVE_ALSA

#endif // LMMS_MIDI_ALSA_SEQ_H
