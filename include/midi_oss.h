/*
 * midi_oss.h - OSS-driver for MIDI-port
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


#ifndef _MIDI_OSS_H
#define _MIDI_OSS_H

#include "audio_oss.h"

#ifdef OSS_SUPPORT


#include <qthread.h>
#include <qfile.h>

#include "midi_device.h"


class QLineEdit;


class midiOSS : public midiDevice, public QThread
{
public:
	midiOSS( channelTrack * _ct = NULL );
	~midiOSS();

	static QString probeDevice( void );


	inline static QString name( void )
	{
		return( setupWidget::tr( "OSS Raw-MIDI (Open Sound System)" ) );
	}


	class setupWidget : public midiDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings( void );

	private:
		QLineEdit * m_device;

	} ;


protected:
	virtual void FASTCALL sendByte( Uint8 _c );
	virtual void FASTCALL run( void );


private:
	QFile m_midiDev;

	volatile bool m_quit;

} ;

#endif


#endif
