/*
 * resources_browser.cpp - implementation of ResourcesBrowser
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

#include <QtGui/QLineEdit>

#include "resources_browser.h"
#include "resources_tree_model.h"
#include "resources_tree_view.h"
#include "unified_resources_provider.h"
#include "engine.h"
#include "embed.h"



ResourcesBrowser::ResourcesBrowser( QWidget * _parent ) :
	sideBarWidget( tr( "Resources Browser" ),
			embed::getIconPixmap( "resources_browser" ),
			_parent )
{
	// create a model which represents our database as a tree
	m_treeModel = new ResourcesTreeModel(
				engine::getResourcesProvider()->database() );

	// create an according tree-view for our tree-model
	m_treeView = new ResourcesTreeView( m_treeModel, contentParent() );

	QLineEdit * filterEdit = new QLineEdit ( contentParent() );

	// add widgets to us (we're a SideBarWidget)
	addContentWidget( m_treeView );
	addContentWidget( filterEdit );


	// instantly apply filter when typing into filterEdit
	connect( filterEdit, SIGNAL( textChanged( const QString & ) ),
	                m_treeView, SLOT( setFilter( const QString & ) ) );
}




ResourcesBrowser::~ResourcesBrowser()
{
	delete m_treeView;
	delete m_treeModel;
}



#include "moc_resources_browser.cxx"

