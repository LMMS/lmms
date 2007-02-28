/*
 * audio_jack.h - support for JACK-transport
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_JACK_JACK_H

#define JACK_SUPPORT

#include <jack/jack.h>


#include "qt3support.h"

#ifdef QT4

#include <QtCore/QMutex>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QMap>

#else

#include <qmutex.h>
#include <qvaluevector.h>
#include <qvaluelist.h>
#include <qmap.h>

#endif


#include "audio_device.h"
#include "tab_widget.h"


class QLineEdit;
class lcdSpinBox;


class audioJACK : public audioDevice
{
public:
	audioJACK( const sample_rate_t _sample_rate, bool & _success_ful,
							mixer * _mixer );
	virtual ~audioJACK();

	inline static QString name( void )
	{
		return( QT_TRANSLATE_NOOP( "setupWidget",
			"JACK (JACK Audio Connection Kit)" ) );
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


private:
	virtual void startProcessing( void );
	virtual void stopProcessing( void );

	virtual void registerPort( audioPort * _port );
	virtual void unregisterPort( audioPort * _port );
	virtual void renamePort( audioPort * _port );

	static int processCallback( jack_nframes_t _nframes, void * _udata );
	static void shutdownCallback( void * _udata );


	jack_client_t * m_client;

	bool m_stopped;

	QMutex m_processCallbackMutex;

	vvector<jack_port_t *> m_outputPorts;
	surroundSampleFrame * m_outBuf;


	f_cnt_t m_framesDoneInCurBuf;
	f_cnt_t m_framesToDoInCurBuf;


	struct stereoPort
	{
		jack_port_t * ports[2];
	} ;

	typedef QMap<audioPort *, stereoPort> jackPortMap;
	jackPortMap m_portMap;

} ;

#endif

#endif
