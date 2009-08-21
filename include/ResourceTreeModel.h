/*
 * ResourceTreeModel.h - a tree model implementation for resources
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

#ifndef _RESOURCE_TREE_MODEL_H
#define _RESOURCE_TREE_MODEL_H

#include "ResourceModel.h"


class ResourceTreeModel : public ResourceModel
{
public:
	ResourceTreeModel( ResourceDB * _db, QObject * _parent = NULL );
	virtual ~ResourceTreeModel()
	{
	}

	virtual int rowCount( const QModelIndex & _parent = QModelIndex() ) const;

	virtual QModelIndex index( int _row, int _col,
			const QModelIndex & _parent = QModelIndex() ) const;

	virtual QModelIndex parent( const QModelIndex & _index ) const;

	virtual void updateFilters();


private:
	bool filterItems( ResourceItem::Relation * _item,
						const QModelIndex & _parent );
	void setHidden( ResourceItem::Relation * _item,
					const QModelIndex & _parent,
					bool _hidden,
						bool _recursive = true );

} ;

#endif
