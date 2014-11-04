/*
 * embed.h - misc. stuff for using embedded data (resources linked into binary)
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _EMBED_H
#define _EMBED_H

#include <QtGui/QPixmap>
#include <QtCore/QString>

#include "export.h"
#include "lmms_basics.h"


namespace embed
{

struct descriptor
{
	int size;
	const unsigned char * data;
	const char * name;
} ;


QPixmap EXPORT getIconPixmap( const char *  _name, int _w = -1, int _h = -1 );
QString EXPORT getText( const char * _name );

}


#ifdef PLUGIN_NAME
namespace PLUGIN_NAME
{

QPixmap getIconPixmap( const char *  _name, int _w = -1, int _h = -1 );
//QString getText( const char * _name );

}
#endif



class PixmapLoader
{
public:
	PixmapLoader( const PixmapLoader * _ref ) :
		m_name( _ref != NULL ? _ref->m_name : QString::null )
	{
	}

	PixmapLoader( const QString & _name = QString::null ) :
		m_name( _name )
	{
	}

	virtual QPixmap pixmap() const
	{
		if( !m_name.isEmpty() )
		{
			return( embed::getIconPixmap(
					m_name.toAscii().constData() ) );
		}
		return( QPixmap() );
	}

	virtual ~PixmapLoader()
	{
	}

	virtual QString pixmapName() const
	{
		return m_name;
	}

protected:
	QString m_name;
} ;


#ifdef PLUGIN_NAME
class PluginPixmapLoader : public PixmapLoader
{
public:
	PluginPixmapLoader( const QString & _name = QString::null ) :
		PixmapLoader( _name )
	{
	}

	virtual QPixmap pixmap() const
	{
		if( !m_name.isEmpty() )
		{
			return( PLUGIN_NAME::getIconPixmap(
					m_name.toAscii().constData() ) );
		}
		return( QPixmap() );
	}

	virtual QString pixmapName() const
	{
		return QString( STRINGIFY(PLUGIN_NAME) ) + "::" + m_name;
	}

} ;
#endif



#endif
