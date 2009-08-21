/*
 * RecentResourceListModel.cpp - implementation of RecentResourceListModel
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

#include "RecentResourceListModel.h"


RecentResourceListModel::RecentResourceListModel( ResourceDB * _db,
													int _numRows,
													QObject * _parent ) :
	QSortFilterProxyModel( _parent ),
	m_model( new ResourceListModel( _db, _parent ) ),
	m_numRows( _numRows )
{
	setSourceModel( m_model );
	setDynamicSortFilter( true );
	sort( 0, Qt::DescendingOrder );
}




ResourceItem * RecentResourceListModel::item( const QModelIndex & _idx )
{
	return m_model->item( mapToSource( _idx ) );
}




bool RecentResourceListModel::lessThan( const QModelIndex & _left,
											const QModelIndex & _right ) const
{
	return m_model->item( _left )->lastMod() < m_model->item( _right )->lastMod();
}



#include "moc_RecentResourceListModel.cxx"

/* vim: set tw=0 noexpandtab: */
