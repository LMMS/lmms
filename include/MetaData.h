/*
 * MetaData.h - generic class to hold meta-data (in particular, of the song)
 *
 * Copyright (c) 2017
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

#ifndef METADATA_H
#define METADATA_H

#include <QMap>
#include <QString>
#include <QDomDocument>

class MetaData
{
 public:
	MetaData();
	QString get(const QString& k);
	bool clear();
	bool set(const QString& k,const QString& v);
	bool load( QDomDocument& doc, QDomElement& element );
	void save( QDomDocument& doc, QDomElement& element );

 protected:
	QMap<QString,QString> m_data;
};

#endif
