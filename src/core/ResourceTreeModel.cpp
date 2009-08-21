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

#include "ResourceTreeModel.h"


ResourceTreeModel::ResourceTreeModel( ResourceDB * _db, QObject * _parent ) :
	ResourceModel( _db, _parent )
{
}




int ResourceTreeModel::rowCount( const QModelIndex & _parent ) const
{
	if( _parent.column() > 0 )
	{
		return 0;
	}

	if( !_parent.isValid() )
	{
		return db()->topLevelNode()->rowCount( this );
	}
	return relation( _parent )->rowCount( this );
}




QModelIndex ResourceTreeModel::index( int _row, int _col,
					const QModelIndex & _parent ) const
{
	if( !hasIndex( _row, _col, _parent ) )
	{
		return QModelIndex();
	}

	ResourceItem::Relation * parentItem;

	if( !_parent.isValid() )
	{
		parentItem = db()->topLevelNode();
	}
	else
	{
		parentItem = relation( _parent );
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

	ResourceItem::Relation * childItem = relation( _idx );
	ResourceItem::Relation * parentItem = childItem->parent();
	if( parentItem == db()->topLevelNode() )
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




void ResourceTreeModel::updateFilters()
{
	filterItems( db()->topLevelNode(),
					createIndex( 0, 0, db()->topLevelNode() ) );
	if( keywordFilter().isEmpty() )
	{
		emit layoutChanged();
	}
}




bool ResourceTreeModel::filterItems( ResourceItem::Relation * _item,
										const QModelIndex & _parent )
{
	if( _item->item() && itemMatchesFilter( *( _item->item() ) ) )
	{
		setHidden( _item, _parent, false );
		return true;
	}

	int row = 0;
	bool hide = true;
	foreachResourceItemRelation( _item->children() )
	{
		QModelIndex idx = createIndex( row, 0, *it );
		if( filterItems( *it, idx ) )
		{
			hide = false;
		}
		++row;
	}

	setHidden( _item, _parent, hide, false );
	return !hide;
}




void ResourceTreeModel::setHidden( ResourceItem::Relation * _item,
						const QModelIndex & _parent,
						bool _hide, bool _recursive )
{
	if( _recursive )
	{
		int row = 0;
		foreachResourceItemRelation( _item->children() )
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
