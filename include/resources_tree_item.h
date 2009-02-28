/*
 * resources_tree_item.h - header file for ResourcesTreeItem
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

#ifndef _RESOURCES_TREE_ITEM_H
#define _RESOURCES_TREE_ITEM_H

#include <QtCore/QList>

#include "resources_item.h"

#define foreachResourcesTreeItem(list)					\
		for(ResourcesTreeItemList::Iterator it=list.begin();	\
					it!=list.end();++it)

#define foreachConstResourcesTreeItem(list)				\
		for(ResourcesTreeItemList::ConstIterator it=list.begin();\
						it!=list.end();++it)


class ResourcesTreeItem;
typedef QList<ResourcesTreeItem *> ResourcesTreeItemList;


class ResourcesTreeItem
{
public:
	ResourcesTreeItem( ResourcesTreeItem * _parent = NULL,
					ResourcesItem * _item = NULL );

	~ResourcesTreeItem();

	inline void setHidden( bool _h )
	{
		m_hidden = _h;
	}

	inline bool isHidden( void ) const
	{
		return m_hidden;
	}

	int rowCount( void ) const;

	ResourcesTreeItem * getChild( int _row );

	int row( void ) const;

	inline void addChild( ResourcesTreeItem * _it )
	{
		m_children.push_back( _it );
	}

	inline void removeChild( ResourcesTreeItem * _it )
	{
		m_children.removeAll( _it );
	}

	inline ResourcesTreeItemList & children( void )
	{
		return m_children;
	}

	inline const ResourcesTreeItemList & children( void ) const
	{
		return m_children;
	}

	ResourcesTreeItem * findChild( const QString & _name,
				ResourcesItem::BaseDirectory _base_dir );

	inline ResourcesItem * item( void )
	{
		return m_item;
	}

	inline const ResourcesItem * item( void ) const
	{
		return m_item;
	}

	inline ResourcesTreeItem * parent( void )
	{
		return m_parent;
	}

	inline void setParent( ResourcesTreeItem * _parent )
	{
		m_parent = _parent;
	}

	inline bool temporaryMarker( void ) const
	{
		return m_temporaryMarker;
	}

	inline void setTemporaryMarker( bool _on )
	{
		m_temporaryMarker = _on;
	}


private:
	// hide copy-ctor
	ResourcesTreeItem( const ResourcesTreeItem & ) { }

	ResourcesTreeItem * m_parent;
	ResourcesTreeItemList m_children;

	bool m_hidden;
	bool m_temporaryMarker;

	ResourcesItem * m_item;

} ;


#endif
