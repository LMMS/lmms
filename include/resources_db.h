/*
 * resources_db.h - header file for ResourcesDB
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

#ifndef _RESOURCES_DB_H
#define _RESOURCES_DB_H

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtXml/QDomDocument>

#include "resources_tree_item.h"


class ResourcesDB : public QObject
{
	Q_OBJECT
public:
	typedef QHash<QString, ResourcesItem *> ItemList;


	ResourcesDB( ResourcesProvider * _provider );
	~ResourcesDB();

	void init( void );

	void load( const QString & _file );
	void save( const QString & _file );

	inline ResourcesProvider * provider( void )
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

	inline ResourcesTreeItem * topLevelNode( void )
	{
		return &m_topLevelNode;
	}

	const ResourcesItem * nearestMatch( const ResourcesItem & _item );

	void addItem( ResourcesItem * newItem );

	void recursiveRemoveItems( ResourcesTreeItem * parent,
					bool removeTopLevelParent = true );


private:
	void saveTreeItem( const ResourcesTreeItem * _i, QDomDocument & _doc,
							QDomElement & _de );
	void loadTreeItem( ResourcesTreeItem * _i, QDomElement & _de );


	ResourcesProvider * m_provider;
	ItemList m_items;
	ResourcesTreeItem m_topLevelNode;


signals:
	void itemsChanged( void );
	void directoryItemAdded( const QString & _path );
	void directoryItemRemoved( const QString & _path );

} ;


#endif
