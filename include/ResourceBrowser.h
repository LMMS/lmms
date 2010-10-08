/*
 * ResourceBrowser.h - header file for ResourceBrowser
 *
 * Copyright (c) 2009-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _RESOURCE_BROWSER_H
#define _RESOURCE_BROWSER_H

#include "ResourceAction.h"
#include "ResourcePreviewer.h"
#include "ResourceTreeModel.h"
#include "SideBarWidget.h"

class QAction;
class QLabel;
class ResourceItem;
class ResourceTreeView;


class ResourceBrowser : public SideBarWidget
{
	Q_OBJECT
public:
	ResourceBrowser( QWidget * _parent );
	virtual ~ResourceBrowser();


private slots:
	void showContextMenu( const QPoint & _pos );
	void startItemPreview( const QModelIndex & _idx );
	void setFocusAndPreview( const QModelIndex & _idx );
	void stopItemPreview( const QModelIndex & _idx );
	void stopPreview();
	void triggerDefaultAction( const QModelIndex & _idx );
	void updateFilterStatus();
	void manageLocations();

	void currentChanged( const QModelIndex &, const QModelIndex & );


private:
	void triggerAction( ResourceAction::Action _action, ResourceItem * _item );

	QAction * m_actions[ResourceAction::NumActions];

	// the object that will preview individual resources
	ResourcePreviewer m_previewer;

	QLineEdit * m_filterEdit;
	// our tree model on-top of a ResourceDB
	ResourceTreeModel m_treeModel;
	

	// a view for the tree model
	ResourceTreeView * m_treeView;

	QLabel * m_filterStatusLabel;

} ;


#endif

/* vim: set tw=0 noexpandtab: */
