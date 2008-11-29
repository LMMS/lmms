/*
 * resources_tree_view.h - view for ResourcesTreeModel
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _RESOURCES_TREE_VIEW_H
#define _RESOURCES_TREE_VIEW_H

#include <QtGui/QTreeView>


class ResourcesTreeModel;

class ResourcesTreeView : public QTreeView
{
	Q_OBJECT
public:
	ResourcesTreeView( ResourcesTreeModel * _tm, QWidget * _parent );


public slots:
	void setFilter( const QString & _s );
	void updateFilter( void );

private:
	ResourcesTreeModel * m_tm;

	QString m_lastFilter;

} ;


#endif
