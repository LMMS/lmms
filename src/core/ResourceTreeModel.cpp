/*
 * ResourceTreeModel.cpp - implementation of ResourceTreeModel
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

#include <QtGui/QPainter>

#include "ResourceTreeModel.h"
#include "embed.h"
#include "plugin.h"
#include "string_pair_drag.h"


ResourceTreeModel::ResourceTreeModel( ResourceDB * _db, QObject * _parent ) :
	ResourceModel( _db, _parent )
{
}




int ResourceTreeModel::rowCount( const QModelIndex & _parent ) const
{
	ResourceTreeItem * parentItem;

	if( _parent.column() > 0 )
	{
		return 0;
	}

	if( !_parent.isValid() )
	{
		parentItem = m_db->topLevelNode();
	}
	else
	{
		parentItem = treeItem( _parent );
	}
	return parentItem->rowCount( this );
}




QModelIndex ResourceTreeModel::index( int _row, int _col,
					const QModelIndex & _parent ) const
{
	if( !hasIndex( _row, _col, _parent ) )
	{
		return QModelIndex();
	}

	ResourceTreeItem * parentItem;

	if( !_parent.isValid() )
	{
		parentItem = m_db->topLevelNode();
	}
	else
	{
		parentItem = treeItem( _parent );
	}

	if( _row < parentItem->rowCount( this ) )
	{
		return createIndex( _row, _col, parentItem->getChild( _row, this ) );
	}
	return QModelIndex();
}




QModelIndex ResourceTreeModel::parent( const QModelIndex & _idx ) const
{
	if( !_idx.isValid() )
	{
		return QModelIndex();
	}

	ResourceTreeItem * childItem = treeItem( _idx );
	ResourceTreeItem * parentItem = childItem->parent();
	if( parentItem == m_db->topLevelNode() )
	{
		return QModelIndex();
	}

	int row = 0;
	if( parentItem )
	{
		row = parentItem->row( this );
	}
	return createIndex( row, 0, parentItem );
}




void ResourceTreeModel::setFilter( const QString & _s )
{
	filterItems( m_db->topLevelNode(),
				createIndex( 0, 0, m_db->topLevelNode() ),
						_s.toLower().split( " " ) );
	if( _s.isEmpty() )
	{
		emit layoutChanged();
	}
}




bool ResourceTreeModel::filterItems( ResourceTreeItem * _item,
						const QModelIndex & _parent,
						const QStringList & _keywords )
{
	if( _item->item() )
	{
		ResourceItem * i = _item->item();
		bool accept = true;
		for( QStringList::ConstIterator it = _keywords.begin();
						it != _keywords.end(); ++it )
		{
			if( !( i->name().toLower().contains( *it ) ||
				i->path().toLower().contains( *it ) ||
				i->author().toLower().contains( *it ) ||
				i->tags().toLower().contains( *it ) ) )
			{
				accept = false;
				break;
			}
		}
		if( accept )
		{
			setHidden( _item, _parent, false );
			return true;
		}
	}

	int row = 0;
	bool hide = true;
	for( ResourceTreeItemList::Iterator it = _item->children().begin();
					it != _item->children().end(); ++it )
	{
		QModelIndex idx = createIndex( row, 0, *it );
		if( filterItems( *it, idx , _keywords ) )
		{
			hide = false;
		}
		++row;
	}

	setHidden( _item, _parent, hide, false );
	return !hide;
}




void ResourceTreeModel::setHidden( ResourceTreeItem * _item,
						const QModelIndex & _parent,
						bool _hide, bool _recursive )
{
	if( _recursive )
	{
		int row = 0;
		for( ResourceTreeItemList::Iterator it =
						_item->children().begin();
					it != _item->children().end(); ++it )
		{
			setHidden( *it, createIndex( row, 0, *it ), _hide );
			++row;
		}
	}
	if( _item->item() && _item->item()->isHidden( this ) != _hide )
	{
		_item->item()->setHidden( _hide, this );

/*		if( _hide )
		{
			int row = _item->row();
			beginRemoveRows( _parent, row, row );
			_item->setHidden( true );
			endRemoveRows();
		}
		else
		{
			_item->setHidden( false );
			int row = _item->row();
			beginInsertRows( _parent, row, row );
			endInsertRows();
		}*/
	}
}



/* vim: set tw=0 noexpandtab: */
