/*
 * midi_alsa_raw.h - midi-client for RawMIDI via ALSA
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


#ifndef _MIDI_ALSA_RAW_H
#define _MIDI_ALSA_RAW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ALSA_ASOUNDLIB_H

#ifndef ALSA_SUPPORT
#define ALSA_SUPPORT
#endif

#include <alsa/asoundlib.h>

#include "qt3support.h"

#ifdef QT4

#include <QThread>

#else

#include <qthread.h>

#endif


#include "midi_client.h"


struct pollfd;
class QLineEdit;


class midiALSARaw : public midiRawClient, public QThread
{
public:
	midiALSARaw( void );
	~midiALSARaw();

	static QString probeDevice( void );


	inline static QString name( void )
	{
		return( setupWidget::tr( "ALSA Raw-MIDI (Advanced Linux Sound "
							"Architecture)" ) );
	}


	class setupWidget : public midiRawClient::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings( void );

	private:
		QLineEdit * m_device;

	} ;


protected:
	virtual void FASTCALL sendByte( const Uint8 _c );
	virtual void run( void );


private:
	snd_rawmidi_t * m_input, * * m_inputp;
	snd_rawmidi_t * m_output, * * m_outputp;
	int m_npfds;
	pollfd * m_pfds;

	volatile bool m_quit;

} ;

#endif

#endif
