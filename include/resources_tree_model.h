/*
 * resources_tree_model.h - tree-model for ResourcesDB
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

#ifndef _RESOURCES_TREE_MODEL_H
#define _RESOURCES_TREE_MODEL_H

#include <QtCore/QAbstractItemModel>

#include "resources_db.h"


class ResourcesTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	ResourcesTreeModel( ResourcesDB * _db, QObject * _parent = NULL );
	virtual ~ResourcesTreeModel()
	{
	}

	virtual QVariant data( const QModelIndex & _idx,
                                        int _role = Qt::DisplayRole ) const;

	virtual Qt::ItemFlags flags( const QModelIndex & _index ) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}

	int rowCount( const QModelIndex & _parent = QModelIndex() ) const;

	virtual int columnCount( const QModelIndex & _parent =
							QModelIndex() ) const
	{
		return 1;
	}

	virtual QModelIndex index( int _row, int _col,
			const QModelIndex & _parent = QModelIndex() ) const;

	virtual QModelIndex parent ( const QModelIndex & index ) const;

	void setFilter( const QString & _s );

	// return ResourcesTreeItem belonging to a certain index
	static inline ResourcesTreeItem * treeItem( const QModelIndex & _idx )
	{
		return static_cast<ResourcesTreeItem *>(
						_idx.internalPointer() );
	}

	// return ResourcesItem belonging to a certain index
	static inline ResourcesItem * item( const QModelIndex & _idx )
	{
		return treeItem( _idx )->item();
	}


private:
	bool filterItems( ResourcesTreeItem * _item,
					const QModelIndex & _parent,
						const QStringList & _keywords );
	void setHidden( ResourcesTreeItem * _item,
				const QModelIndex & _parent,
					bool _hidden,
						bool _recursive = true );

	ResourcesDB * m_db;


signals:
	void itemsChanged( void );

} ;

#endif
