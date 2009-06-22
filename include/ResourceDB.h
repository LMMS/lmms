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

#include "ResourceTreeItem.h"


class ResourceDB : public QObject
{
	Q_OBJECT
public:
	typedef QHash<QString, ResourceItem *> ItemList;


	ResourceDB( ResourceProvider * _provider );
	~ResourceDB();

	void init( void );

	void load( const QString & _file );
	void save( const QString & _file );

	inline ResourceProvider * provider( void )
	{
		return m_provider;
	}

	inline const ItemList & items( void ) const
	{
		return m_items;
	}

	inline ItemList & items( void )
	{
		return m_items;
	}

	inline ResourceTreeItem * topLevelNode( void )
	{
		return &m_topLevelNode;
	}

	const ResourceItem * itemByHash( const QString & _hash ) const;
	const ResourceItem * nearestMatch( const ResourceItem & _item );

	void addItem( ResourceItem * newItem );

	void recursiveRemoveItems( ResourceTreeItem * parent,
					bool removeTopLevelParent = true );


private:
	void saveTreeItem( const ResourceTreeItem * _i, QDomDocument & _doc,
							QDomElement & _de );
	void loadTreeItem( ResourceTreeItem * _i, QDomElement & _de );

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
	ItemList m_items;
	ResourceTreeItem m_topLevelNode;


signals:
	void itemsChanged( void );
	void directoryItemAdded( const QString & _path );
	void directoryItemRemoved( const QString & _path );

} ;


#endif
