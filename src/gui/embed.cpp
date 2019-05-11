/*
 * embed.cpp - misc stuff for using embedded resources (linked into binary)
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

#include <QDebug>
#include <QImageReader>
#include <QPixmapCache>
#include <QResource>
#include "embed.h"

namespace embed
{

QPixmap getIconPixmap(const QString& pixmapName,
	int width, int height, const char** xpm )
{
	QString cacheName;
	if (width > 0 && height > 0)
	{
		cacheName = QString("%1_%2_%3").arg(pixmapName, width, height);
	}
	else
	{
		cacheName = pixmapName;
	}

	// Return cached pixmap
	QPixmap pixmap;
	if( QPixmapCache::find(cacheName, &pixmap) )
	{
		return pixmap;
	}

	if(xpm)
	{
		pixmap = QPixmap(xpm);
	}
	else
	{
		QImageReader reader(QString("artwork:%1").arg(pixmapName));

		if (width > 0 && height > 0)
		{
			reader.setScaledSize(QSize(width, height));
		}

		pixmap = QPixmap::fromImageReader(&reader);

		if (pixmap.isNull())
		{
			qWarning().nospace() << "Error loading icon pixmap " << pixmapName << ": " <<
									reader.errorString().toLocal8Bit().data();
			return QPixmap(1,1);
		}
	}

	// Save to cache and return
	QPixmapCache::insert(cacheName, pixmap);
	return pixmap;
}


QString getText( const char * name )
{
	return QString::fromUtf8( (const char*) QResource(QString(":/%1").arg(name)).data());
}


}


