/*
 * embed.cpp - misc stuff for using embedded resources (linked into binary)
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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

#include <QApplication>
#include <QTranslator>
#include <QImage>

#else

#include <qapplication.h>
#include <qtranslator.h>
#include <qimage.h>

#endif


#include "embed.h"
#include "config_mgr.h"


namespace embed
{

#include "embedded_resources.h"


QPixmap getIconPixmap( const char * _name, int _w, int _h )
{
	if( _w == -1 || _h == -1 )
	{
		QString name = QString( _name ) + ".png";
#ifdef QT4
		const embedDesc & e = findEmbeddedData(
						name.toAscii().constData() );
#else
		const embedDesc & e = findEmbeddedData( name.ascii() );
#endif
		// not found?
		if( QString( e.name ) != name )
		{
			// then look whether icon is in data-dir
			QPixmap p( configManager::inst()->artworkDir() + name );
			if( p.isNull() )
			{
				p = QPixmap( 1, 1 );
			}
			return( p );
		}
		QPixmap p;
		p.loadFromData( e.data, e.size );
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
	const embedDesc & e = findEmbeddedData( _name );
	return( QString::fromLatin1( (const char *) e.data, e.size ) );
}




void loadTranslation( const QString & _tname )
{
	QTranslator * t = new QTranslator( 0 );
	QString name = _tname + ".qm";

#if QT_VERSION >= 0x030100

#ifdef QT4
	const embedDesc & e = findEmbeddedData( name.toAscii().constData() );
#else
	const embedDesc & e = findEmbeddedData( name.ascii() );
#endif
	// not found?
	if( QString( e.name ) != name )
	{
#endif
		// then look whether translation is in data-dir
		t->load( name, configManager::inst()->localeDir() );
#if QT_VERSION >= 0x030100
	}
	else
	{
		t->load( e.data, (int) e.size );
	}
#endif

	qApp->installTranslator( t );
}


}
