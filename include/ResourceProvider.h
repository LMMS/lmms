/*
 * ResourceProvider.h - header file for ResourceProvider
 *
 * Copyright (c) 2009-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


/*! \brief The ResourceProvider class provides an abstract API for various
 * backends that can serve a resource index and data of individual ResourceItems
 *
 * A ResourceProvider abstracts I/O of resource related data. It both serves
 * a resource index (in form of a ResourceDB) and functions such as
 * #fetchData(). Subclasses have to reimplement the pure virtual functions.
 */

class ResourceProvider : public QObject
{
	Q_OBJECT
public:
	/*! \brief Constructs a ResourceProvider object.
	* \param url A generic URL (can be e.g. a local path or a web URL)
	*/
	ResourceProvider( const QString & _url );
	/*! \brief Destroys the ResourceProvider object. */
	virtual ~ResourceProvider();

	/*! \brief Returns the name of subclass.
	*
	* Used for example for generating a unique file name for the local cache
	* file. */
	virtual QString providerName() const = 0;

	/*! \brief Triggers action to update the database.
	*
	* Subclasses can start to update their index e.g. by directory crawling
    * or downloading an index file from somewhere. */
	virtual void updateDatabase() = 0;

	/*! \brief Determines size of a ResourceItem (e.g. by stat()ing local file).
	* \param item The ResourceItem whose size to determine
    * \ret The determined size of the passed ResourceItem */
	virtual int dataSize( const ResourceItem * _item ) const = 0;

	/*! \brief Fetches data represented by a ResourceItem.
	* \param item The item whose data to fetch
	* \param maxSize A limit for the data size to fetch, -1 for no limit */
	virtual QByteArray fetchData( const ResourceItem * _item,
					int _maxSize = -1 ) const = 0;

	/*! \brief Returns wether this provider provides local resources.
	*
	* This can be important for code which fetches data using a ResourceProvider
	* and which wants to know whether it should cache the fetched data somehow. */
	virtual bool isLocal() const = 0;

	/*! \brief Returns whether the ResourceProvider base class should manage
	* caching of the database.
	*
	* Some special ResourceProvider subclasses that do some kind of proxying
	* should overload this and return false. */
	virtual bool cacheDatabase() const
	{
		return true;
	}

	/*! \brief Returns URL of this ResourceProvider object. */
	inline const QString & url() const
	{
		return m_url;
	}

	/*! \brief Returns full path to local cache file for this ResourceProvider. */
	QString localCacheFile() const;

	/*! \brief Returns pointer to internal ResourceDB. */
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
