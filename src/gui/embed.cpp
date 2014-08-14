/*
 * embed.cpp - misc stuff for using embedded resources (linked into binary)
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QImage>
#include <QHash>
#include <QImageReader>
#include <QList>
#include "embed.h"
#include "config_mgr.h"

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


QPixmap getIconPixmap( const char * _name, int _w, int _h )
{
	if( _w == -1 || _h == -1 )
	{
	        // Return cached pixmap
                QPixmap cached = s_pixmapCache.value( _name );
		if( !cached.isNull() )
		{
			return cached;
		}

		// Or try to load it
		QList<QByteArray> formats = 
			QImageReader::supportedImageFormats();
		QList<QString> candidates;
		QPixmap p;
		QString name;
		int i;
		
		for ( i = 0; i < formats.size() && p.isNull(); ++i )  
		{
			candidates << QString( _name ) + "." + formats.at( i ).data();
		}

#ifdef PLUGIN_NAME
		for ( i = 0; i < candidates.size() && p.isNull(); ++i )  {
			name = candidates.at( i );
			p = QPixmap( configManager::inst()->artworkDir() + "plugins/" +
				     STRINGIFY( PLUGIN_NAME ) + "_" + name );
		}
#endif
		for ( i = 0; i < candidates.size() && p.isNull(); ++i )  {
			name = candidates.at( i );
			p = QPixmap( configManager::inst()->artworkDir() + name );
		}
		
		// nothing found, so look in default-artwork-dir
		for ( i = 0; i < candidates.size() && p.isNull(); ++i )  {
			name = candidates.at( i );
			p = QPixmap( configManager::inst()->defaultArtworkDir() 
				     + name );
		}
		
		for ( i = 0; i < candidates.size() && p.isNull(); ++i )  {
			name = candidates.at( i );
			const embed::descriptor & e = 
				findEmbeddedData( name.toUtf8().constData() );
			// found?
			if( QString( e.name ) == name )
			{
				p.loadFromData( e.data, e.size );
			}
		}
		
		// Fallback
		if(p.isNull()) 
		{
			p = QPixmap( 1, 1 );
		}
		// Save to cache and return
		s_pixmapCache.insert( _name, p );
		return p;

	}

	return getIconPixmap( _name ).
		scaled( _w, _h, Qt::IgnoreAspectRatio, 
			Qt::SmoothTransformation );
}


QString getText( const char * _name )
{
	const embed::descriptor & e = findEmbeddedData( _name );
	return QString::fromUtf8( (const char *) e.data, e.size );
}


}


