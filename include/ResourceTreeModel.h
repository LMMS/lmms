/*
 * ResourceTreeModel.h - tree-model for ResourceDB
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

#include <QtCore/QAbstractItemModel>

#include "ResourceDB.h"


class ResourceTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	ResourceTreeModel( ResourceDB * _db, QObject * _parent = NULL );
	virtual ~ResourceTreeModel()
	{
	}

	virtual QVariant data( const QModelIndex & _idx,
                                        int _role = Qt::DisplayRole ) const;

	virtual Qt::ItemFlags flags( const QModelIndex & _index ) const;

	int rowCount( const QModelIndex & _parent = QModelIndex() ) const;

	virtual int columnCount( const QModelIndex & _parent =
							QModelIndex() ) const
	{
		return 1;
	}

	virtual QModelIndex index( int _row, int _col,
			const QModelIndex & _parent = QModelIndex() ) const;

	virtual QModelIndex parent( const QModelIndex & index ) const;

	// return list of possible MIME types for items in this model
	virtual QStringList mimeTypes() const;

	// used for drag'n'drop - return proper MIME data for indexes
	virtual QMimeData * mimeData( const QModelIndexList & _indexes ) const;

	void setFilter( const QString & _s );

	// return ResourceTreeItem belonging to a certain index
	static inline ResourceTreeItem * treeItem( const QModelIndex & _idx )
	{
		return static_cast<ResourceTreeItem *>(
						_idx.internalPointer() );
	}

	// return ResourceItem belonging to a certain index
	static inline ResourceItem * item( const QModelIndex & _idx )
	{
		return treeItem( _idx )->item();
	}

	int totalItems() const;
	int shownItems() const;


private:
	bool filterItems( ResourceTreeItem * _item,
					const QModelIndex & _parent,
						const QStringList & _keywords );
	void setHidden( ResourceTreeItem * _item,
				const QModelIndex & _parent,
					bool _hidden,
						bool _recursive = true );

	ResourceDB * m_db;


signals:
	void itemsChanged( void );

} ;

#endif
