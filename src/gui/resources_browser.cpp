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

#include <QtGui/QAction>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>

#include "resources_browser.h"
#include "resources_tree_model.h"
#include "resources_tree_view.h"
#include "unified_resources_provider.h"
#include "engine.h"
#include "embed.h"


struct ActionDesc
{
	ResourcesBrowser::Actions action;
	const char * pixmap;
	const char * text;
} ;

static ActionDesc resourcesBrowserActions[] =
{
	{ ResourcesBrowser::EditProperties, "edit_draw",
		QT_TRANSLATE_NOOP( "ResourcesBrowser", "Show/edit properties" ) },
	{ ResourcesBrowser::LoadProject, "project_open",
		QT_TRANSLATE_NOOP( "ResourcesBrowser", "Load project" ) },
	{ ResourcesBrowser::LoadInNewTrackSongEditor, "songeditor",
		QT_TRANSLATE_NOOP( "ResourcesBrowser", "Load in new track in Song Editor" ) },
	{ ResourcesBrowser::LoadInNewTrackBBEditor, "bb_track",
		QT_TRANSLATE_NOOP( "ResourcesBrowser", "Load in new track in B+B Editor" ) },
	{ ResourcesBrowser::LoadInActiveInstrumentTrack, "instrument_track",
		QT_TRANSLATE_NOOP( "ResourcesBrowser", "Load into active instrument track" ) },
	{ ResourcesBrowser::DownloadIntoCollection, "mimetypes/folder-downloads",
		QT_TRANSLATE_NOOP( "ResourcesBrowser", "Download into collection" ) },
	{ ResourcesBrowser::UploadToWWW, "mimetypes/network-workgroup",
		QT_TRANSLATE_NOOP( "ResourcesBrowser", "Upload to WWW" ) },
	{ ResourcesBrowser::DeleteLocalResource, "edit-delete",
		QT_TRANSLATE_NOOP( "ResourcesBrowser", "Delete resource" ) },
	{ ResourcesBrowser::ImportFile, "project_import",
		QT_TRANSLATE_NOOP( "ResourcesBrowser", "Import file" ) }
} ;




ResourcesBrowser::ResourcesBrowser( QWidget * _parent ) :
	sideBarWidget( tr( "Resources Browser" ),
			embed::getIconPixmap( "resources_browser" ),
			_parent )
{
	// create a model which represents our database as a tree
	m_treeModel = new ResourcesTreeModel(
				engine::getResourcesProvider()->database() );

	// create filter UI
	QHBoxLayout * filterLayout = new QHBoxLayout;

	QLabel * filterPixmap = new QLabel;
	filterPixmap->setPixmap( embed::getIconPixmap( "edit-find" ) );

	QLineEdit * filterEdit = new QLineEdit;

	m_filterStatusLabel = new QLabel;

	filterLayout->addWidget( filterPixmap );
	filterLayout->addWidget( filterEdit );
	filterLayout->addWidget( m_filterStatusLabel );

	// create an according tree-view for our tree-model
	m_treeView = new ResourcesTreeView( m_treeModel, contentParent() );

	// set up context menu handling
	m_treeView->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( m_treeView,
			SIGNAL( customContextMenuRequested( const QPoint & ) ),
			this, SLOT( showContextMenu( const QPoint & ) ) );

	// add widgets/layouts to us (we're a SideBarWidget)
	addContentLayout( filterLayout );
	addContentWidget( m_treeView );


	// instantly apply filter when typing into filterEdit
	connect( filterEdit, SIGNAL( textChanged( const QString & ) ),
	                m_treeView, SLOT( setFilter( const QString & ) ) );
	connect( filterEdit, SIGNAL( textChanged( const QString & ) ),
	                this, SLOT( updateFilterStatus() ) );
	connect( m_treeModel, SIGNAL( itemsChanged() ),
	                this, SLOT( updateFilterStatus() ) );

	// setup actions to be used in context menu
	for( int i = 0;i < (int) ( sizeof( resourcesBrowserActions ) /
						sizeof( ActionDesc ) ); ++i )
	{
		Actions a = resourcesBrowserActions[i].action;
		m_actions[a] = new QAction(
			embed::getIconPixmap(
				resourcesBrowserActions[i].pixmap ),
			tr( resourcesBrowserActions[i].text ),
			this );
		m_actions[a]->setData( i );
	}
}




ResourcesBrowser::~ResourcesBrowser()
{
	delete m_treeView;
	delete m_treeModel;
}




void ResourcesBrowser::showContextMenu( const QPoint & _pos )
{
	// clicked at a valid position?
	QModelIndex idx = m_treeView->indexAt( _pos );
	if( !idx.isValid() )
	{
		return;
	}

	// construct menu depending on selected item
	QMenu m;

	ResourcesItem * item = m_treeModel->item( idx );
	switch( item->type() )
	{
		case ResourcesItem::TypeSample:
		case ResourcesItem::TypeSoundFont:
		case ResourcesItem::TypePreset:
		case ResourcesItem::TypePlugin:
			m.addAction( m_actions[LoadInNewTrackSongEditor] );
			m.addAction( m_actions[LoadInNewTrackBBEditor] );
			m.addAction( m_actions[LoadInActiveInstrumentTrack] );
			break;
		case ResourcesItem::TypeProject:
			m.addAction( m_actions[LoadProject] );
			break;
		case ResourcesItem::TypeForeignProject:
		case ResourcesItem::TypeMidiFile:
			m.addAction( m_actions[ImportFile] );
			break;
		case ResourcesItem::TypeImage:
		case ResourcesItem::TypeDirectory:
		case ResourcesItem::TypeUnknown:
		case ResourcesItem::NumTypes:
			break;
	}

	if( item->type() != ResourcesItem::TypeDirectory )
	{
		m.addSeparator();
		if( item->isLocalResource() )
		{
			m.addAction( m_actions[DeleteLocalResource] );
			m.addAction( m_actions[UploadToWWW] );
		}
		else
		{
			m.addAction( m_actions[DownloadIntoCollection] );
		}
	}

	m.addSeparator();
	m.addAction( m_actions[EditProperties] );

	// show and exec menu
	QAction * a = m.exec( m_treeView->mapToGlobal( _pos ) );
	if( a )
	{
		// trigger action if one has been selected
		triggerAction( static_cast<Actions>( a->data().toInt() ),
									item );
	}

}




void ResourcesBrowser::updateFilterStatus()
{
	m_filterStatusLabel->setText( QString( "%1/%2" ).
					arg( m_treeModel->shownItems() ).
					arg( m_treeModel->totalItems() ) );
}




void ResourcesBrowser::triggerAction( Actions _action, ResourcesItem * _item )
{
	// TODO
}




#include "moc_resources_browser.cxx"

