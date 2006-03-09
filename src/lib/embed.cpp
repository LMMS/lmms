#ifndef SINGLE_SOURCE_COMPILE

/*
 * embed.cpp - misc stuff for using embedded resources (linked into binary)
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QImage>

#else

#include <qimage.h>

#endif


#include "embed.h"
#include "config_mgr.h"



#ifndef PLUGIN_NAME
namespace embed
#else
namespace PLUGIN_NAME
#endif
{


#include "embedded_resources.h"


QPixmap getIconPixmap( const char * _name, int _w, int _h )
{
	if( _w == -1 || _h == -1 )
	{
		QString name = QString( _name ) + ".png";

#ifdef PLUGIN_NAME
		QPixmap p( configManager::inst()->artworkDir() + "plugins/" +
			STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ) + "_" + name );
		if( p.isNull() )
		{
			p = QPixmap( configManager::inst()->artworkDir() +
									name );
		}
#else
		// look whether icon is in artwork-dir
		QPixmap p( configManager::inst()->artworkDir() + name );
#endif
		if( p.isNull() )
		{
			// nothing found, so look in default-artwork-dir
			p =
		QPixmap( configManager::inst()->defaultArtworkDir() + name );
		}
		if( p.isNull() )
		{
#ifdef QT4
			const embed::descriptor & e = findEmbeddedData(
						name.toAscii().constData() );
#else
			const embed::descriptor & e = findEmbeddedData(
								name.ascii() );
#endif
			// found?
			if( QString( e.name ) == name )
			{
				p.loadFromData( e.data, e.size );
			}
			else
			{
				p = QPixmap( 1, 1 );
			}
		}
		return( p );
	}
#ifdef QT4
	return( getIconPixmap( _name ).scaled( _w, _h, Qt::IgnoreAspectRatio,
						Qt::SmoothTransformation ) );
#else
	return( getIconPixmap( _name ).convertToImage().smoothScale( _w, _h ) );
#endif
}




QString getText( const char * _name )
{
	const embed::descriptor & e = findEmbeddedData( _name );
	return( QString::fromLatin1( (const char *) e.data, e.size ) );
}


}


#endif
