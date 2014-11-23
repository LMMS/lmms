/*
 * AudioJack.h - support for JACK-transport
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _AUDIO_JACK_H
#define _AUDIO_JACK_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_JACK
#include <jack/jack.h>
#endif

#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QMap>

#include "AudioDevice.h"


class QLineEdit;
class LcdSpinBox;


class AudioJack : public QObject, public AudioDevice
{
	Q_OBJECT
public:
	AudioJack( bool & _success_ful, Mixer* mixer );
	virtual ~AudioJack();

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget",
			"JACK (JACK Audio Connection Kit)" );
	}


	class setupWidget : public AudioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings();

	private:
		QLineEdit * m_clientName;
		LcdSpinBox * m_channels;

	} ;


private slots:
	void restartAfterZombified();


#ifdef LMMS_HAVE_JACK
private:
	bool initJackClient();

	virtual void startProcessing();
	virtual void stopProcessing();
	virtual void applyQualitySettings();

	virtual void registerPort( AudioPort * _port );
	virtual void unregisterPort( AudioPort * _port );
	virtual void renamePort( AudioPort * _port );

	int processCallback( jack_nframes_t _nframes, void * _udata );

	static int staticProcessCallback( jack_nframes_t _nframes,
							void * _udata );
	static void shutdownCallback( void * _udata );


	jack_client_t * m_client;

	bool m_active;
	bool m_stopped;

	QSemaphore m_stopSemaphore;

	QVector<jack_port_t *> m_outputPorts;
	jack_default_audio_sample_t * * m_tempOutBufs;
	surroundSampleFrame * m_outBuf;

	f_cnt_t m_framesDoneInCurBuf;
	f_cnt_t m_framesToDoInCurBuf;


#ifdef AUDIO_PORT_SUPPORT
	struct StereoPort
	{
		jack_port_t * ports[2];
	} ;

	typedef QMap<AudioPort *, StereoPort> JackPortMap;
	JackPortMap m_portMap;
#endif
#endif

signals:
	void zombified();

} ;

#endif
