/*
 * FlpImport.h - support for importing FLP-files
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _FLP_IMPORT_H
#define _FLP_IMPORT_H

#include <QtCore/QString>
#include <QtCore/QPair>
#include <QtCore/QVector>

#include "ImportFilter.h"
#include "note.h"



class instrument;
struct FL_Channel;

class FlpImport : public ImportFilter
{
public:
	FlpImport( const QString & _file );
	virtual ~FlpImport();

	virtual PluginView * instantiateView( QWidget * )
	{
		return NULL;
	}


private:
	virtual bool tryImport( TrackContainer* tc );

	void processPluginParams( FL_Channel * _ch );

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

	inline int32_t read32LE()
	{
		int value = readByte();
		value |= readByte() << 8;
		value |= readByte() << 16;
		value |= readByte() << 24;
		return( value );
	}
	inline int32_t read16LE()
	{
		int value = readByte();
		value |= readByte() << 8;
		return( value );
	}

	inline int32_t readID()
	{
		return( read32LE() );
	}

	inline void skip( int _bytes )
	{
		while( _bytes > 0 )
		{
			readByte();
			--_bytes;
		}
	}

} ;


#endif
