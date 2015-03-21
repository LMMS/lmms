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

#include <QDebug>
#include <QImage>
#include <QHash>
#include <QImageReader>
#include <QList>
#include <QResource>
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


QPixmap getIconPixmap( const char * pixmapName, int width, int height )
{
	// Return cached pixmap
	QPixmap cached = s_pixmapCache.value( pixmapName );
	if( !cached.isNull() )
	{
		return cached;
	}

#ifdef PLUGIN_NAME
	QString name(QString("artwork:%1/%2").arg(STRINGIFY(PLUGIN_NAME), pixmapName));
#else
	QString name(QString("artwork:%1").arg(pixmapName));
#endif
	QImageReader reader(name);

	if (width > 0 && height > 0)
	{
		reader.setScaledSize(QSize(width, height));
	}

	QImage image = reader.read();
	if (image.isNull())
	{
		qWarning().nospace() << "Error loading icon pixmap " << name << ": " <<
								reader.errorString().toLocal8Bit().data();
		return QPixmap(1,1);
	}

	QPixmap pixmap = QPixmap::fromImage(image);

	// Save to cache and return
	s_pixmapCache.insert( pixmapName, pixmap );
	return pixmap;
}


QString getText( const char * name )
{
	return QString::fromUtf8( (const char*) QResource(QString(":/%1").arg(name)).data());
}


}


