/*
 * ResourceProvider.h - header file for ResourceProvider
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _RESOURCE_PROVIDER_H
#define _RESOURCE_PROVIDER_H

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>

class ResourceDB;
class ResourceItem;


class ResourceProvider : public QObject
{
	Q_OBJECT
public:
	ResourceProvider( const QString & _url );
	virtual ~ResourceProvider();

	virtual QString providerName() const = 0;
	virtual void updateDatabase() = 0;
	virtual int dataSize( const ResourceItem * _item ) const = 0;
	virtual QByteArray fetchData( const ResourceItem * _item,
					int _maxSize = -1 ) const = 0;
	// return wether this provider provides local resources
	virtual bool isLocal() const = 0;

	virtual bool cacheDatabase() const
	{
		return true;
	}

	inline const QString & url() const
	{
		return m_url;
	}

	QString localCacheFile() const;

	ResourceDB * database()
	{
		return m_database;
	}


private:
	ResourceDB * m_database;
	QString m_url;


signals:
	void itemsChanged();

} ;


#endif
