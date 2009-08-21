/*
 * ResourceListModel.h - a list model implementation for resources
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

#ifndef _RESOURCE_LIST_MODEL_H
#define _RESOURCE_LIST_MODEL_H

#include <QtCore/QVector>

#include "ResourceModel.h"


class ResourceListModel : public ResourceModel
{
public:
	ResourceListModel( ResourceDB * _db, QObject * _parent = NULL );
	virtual ~ResourceListModel()
	{
	}

	virtual int rowCount( const QModelIndex & _parent = QModelIndex() ) const;

	virtual QModelIndex index( int _row, int _col,
			const QModelIndex & _parent = QModelIndex() ) const;

	virtual QModelIndex parent( const QModelIndex & ) const
	{
		return QModelIndex();
	}

	virtual void updateFilters();


private:
	QVector<ResourceItem *> m_lookupTable;

} ;

#endif
