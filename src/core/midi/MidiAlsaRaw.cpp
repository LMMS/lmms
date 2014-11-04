/*
 * MidiAlsaRaw.cpp - MIDI client for RawMIDI via ALSA
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include "MidiAlsaRaw.h"
#include "config_mgr.h"
#include "gui_templates.h"


#ifdef LMMS_HAVE_ALSA


MidiAlsaRaw::MidiAlsaRaw() :
	MidiClientRaw(),
	m_inputp( &m_input ),
	m_outputp( &m_output ),
	m_quit( false )
{
	int err;
	if( ( err = snd_rawmidi_open( m_inputp, m_outputp,
					probeDevice().toAscii().constData(),
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

	start( QThread::LowPriority );
}




MidiAlsaRaw::~MidiAlsaRaw()
{
	if( isRunning() )
	{
		m_quit = true;
		wait( 1000 );
		terminate();

		snd_rawmidi_close( m_input );
		snd_rawmidi_close( m_output );
		delete[] m_pfds;
	}
}




QString MidiAlsaRaw::probeDevice()
{
	QString dev = configManager::inst()->value( "MidiAlsaRaw", "Device" );
	if( dev == "" )
	{
		if( getenv( "MIDIDEV" ) != NULL )
		{
			return getenv( "MIDIDEV" );
		}
		return "default";
	}
	return dev;
}




void MidiAlsaRaw::sendByte( unsigned char c )
{
	snd_rawmidi_write( m_output, &c, sizeof( c ) );
}




void MidiAlsaRaw::run()
{
	unsigned char buf[128];
	//int cnt = 0;
	while( m_quit == false )
	{
		msleep( 5 );	// must do that, otherwise this thread takes
				// too much CPU-time, even with LowPriority...
		int err = poll( m_pfds, m_npfds, 10000 );
		if( err < 0 && errno == EINTR )
		{
			printf( "MidiAlsaRaw::run(): Got EINTR while "
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




MidiAlsaRaw::setupWidget::setupWidget( QWidget * _parent ) :
	MidiClientRaw::setupWidget( MidiAlsaRaw::name(), _parent )
{
	m_device = new QLineEdit( MidiAlsaRaw::probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<7>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );
}




MidiAlsaRaw::setupWidget::~setupWidget()
{
}




void MidiAlsaRaw::setupWidget::saveSettings()
{
	configManager::inst()->setValue( "MidiAlsaRaw", "Device",
							m_device->text() );
}


#endif

