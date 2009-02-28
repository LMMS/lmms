/*
 * resources_provider.h - header file for ResourcesProvider
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

#ifndef _RESOURCES_PROVIDER_H
#define _RESOURCES_PROVIDER_H

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QString>

class ResourcesDB;
class ResourcesItem;


class ResourcesProvider : public QObject
{
	Q_OBJECT
public:
	ResourcesProvider( const QString & _url );
	virtual ~ResourcesProvider();

	virtual QString providerName( void ) const = 0;
	virtual void updateDatabase( void ) = 0;
	virtual int dataSize( const ResourcesItem * _item ) const = 0;
	virtual QByteArray fetchData( const ResourcesItem * _item,
					int _maxSize = -1 ) const = 0;
	virtual bool cacheDatabase( void ) const
	{
		return true;
	}

	inline const QString & url( void ) const
	{
		return m_url;
	}

	QString localCacheFile( void ) const;

	ResourcesDB * database( void )
	{
		return m_database;
	}


private:
	ResourcesDB * m_database;
	QString m_url;


signals:
	void itemsChanged( void );

} ;


#endif
