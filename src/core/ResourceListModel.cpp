/*
 * ResourceListModel.cpp - implementation of ResourceListModel
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

#include "ResourceListModel.h"


ResourceListModel::ResourceListModel( ResourceDB * _db, QObject * _parent ) :
	ResourceModel( _db, _parent ),
	m_lookupTable()
{
	updateFilters();
}




int ResourceListModel::rowCount( const QModelIndex & _parent ) const
{
	Q_UNUSED(_parent);

	// return number of non-hidden items
	return m_lookupTable.size();
}




QModelIndex ResourceListModel::index( int _row, int _col,
										const QModelIndex & _parent ) const
{
	if( _row < m_lookupTable.size() )
	{
		return createIndex( _row, _col, m_lookupTable[_row]->relation() );
	}
	return QModelIndex();
}




void ResourceListModel::updateFilters()
{
	// update lookup table so ResourceListModel::index(...) can run with O(1)
	int items = 0;
	if( keywordFilter().isEmpty() &&
			typeFilter() == ResourceItem::TypeUnknown )
	{
		// unhide all items if empty filter string given
		m_lookupTable.resize( db()->items().size() );
		foreach( ResourceItem * item, db()->items() )
		{
			item->setHidden( false, this );
			m_lookupTable[items] = item;
			++items;
		}
	}
	else
	{
		// filter and count number of non-hidden items
		foreach( ResourceItem * item, db()->items() )
		{
			if( itemMatchesFilter( *item ) )
			{
				item->setHidden( false, this );
				++items;
			}
			else
			{
				item->setHidden( true, this );
			}
		}

		// resize our lookup table accordingly
		m_lookupTable.resize( items );

		// fill the lookup table
		items = 0;
		foreach( ResourceItem * item, db()->items() )
		{
			// insert all items that are not hidden due to filtering
			if( !item->isHidden( this ) )
			{
				m_lookupTable[items] = item;
				++items;
			}
		}
	}

	// finally notify view about changed data
	emit layoutChanged();
}


/* vim: set tw=0 noexpandtab: */
