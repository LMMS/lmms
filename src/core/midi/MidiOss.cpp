/*
 * MidiOss.cpp - OSS raw MIDI client
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "MidiOss.h"

#ifdef LMMS_HAVE_OSS

#include "ConfigManager.h"


namespace lmms
{


MidiOss::MidiOss() :
	MidiClientRaw(),
	m_midiDev( probeDevice() ),
	m_quit( false )
{
	// only start thread, if opening of MIDI-device is successful,
	// otherwise isRunning()==false indicates error
	if( m_midiDev.open( QIODevice::ReadWrite |
		QIODevice::Unbuffered ) ||
		m_midiDev.open( QIODevice::ReadOnly |
			QIODevice::Unbuffered ) )
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
	QString dev = ConfigManager::inst()->value( "midioss", "device" );
	if( dev.isEmpty() )
	{
		if( getenv( "MIDIDEV" ) != nullptr )
		{
			return getenv( "MIDIDEV" );
		}
#ifdef __NetBSD__
		return "/dev/rmidi0";
#else
		return "/dev/midi";
#endif
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


} // namespace lmms

#endif // LMMS_HAVE_OSS
