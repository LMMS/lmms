/*
 * midi_mapper.cpp - MIDI-mapper for any midiDevice
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



#include "midi_mapper.h"

#ifdef QT4

#include <QRegExp>

#else

#include <qregexp.h>

#define indexOf find

#endif


midiMapper::midiMapper( const QString & _map ) :
	m_drumsetChannel( 0 ),
	m_drumsetPatch( 0 )
{
	// default mappings
	for( Uint8 i = 0; i < MIDI_PROGRAMS; ++i )
	{
		m_patchMap[i].first = i;
	}
	for( Uint8 i = 0; i < MIDI_KEYS; ++i )
	{
		m_drumsetKeyMap[i].first = i;
	}
	for( Uint8 i = 0; i < MIDI_CHANNELS; ++i )
	{
		m_channelMap[i] = i;
	}
	QFile map( _map );
#ifdef QT4
	if( !map.open( QIODevice::ReadOnly ) )
#else
	if( !map.open( IO_ReadOnly ) )
#endif
	{
		return;
	}
	while( !map.atEnd() )
	{
		char buf[1024];
		int len = map.readLine( buf, sizeof( buf ) );
		if( len <= 0 )
		{
			continue;
		}
		QString line( buf );
#if QT_VERSION >= 0x030100
		line.replace( '\n', "" );
#else
		if( line.contains( '\n' ) )
		{
			line = line.left( line.length() - 1 );
		}
#endif
		if( line.left( 6 ) == "DEFINE" )
		{
			if( line.section( ' ', 1, 1 ) == "PATCHMAP" )
			{
				readPatchMap( map );
			}
			else if( line.section( ' ', 1, 1 ) == "KEYMAP" &&
					line.section( ' ', 2, 2 ) ==
								"\"Drumset\"" )
			{
				readDrumsetKeyMap( map );
			}
			else if( line.section( ' ', 1, 1 ) == "CHANNELMAP" )
			{
				readChannelMap( map );
			}
		}
	}
}




midiMapper::~midiMapper()
{
}




void midiMapper::readPatchMap( QFile & _f )
{
	Uint8 prog_idx = 0;

	while( !_f.atEnd() && prog_idx < MIDI_PROGRAMS )
	{
		char buf[1024];
		int len = _f.readLine( buf, sizeof( buf ) );
		if( len <= 0 )
		{
			continue;
		}
		QString line( buf );
#if QT_VERSION >= 0x030100
		line.replace( '\n', "" );
#else
		if( line.contains( '\n' ) )
		{
			line = line.left( line.length() - 1 );
		}
#endif
		if( line.left( 3 ) == "END" )
		{
			return;
		}
		if( line[0] == '#' )
		{
			continue;
		}
		m_patchMap[prog_idx].first = line.section( '=', 1, 1 ).toInt();
		m_patchMap[prog_idx].second = line.section( '=', 0, 0 )
#if QT_VERSION >= 0x030100
							.replace( ' ', "" )
#endif
						;
		++prog_idx;
	}
}




void midiMapper::readDrumsetKeyMap( QFile & _f )
{
	Uint8 key = 0;
	while( !_f.atEnd() )
	{
		char buf[1024];
		int len = _f.readLine( buf, sizeof( buf ) );
		if( len <= 0 )
		{
			continue;
		}
		QString line( buf );
#if QT_VERSION >= 0x030100
		line.replace( '\n', "" );
#else
		if( line.contains( '\n' ) )
		{
			line = line.left( line.length() - 1 );
		}
#endif
		if( line.left( 3 ) == "END" )
		{
			return;
		}
		if( line[0] == '#' )
		{
			continue;
		}
		if( line[4] != '=' )
		{
			m_drumsetKeyMap[key].first = line.section( '=', 1, 1 ).
									toInt();

			m_drumsetKeyMap[key].second = line.mid( 4 ).
							section( '=', 0, 0 ).
							section( ' ', 1, 1 )
#if QT_VERSION >= 0x030100
							.replace( ' ', "" )
#endif
							;
		}
		++key;
	}
}




void midiMapper::readChannelMap( QFile & _f )
{
	while( !_f.atEnd() )
	{
		char buf[1024];
		int len = _f.readLine( buf, sizeof( buf ) );
		if( len <= 0 )
		{
			continue;
		}
		QString line( buf );
#if QT_VERSION >= 0x030100
		line.replace( '\n', "" );
#else
		if( line.contains( '\n' ) )
		{
			line = line.left( line.length() - 1 );
		}
#endif
		if( line.left( 3 ) == "END" )
		{
			return;
		}
		if( line[0] == '#' )
		{
			continue;
		}
		Uint8 ch = line.section( ' ', 0, 0 ).toInt();
		Uint8 mch = line.section( '=', 1, 1 ).mid( 1 ).
					section( ' ', 0, 0 ).
					toInt();
		if( ch < MIDI_CHANNELS && mch < MIDI_CHANNELS )
		{
			m_channelMap[ch] = mch;
			if( line.contains( QRegExp( "Keymap *\"Drumset\"" ) ) )
			{
				m_drumsetChannel = mch;
				int fp = line.indexOf( "ForcePatch" );
				if( fp != -1 )
				{
					m_drumsetPatch = line.mid( fp ).
							section( ' ', 1, 1 ).
								toInt();
				}
			}
		}
	}
}



#undef indexOf

