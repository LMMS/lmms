/*
 * import_filter.h - declaration of class importFilter, the base-class for all
 *                   file import filters
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _IMPORT_FILTER_H
#define _IMPORT_FILTER_H

#include <QtCore/QFile>


#include "plugin.h"


class trackContainer;


class EXPORT importFilter : public plugin
{
public:
	importFilter( const QString & _file_name,
					const descriptor * _descriptor );
	virtual ~importFilter();


	// tries to import given file to given track-container by having all
	// available import-filters to try to import the file
	static void FASTCALL import( const QString & _file_to_import,
						trackContainer * _tc );


protected:
	virtual bool tryImport( trackContainer * _tc ) = 0;

	const QFile & file( void ) const
	{
		return( m_file );
	}

	bool openFile( void );

	inline void closeFile( void )
	{
		m_file.close();
	}

	inline int readByte( void )
	{
		unsigned char c;
		if( m_file.getChar( (char*) &c ) )
		{
			return( static_cast<int>( c ) );
		}
		return( -1 );
	}

	inline int readBlock( char * _data, int _len )
	{
		return( m_file.read( _data, _len ) );
	}

	inline void ungetChar( int _ch )
	{
		m_file.ungetChar( _ch );
	}

	virtual void FASTCALL saveSettings( QDomDocument &,
						QDomElement & )
	{
	}

	virtual void FASTCALL loadSettings( const QDomElement & )
	{
	}

	virtual QString nodeName( void ) const
	{
		return( "import_filter" );
	}


private:
	QFile m_file;

} ;


#endif
