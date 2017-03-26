/*
 * MidiSndio.cpp - base-class that implements sndio midi support
 *
 * Copyright (c) 2010-2016 jackmsr@openbsd.net
 * Copyright (c) 2016-2017 David Carlier <devnexen@gmail.com>
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

#include "MidiSndio.h"

#ifdef LMMS_HAVE_SNDIO

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <poll.h>

#include "ConfigManager.h"
#include "gui_templates.h"


MidiSndio::MidiSndio( void ) :
	MidiClientRaw(),
	m_quit( FALSE )
{
	QString dev = probeDevice();

	if (dev == "")
	{
		m_hdl = mio_open( NULL, MIO_IN | MIO_OUT, 0 );
	}
	else
	{
		m_hdl = mio_open( dev.toAscii().data(), MIO_IN | MIO_OUT, 0 );
	}

	if( m_hdl == NULL )
	{
		printf( "sndio: failed opening sndio midi device\n" );
		return;
	}

	start( QThread::LowPriority );
}


MidiSndio::~MidiSndio()
{
	if( isRunning() )
	{
		m_quit = TRUE;
		wait( 1000 );
		terminate();
	}
}


QString MidiSndio::probeDevice( void )
{
	QString dev = ConfigManager::inst()->value( "MidiSndio", "device" );

	return dev ;
}


void MidiSndio::sendByte( const unsigned char c )
{
	mio_write( m_hdl, &c, 1 );
}


void MidiSndio::run( void )
{
	struct pollfd pfd;
	nfds_t nfds;
	char buf[0x100], *p;
	size_t n;
	int ret;
	while( m_quit == FALSE && m_hdl )
	{
		nfds = mio_pollfd( m_hdl, &pfd, POLLIN );
		ret = poll( &pfd, nfds, 100 );
		if ( ret < 0 )
			break;
		if ( !ret || !( mio_revents( m_hdl, &pfd ) & POLLIN ) )
			continue;
		n = mio_read( m_hdl, buf, sizeof(buf) );
		if ( !n )
		{
			break;
		}
		for (p = buf; n > 0; n--, p++)
		{
			parseData( *p );
		}
	}
}

#endif	/* LMMS_HAVE_SNDIO */
