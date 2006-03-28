/*
 * import_filter.h - declaration of class importFilter, the base-class for all
 *                   file import filters
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _IMPORT_FILTER_H
#define _IMPORT_FILTER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <QFile>

#else

#include <qfile.h>

#endif


#include "plugin.h"


class trackContainer;


class importFilter : public plugin
{
public:
	importFilter( const QString & _file_name,
			const descriptor * _descriptor, engine * _eng );
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
#ifdef QT4
		char c;
		if( m_file.getChar( &c ) )
		{
			return( static_cast<int>( c ) );
		}
		return( -1 );
#else
		return( m_file.getch() );
#endif
	}

	inline int readBlock( char * _data, int _len )
	{
		return( m_file.readBlock( _data, _len ) );
	}

	inline void ungetChar( int _ch )
	{
#ifndef QT3
		m_file.ungetChar( _ch );
#else
		m_file.ungetch( _ch );
#endif
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
