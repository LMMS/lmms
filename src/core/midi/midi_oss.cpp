#ifndef SINGLE_SOURCE_COMPILE

/*
 * midi_oss.cpp - OSS-raw-midi-client
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "midi_oss.h"

#ifdef LMMS_HAVE_OSS


#include <QtGui/QLabel>
#include <QtGui/QLineEdit>


#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif


#include "config_mgr.h"
#include "gui_templates.h"



midiOSS::midiOSS( void ) :
	m_midiDev( probeDevice() ),
	m_quit( FALSE )
{
	// only start thread, if opening of MIDI-device is successful,
	// otherwise isRunning()==FALSE indicates error
	if( m_midiDev.open( QIODevice::ReadWrite ) ||
					m_midiDev.open( QIODevice::ReadOnly ) )
	{
		start( QThread::LowPriority );
	}
}




midiOSS::~midiOSS()
{
	if( isRunning() )
	{
		m_quit = TRUE;
		wait( 1000 );
		terminate();
	}
}




QString midiOSS::probeDevice( void )
{
	QString dev = configManager::inst()->value( "midioss", "device" );
	if( dev == "" )
	{
		if( getenv( "MIDIDEV" ) != NULL )
		{
			return( getenv( "MIDIDEV" ) );
		}
		return( "/dev/midi" );
	}
	return( dev );
}




void midiOSS::sendByte( const Uint8 _c )
{
	m_midiDev.putChar( _c );
}




void midiOSS::run( void )
{
	while( m_quit == FALSE && m_midiDev.isOpen() )
	{
		char c;
		if( !m_midiDev.getChar( &c ) )
		{
			continue;
		}
		parseData( c );
	}
}





midiOSS::setupWidget::setupWidget( QWidget * _parent ) :
	midiClientRaw::setupWidget( midiOSS::name(), _parent )
{
	m_device = new QLineEdit( midiOSS::probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );
}




midiOSS::setupWidget::~setupWidget()
{
}




void midiOSS::setupWidget::saveSettings( void )
{
	configManager::inst()->setValue( "midioss", "device",
							m_device->text() );
}



#endif


#endif
