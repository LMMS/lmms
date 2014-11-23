/*
 * ImportFilter.h - declaration of class ImportFilter, the base-class for all
 *                  file import filters
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

#ifndef _IMPORT_FILTER_H
#define _IMPORT_FILTER_H

#include <QtCore/QFile>

#include "Plugin.h"


class TrackContainer;


class EXPORT ImportFilter : public Plugin
{
public:
	ImportFilter( const QString & _file_name,
					const Descriptor * _descriptor );
	virtual ~ImportFilter();


	// tries to import given file to given track-container by having all
	// available import-filters to try to import the file
	static void import( const QString & _file_to_import,
						TrackContainer* tc );


protected:
	virtual bool tryImport( TrackContainer* tc ) = 0;

	const QFile & file() const
	{
		return m_file;
	}

	bool openFile();

	inline void closeFile()
	{
		m_file.close();
	}

	inline int readByte()
	{
		unsigned char c;
		if( m_file.getChar( (char*) &c ) )
		{
			return static_cast<int>( c );
		}
		return -1;
	}

	inline int readBlock( char * _data, int _len )
	{
		return m_file.read( _data, _len );
	}

	inline void ungetChar( char _ch )
	{
		m_file.ungetChar( _ch );
	}

	virtual void saveSettings( QDomDocument &, QDomElement & )
	{
	}

	virtual void loadSettings( const QDomElement & )
	{
	}

	virtual QString nodeName() const
	{
		return "import_filter";
	}


private:
	QFile m_file;

} ;


#endif
