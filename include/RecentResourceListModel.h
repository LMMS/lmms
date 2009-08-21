/*
 * RecentResourceListModel.h - a model providing list of recent resources
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

#ifndef _RECENT_RESOURCE_LIST_MODEL_H
#define _RECENT_RESOURCE_LIST_MODEL_H

#include <QtGui/QSortFilterProxyModel>

#include "ResourceListModel.h"


class RecentResourceListModel : public QSortFilterProxyModel
{
public:
	RecentResourceListModel( ResourceDB * _db, int _numRows = -1,
								QObject * _parent = NULL );
	virtual ~RecentResourceListModel()
	{
	}

	virtual int rowCount( const QModelIndex & = QModelIndex() ) const
	{
		return m_numRows > 0 ? m_numRows : m_model->rowCount();
	}

	// return ResourceListModel, this proxy-model is operating on
	ResourceListModel * resourceListModel()
	{
		return m_model;
	}

	ResourceItem * item( const QModelIndex & _idx );


protected:
	// compares items at two indices by date-property
	virtual bool lessThan( const QModelIndex &, const QModelIndex & ) const;


private:
	ResourceListModel * m_model;
	int m_numRows;

} ;

#endif
