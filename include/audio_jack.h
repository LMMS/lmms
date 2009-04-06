/*
 * audio_jack.h - support for JACK-transport
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_JACK_H
#define _AUDIO_JACK_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_JACK
#include <jack/jack.h>
#endif

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QMap>


#include "audio_device.h"


class QLineEdit;
class lcdSpinBox;


class audioJACK : public QObject, public audioDevice
{
	Q_OBJECT
public:
	audioJACK( bool & _success_ful, mixer * _mixer );
	virtual ~audioJACK();

	inline static QString name( void )
	{
		return QT_TRANSLATE_NOOP( "setupWidget",
			"JACK (JACK Audio Connection Kit)" );
	}


	class setupWidget : public audioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings( void );

	private:
		QLineEdit * m_clientName;
		lcdSpinBox * m_channels;

	} ;


private slots:
	void restartAfterZombified( void );


#ifdef LMMS_HAVE_JACK
private:
	bool initJackClient( void );

	virtual void startProcessing( void );
	virtual void stopProcessing( void );
	virtual void applyQualitySettings( void );

	virtual void registerPort( audioPort * _port );
	virtual void unregisterPort( audioPort * _port );
	virtual void renamePort( audioPort * _port );

	int processCallback( jack_nframes_t _nframes, void * _udata );

	static int staticProcessCallback( jack_nframes_t _nframes,
							void * _udata );
	static void shutdownCallback( void * _udata );


	jack_client_t * m_client;

	bool m_active;
	bool m_stopped;

	QSemaphore m_stopSemaphore;

	QVector<jack_port_t *> m_outputPorts;
	surroundSampleFrame * m_outBuf;


	f_cnt_t m_framesDoneInCurBuf;
	f_cnt_t m_framesToDoInCurBuf;


	struct stereoPort
	{
		jack_port_t * ports[2];
	} ;

	typedef QMap<audioPort *, stereoPort> jackPortMap;
	jackPortMap m_portMap;
#endif

signals:
	void zombified( void );

} ;

#endif
