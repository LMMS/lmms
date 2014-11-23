/*
 * MidiOss.cpp - OSS raw MIDI client
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


#include "MidiOss.h"

#ifdef LMMS_HAVE_OSS


#include <QtGui/QLabel>
#include <QtGui/QLineEdit>


#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif


#include "config_mgr.h"
#include "gui_templates.h"



MidiOss::MidiOss() :
	MidiClientRaw(),
	m_midiDev( probeDevice() ),
	m_quit( false )
{
	// only start thread, if opening of MIDI-device is successful,
	// otherwise isRunning()==false indicates error
	if( m_midiDev.open( QIODevice::ReadWrite ) ||
					m_midiDev.open( QIODevice::ReadOnly ) )
	{
		start( QThread::LowPriority );
	}
}




MidiOss::~MidiOss()
{
	if( isRunning() )
	{
		m_quit = true;
		wait( 1000 );
		terminate();
	}
}




QString MidiOss::probeDevice()
{
	QString dev = configManager::inst()->value( "midioss", "device" );
	if( dev.isEmpty() )
	{
		if( getenv( "MIDIDEV" ) != NULL )
		{
			return getenv( "MIDIDEV" );
		}
		return "/dev/midi";
	}
	return dev;
}




void MidiOss::sendByte( const unsigned char c )
{
	m_midiDev.putChar( c );
}




void MidiOss::run()
{
	while( m_quit == false && m_midiDev.isOpen() )
	{
		char c;
		if( !m_midiDev.getChar( &c ) )
		{
			continue;
		}
		parseData( c );
	}
}





MidiOss::setupWidget::setupWidget( QWidget * _parent ) :
	MidiClientRaw::setupWidget( MidiOss::name(), _parent )
{
	m_device = new QLineEdit( MidiOss::probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<7>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );
}




MidiOss::setupWidget::~setupWidget()
{
}




void MidiOss::setupWidget::saveSettings()
{
	configManager::inst()->setValue( "midioss", "device",
							m_device->text() );
}



#endif


