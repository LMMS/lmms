/*
 * embed.h - misc. stuff for using embedded data (resources linked into binary)
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef EMBED_H
#define EMBED_H

#include <QPixmap>
#include <QtCore/QString>

#include "lmms_export.h"
#include "lmms_basics.h"


namespace embed
{

/**
 * Return an image for the icon pixmap cache.
 *
 * @param _name Identifier for the pixmap. If it is not in the icon pixmap
 *   cache, it will be loaded from the artwork QDir search paths (exceptions are
 *   compiled-in XPMs, you need to provide @p xpm for loading them).
 * @param xpm Must be XPM data if the source should be raw XPM data instead of
 *   a file
 */
QPixmap LMMS_EXPORT getIconPixmap( const QString&  _name,
	int _w = -1, int _h = -1 , const char** xpm = nullptr );
QString LMMS_EXPORT getText( const char * _name );

}


#ifdef PLUGIN_NAME
namespace PLUGIN_NAME
{

inline QPixmap getIconPixmap( const QString&  _name,
	int _w = -1, int _h = -1, const char** xpm = nullptr )
{
	return embed::getIconPixmap(QString("%1/%2").arg(STRINGIFY(PLUGIN_NAME), _name), _w, _h, xpm);
}
//QString getText( const char * _name );

}
#endif



class PixmapLoader
{
public:
	PixmapLoader( const PixmapLoader * _ref ) :
		m_name( _ref != NULL ? _ref->m_name : QString() ),
		m_xpm( _ref->m_xpm )
	{
	}

	PixmapLoader( const QString & _name = QString(),
		const char** xpm = nullptr ) :
		m_name( _name ),
		m_xpm(xpm)
	{
	}

	virtual QPixmap pixmap() const
	{
		if( !m_name.isEmpty() )
		{
			return( embed::getIconPixmap(
				m_name.toLatin1().constData(), -1, -1, m_xpm ));
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
	const char** m_xpm = nullptr;
} ;


#ifdef PLUGIN_NAME
class PluginPixmapLoader : public PixmapLoader
{
public:
	PluginPixmapLoader( const QString & _name = QString() ) :
		PixmapLoader( _name )
	{
	}

	virtual QPixmap pixmap() const
	{
		if( !m_name.isEmpty() )
		{
			return( PLUGIN_NAME::getIconPixmap(
					m_name.toLatin1().constData() ) );
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
