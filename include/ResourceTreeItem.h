/*
 * ResourceTreeItem.h - header file for ResourceTreeItem
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

#ifndef _RESOURCE_TREE_ITEM_H
#define _RESOURCE_TREE_ITEM_H

#include <QtCore/QList>

#include "ResourceItem.h"

#define foreachResourceTreeItem(list)					\
		for(ResourceTreeItemList::Iterator it=list.begin();	\
					it!=list.end();++it)

#define foreachConstResourceTreeItem(list)				\
		for(ResourceTreeItemList::ConstIterator it=list.begin();\
						it!=list.end();++it)


class ResourceTreeItem;
typedef QList<ResourceTreeItem *> ResourceTreeItemList;


class ResourceTreeItem
{
public:
	ResourceTreeItem( ResourceTreeItem * _parent = NULL,
					ResourceItem * _item = NULL );

	~ResourceTreeItem();

	inline void setHidden( bool _h )
	{
		m_hidden = _h;
	}

	inline bool isHidden( void ) const
	{
		return m_hidden;
	}

	int rowCount( void ) const;

	ResourceTreeItem * getChild( int _row );

	int row( void ) const;

	inline void addChild( ResourceTreeItem * _it )
	{
		m_children.push_back( _it );
	}

	inline void removeChild( ResourceTreeItem * _it )
	{
		m_children.removeAll( _it );
	}

	inline ResourceTreeItemList & children( void )
	{
		return m_children;
	}

	inline const ResourceTreeItemList & children( void ) const
	{
		return m_children;
	}

	ResourceTreeItem * findChild( const QString & _name,
				ResourceItem::BaseDirectory _base_dir );

	inline ResourceItem * item( void )
	{
		return m_item;
	}

	inline const ResourceItem * item( void ) const
	{
		return m_item;
	}

	inline ResourceTreeItem * parent( void )
	{
		return m_parent;
	}

	inline void setParent( ResourceTreeItem * _parent )
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
	ResourceTreeItem( const ResourceTreeItem & ) { }

	ResourceTreeItem * m_parent;
	ResourceTreeItemList m_children;

	bool m_hidden;
	bool m_temporaryMarker;

	ResourceItem * m_item;

} ;


#endif
