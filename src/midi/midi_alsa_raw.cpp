/*
 * midi_alsa_raw.cpp - midi-client for RawMIDI via ALSA
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


#include "qt3support.h"

#ifdef QT4

#include <QLineEdit>
#include <QLabel>

#else

#include <qpair.h>
#include <qlineedit.h>
#include <qlabel.h>

#endif


#include "midi_alsa_raw.h"
#include "config_mgr.h"
#include "gui_templates.h"


#ifdef ALSA_SUPPORT


midiALSARaw::midiALSARaw( void ) :
	midiRawClient(),
	QThread(),
	m_inputp( &m_input ),
	m_outputp( &m_output ),
	m_quit( FALSE )
{
	int err;
	if( ( err = snd_rawmidi_open( m_inputp, m_outputp,
#ifdef QT4
					probeDevice().toAscii().constData(),
#else
					probeDevice().ascii(),
#endif
								0 ) ) < 0 )
	{
		printf( "cannot open MIDI-device: %s\n", snd_strerror( err ) );
		return;
	}

	snd_rawmidi_read( m_input, NULL, 0 );

	snd_rawmidi_nonblock( m_input, 1 );
	m_npfds = snd_rawmidi_poll_descriptors_count( m_input );
	m_pfds = new pollfd[m_npfds];
	snd_rawmidi_poll_descriptors( m_input, m_pfds, m_npfds );

	start( 
#if QT_VERSION >= 0x030200	
	    	QThread::LowPriority 
#endif
					);
}




midiALSARaw::~midiALSARaw()
{
	if( running() )
	{
		m_quit = TRUE;
		wait( 500 );
		terminate();

		snd_rawmidi_close( m_input );
		snd_rawmidi_close( m_output );
		delete[] m_pfds;
	}
}




QString midiALSARaw::probeDevice( void )
{
	QString dev = configManager::inst()->value( "midialsa", "device" );
	if( dev == "" )
	{
		if( getenv( "MIDIDEV" ) != NULL )
		{
			return( getenv( "MIDIDEV" ) );
		}
		return( "default" );
	}
	return( dev );
}




void midiALSARaw::sendByte( Uint8 _c )
{
	snd_rawmidi_write( m_output, &_c, sizeof( _c ) );
}




void midiALSARaw::run( void )
{
	Uint8 buf[128];
	//int cnt = 0;
	while( m_quit == FALSE )
	{
		msleep( 5 );	// must do that, otherwise this thread takes
				// too much CPU-time, even with LowPriority...
		int err = poll( m_pfds, m_npfds, 10000 );
		if( err < 0 && errno == EINTR )
		{
			printf( "midiALSARaw::run(): Got EINTR while "
				"polling. Will stop polling MIDI-events from "
				"MIDI-port.\n" );
			break;
		}
		if( err < 0 )
		{
			printf( "poll failed: %s\nWill stop polling "
				"MIDI-events from MIDI-port.\n",
							strerror( errno ) );
			break;
		}
		if( err == 0 )
		{
			//printf( "there seems to be no active MIDI-device %d\n", ++cnt );
			continue;
		}
		unsigned short revents;
		if( ( err = snd_rawmidi_poll_descriptors_revents(
				m_input, m_pfds, m_npfds, &revents ) ) < 0 )
		{
			printf( "cannot get poll events: %s\nWill stop polling "
				"MIDI-events from MIDI-port.\n",
							snd_strerror( errno ) );
			break;
		}
		if( revents & ( POLLERR | POLLHUP ) )
		{
			printf( "POLLERR or POLLHUP\n" );
			break;
		}
		if( !( revents & POLLIN ) )
		{
			continue;
		}
		err = snd_rawmidi_read( m_input, buf, sizeof( buf ) );
		if( err == -EAGAIN )
		{
			continue;
		}
		if( err < 0 )
		{
			printf( "cannot read from port \"%s\": %s\nWill stop "
				"polling MIDI-events from MIDI-port.\n",
				/*port_name*/"default", snd_strerror( err ) );
			break;
		}
		if( err == 0 )
		{
			continue;
		}
		for( int i = 0; i < err; ++i )
		{
			parseData( buf[i] );
		}
	}

}




midiALSARaw::setupWidget::setupWidget( QWidget * _parent ) :
	midiRawClient::setupWidget( midiALSARaw::name(), _parent )
{
	m_device = new QLineEdit( midiALSARaw::probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );
}




midiALSARaw::setupWidget::~setupWidget()
{
}




void midiALSARaw::setupWidget::saveSettings( void )
{
	configManager::inst()->setValue( "midialsa", "device",
							m_device->text() );
}



#endif

