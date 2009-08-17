/*
 * ResourceTreeView.cpp - implementation of ResourceTreeView
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

#include "ResourceTreeView.h"
#include "ResourceTreeModel.h"


ResourceTreeView::ResourceTreeView( ResourceTreeModel * _tm,
					QWidget * _parent ) :
	QTreeView( _parent ),
	m_tm( _tm )
{
	setHeaderHidden( true );
	setDragEnabled( true );

	setModel( m_tm );
	connect( m_tm, SIGNAL( itemsChanged() ),
			this, SLOT( updateFilter() ) );
}




void ResourceTreeView::setFilter( const QString & _s )
{
	setUpdatesEnabled( false );
	if( _s.isEmpty() )
	{
		collapseAll();
		m_tm->setFilter( _s );
	}
	else
	{
		m_tm->setFilter( _s );
		expandToDepth( _s.size() );
	}
	setUpdatesEnabled( true );

	m_lastFilter = _s;
}




void ResourceTreeView::currentChanged( const QModelIndex & _current,
										const QModelIndex & _previous )
{
	emit treeViewCurrentChanged( _current, _previous );
	QTreeView::currentChanged( _current, _previous );
}




void ResourceTreeView::selectionChanged( const QItemSelection & _selected,
											const QItemSelection & _deselected )
{
	emit treeViewSelectionChanged( _selected, _deselected );
	QTreeView::selectionChanged( _selected, _deselected );
}




void ResourceTreeView::updateFilter()
{
	setFilter( m_lastFilter );
}




void ResourceTreeView::startDrag( Qt::DropActions supportedActions )
{
	emit dragStarted();

	QTreeView::startDrag( supportedActions );
}



#include "moc_ResourceTreeView.cxx"

/* vim: set tw=0 noexpandtab: */
