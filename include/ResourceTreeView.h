/*
 * ResourceTreeView.h - view for ResourceTreeModel
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

#ifndef _RESOURCE_TREE_VIEW_H
#define _RESOURCE_TREE_VIEW_H

#include <QtGui/QTreeView>


class ResourceTreeModel;

class ResourceTreeView : public QTreeView
{
	Q_OBJECT
public:
	ResourceTreeView( ResourceTreeModel * _tm, QWidget * _parent );


public slots:
	void setFilter( const QString & _s );
	void updateFilter();

	void currentChanged( const QModelIndex & current,
		const QModelIndex & previous );
	void selectionChanged( const QItemSelection & selected, 
		const QItemSelection & deselected );

protected:
	virtual void startDrag( Qt::DropActions supportedActions );


private:
	ResourceTreeModel * m_tm;
 
	QString m_lastFilter;


signals:
	void dragStarted();

	void treeViewCurrentChanged( const QModelIndex & current,
		const QModelIndex & previous );
	void treeViewSelectionChanged( const QItemSelection & selected, 
		const QItemSelection & deselected );

} ;


#endif

/* vim: set tw=0 noexpandtab: */
