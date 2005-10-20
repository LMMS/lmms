/*
 * audio_jack.h - support for JACK-transport
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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

#include <QMutex>
#include <QVector>
#include <QList>

#else

#include <qmutex.h>
#include <qvaluevector.h>
#include <qvaluelist.h>

#endif


#include "audio_device.h"
#include "tab_widget.h"


class QLineEdit;
class lcdSpinBox;


class audioJACK : public audioDevice
{
public:
	audioJACK( Uint32 _sample_rate, bool & _success_ful );
	~audioJACK();

	inline static QString name( void )
	{
		return( setupWidget::tr( "JACK (Jack Audio Connection Kit)" ) );
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
	virtual void FASTCALL writeBufferToDev( surroundSampleFrame * _ab,
						Uint32 _frames,
						float _master_gain );

	void clearBuffer( void );

	static int processCallback( jack_nframes_t _nframes, void * _udata );
	static int bufSizeCallback( jack_nframes_t _nframes, void * _udata );


	jack_client_t * m_client;
	vvector<jack_port_t *> m_outputPorts;
	struct bufset
	{
		sampleType * buf;
		Uint32 frames;
	} ;

	vlist<vvector<bufset> > m_bufferSets;
	Uint32 m_framesDoneInCurBuf;
	volatile Uint32 m_frameSync;

	Uint32 m_jackBufSize;

	QMutex m_bufMutex;

} ;

#endif

#endif
