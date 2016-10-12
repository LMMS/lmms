/*
 * embed.cpp - misc stuff for using embedded resources (linked into binary)
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


#include <QImage>
#include <QHash>
#include <QImageReader>
#include <QList>
#include "embed.h"
#include "ConfigManager.h"

#ifndef PLUGIN_NAME
namespace embed
#else
namespace PLUGIN_NAME
#endif
{

namespace
{
static QHash<QString, QPixmap> s_pixmapCache;
}

#include "embedded_resources.h"


QPixmap getIconPixmap( const char * pixmapName, int width, int height )
{
	if( width == -1 || height == -1 )
	{
		// Return cached pixmap
		QPixmap cached = s_pixmapCache.value( pixmapName );

		if( !cached.isNull() )
		{
			return cached;
		}

		// Or try to load it
		QList<QByteArray> formats = QImageReader::supportedImageFormats();
		QList<QString> candidates;
		QPixmap pixmap;
		QString name;
		int i;

		for ( i = 0; i < formats.size() && pixmap.isNull(); ++i )
		{
			candidates << QString( pixmapName ) + "." + formats.at( i ).data();
		}

#ifdef PLUGIN_NAME

		for ( i = 0; i < candidates.size() && pixmap.isNull(); ++i )
		{
			name = candidates.at( i );
			pixmap = QPixmap( "resources:plugins/" STRINGIFY( PLUGIN_NAME ) "_" + name );
		}

#endif

		for ( i = 0; i < candidates.size() && pixmap.isNull(); ++i )
		{
			name = candidates.at( i );
			pixmap = QPixmap( "resources:" + name );
		}

		for ( i = 0; i < candidates.size() && pixmap.isNull(); ++i )
		{
			name = candidates.at( i );
			const embed::descriptor & e =
				findEmbeddedData( name.toUtf8().constData() );

			// found?
			if( name == e.name )
			{
				pixmap.loadFromData( e.data, e.size );
			}
		}

		// Fallback
		if( pixmap.isNull() )
		{
			pixmap = QPixmap( 1, 1 );
		}

		// Save to cache and return
		s_pixmapCache.insert( pixmapName, pixmap );
		return pixmap;
	}

	return getIconPixmap( pixmapName ).
	       scaled( width, height, Qt::IgnoreAspectRatio,
		       Qt::SmoothTransformation );
}


QString getText( const char * _name )
{
	const embed::descriptor & e = findEmbeddedData( _name );
	return QString::fromUtf8( ( const char * ) e.data, e.size );
}


}


