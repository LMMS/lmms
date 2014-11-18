/*
 * MidiImport.h - support for importing MIDI-files
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

#ifndef _MIDI_IMPORT_H
#define _MIDI_IMPORT_H

#include <QtCore/QString>
#include <QtCore/QPair>
#include <QtCore/QVector>

#include "MidiEvent.h"
#include "ImportFilter.h"


class MidiImport : public ImportFilter
{
	Q_OBJECT
public:
	MidiImport( const QString & _file );
	virtual ~MidiImport();

	virtual PluginView * instantiateView( QWidget * )
	{
		return( NULL );
	}


private:
	virtual bool tryImport( TrackContainer* tc );

	bool readSMF( TrackContainer* tc );
	bool readRIFF( TrackContainer* tc );
	bool readTrack( int _track_end, QString & _track_name );

	void error( void );


	inline int readInt( int _bytes )
	{
		int c, value = 0;
		do
		{
			c = readByte();
			if( c == -1 )
			{
				return( -1 );
			}
			value = ( value << 8 ) | c;
		} while( --_bytes );
		return( value );
	}
	inline int read32LE()
	{
		int value = readByte();
		value |= readByte() << 8;
		value |= readByte() << 16;
		value |= readByte() << 24;
		return value;
	}
	inline int readVar()
	{
		int c = readByte();
		int value = c & 0x7f;
		if( c & 0x80 )
		{
			c = readByte();
			value = ( value << 7 ) | ( c & 0x7f );
			if( c & 0x80 )
			{
				c = readByte();
				value = ( value << 7 ) | ( c & 0x7f );
				if( c & 0x80 )
				{
					c = readByte();
					value = ( value << 7 ) | c;
					if( c & 0x80 )
					{
						return -1;
					}
				}
			}
	        }
        	return( !file().atEnd() ? value : -1 );
	}

	inline int readID()
	{
		return read32LE();
	}
	inline void skip( int _bytes )
	{
		while( _bytes > 0 )
		{
			readByte();
			--_bytes;
		}
	}


	typedef QVector<QPair<int, MidiEvent> > EventVector;
	EventVector m_events;
	int m_timingDivision;

} ;


#endif
