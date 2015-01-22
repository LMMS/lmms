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

#ifndef EMBED_H
#define EMBED_H

#include <QPixmap>
#include <QtCore/QString>

#include "export.h"
#include "lmms_basics.h"


namespace embed
{

QString EXPORT getText( const char * _name );

}


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
			return QPixmap(QString("icons:%1.png").arg(m_name));
		}
		return QPixmap();
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
			QString pixmap_path = QString(":/%1/%2.png").arg(STRINGIFY(PLUGIN_NAME), m_name);
			return QPixmap(pixmap_path);
		}
		return QPixmap();
	}

	virtual QString pixmapName() const
	{
		return QString( STRINGIFY(PLUGIN_NAME) ) + "::" + m_name;
	}

} ;
#endif

#endif
