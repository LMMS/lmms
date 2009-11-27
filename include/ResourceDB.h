/*
 * ResourceDB.h - header file for ResourceDB
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _RESOURCE_DB_H
#define _RESOURCE_DB_H

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtXml/QDomDocument>

#include "ResourceItem.h"


/*! \brief The ResourceDB class organizes and caches ResourceItems.
 *
 * ResourceItem sets are organized in the ResourceDB::ItemHashMap. This
 * allows fast lookup of ResourceItems by hash string. ResourceItems are added
 * and removed by a ResourceProvider.
 *
 * The ResourceItem array can be cached to a file and restored later. This is
 * essential as otherwise e.g. the LocalResourceProvider would have to re-read
 * all directories and files and recompute hash strings based on file contents.
 *
 * One can also descend the hierarchical ResourceItem tree by starting from
 * ResourceDB::topLevelNode().
 */

class EXPORT ResourceDB : public QObject
{
	Q_OBJECT
public:
	/*! A QHash instantiation used for storing ResourceItems and allow lookup
	 * by hash string */
	typedef QHash<QString, ResourceItem *> ItemHashMap;


	/*! \brief Constructs a ResourceDB object.
	* \param provider The ResourceProvider that will manage ResourceItems in this DB
	*/
	ResourceDB( ResourceProvider * provider );
	~ResourceDB();

	/*! \brief Initializes ResourceDB object, i.e. restore cache and update itself via the ResourceProvider. */
	void init();

	/*! \brief Dumps all ResourceItems with their relations to a file.
	 * \param file The filename to save data to */
	void load( const QString & file );

	/*! \brief Restores ResourceItems with their relations from a file.
	 * \param file The filename to load data from */
	void save( const QString & file );

	inline ResourceProvider * provider()
	{
		return m_provider;
	}

	inline const ItemHashMap & items() const
	{
		return m_items;
	}

	inline ItemHashMap & items()
	{
		return m_items;
	}

	inline ResourceItem::Relation * topLevelNode()
	{
		return &m_topLevelNode;
	}

	/*! \brief Looks up a ResourceItem by hash string - similiar to items()[hash]
	 * but faster and returns NULL if not found.
	 * \param hash The hash string that is searched for
	 * \return A const pointer to the matching ResourceItem */
	const ResourceItem * itemByHash( const QString & hash ) const;

	/*! \brief Return a list of ResourceItems which somehow match the given keywords.
	 * \param keywords A list of keywords that are searched for
	 * \return A ResourceItemList which the result */
	ResourceItemList matchItems( const QStringList & keywords );

	/*! \brief Returns a ResourceItem which matches a given ResourceItem best.
	 * \param item A ResourceItem to search the best match for
	 * \return A const pointer to the best marching ResourceItem in the DB */
	const ResourceItem * nearestMatch( const ResourceItem & item );

	/*! \brief Adds given ResourceItem to DB.
	 * \param newItem The ResourceItem to be added */
	void addItem( ResourceItem * newItem );

	/*! \brief Remove items recursively starting from a certain ResourceItem::Relation
	 * \param parent The parent relation to start from with removing
	 * \param removeParent A boolean which specifies whether to also remove the parent item */
	void removeItemsRecursively( ResourceItem::Relation * parent,
									bool removeParent = true );


private:
	void saveRelation( const ResourceItem::Relation * _i, QDomDocument & _doc,
							QDomElement & _de );
	void loadRelation( ResourceItem::Relation * _i, QDomElement & _de );

	static inline QString typeName( ResourceItem::Type _t )
	{
		return s_typeNames[_t];
	}

	static inline QString baseDirName( ResourceItem::BaseDirectory _bd )
	{
		return s_baseDirNames[_bd];
	}

	static inline ResourceItem::Type typeFromName( const QString & _n )
	{
		for( TypeStringMap::ConstIterator it = s_typeNames.begin();
						it != s_typeNames.end(); ++it )
		{
			if( it.value() == _n )
			{
				return it.key();
			}
		}
		return ResourceItem::TypeUnknown;
	}

	static inline ResourceItem::BaseDirectory baseDirFromName(
							const QString & _n )
	{
		for( BaseDirStringMap::ConstIterator it =
							s_baseDirNames.begin();
					it != s_baseDirNames.end(); ++it )
		{
			if( it.value() == _n )
			{
				return it.key();
			}
		}
		return ResourceItem::BaseRoot;
	}

	typedef QMap<ResourceItem::Type, QString> TypeStringMap;
	typedef QMap<ResourceItem::BaseDirectory, QString> BaseDirStringMap;
	static TypeStringMap s_typeNames;
	static BaseDirStringMap s_baseDirNames;

	ResourceProvider * m_provider;
	ItemHashMap m_items;
	ResourceItem::Relation m_topLevelNode;


signals:
	/*! \brief Forwarded signal of ResourceProvider::itemsChanged() */
	void itemsChanged();
	/*! \brief Emitted whenever a ResourceItem of type ResourceItem::TypeDirectory is added */
	void directoryItemAdded( const QString & path );
	/*! \brief Emitted whenever a ResourceItem of type ResourceItem::TypeDirectory is removed */
	void directoryItemRemoved( const QString & path );

} ;


#endif
