/*
 * ResourceBrowser.cpp - implementation of ResourceBrowser
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

#include "ResourceBrowser.h"
#include "ResourceTreeModel.h"
#include "ResourceTreeView.h"
#include "UnifiedResourceProvider.h"
#include "engine.h"
#include "embed.h"


struct ActionDesc
{
	ResourceBrowser::Actions action;
	const char * pixmap;
	const char * text;
} ;

static ActionDesc resourceBrowserActions[] =
{
	{ ResourceBrowser::EditProperties, "edit_draw",
		QT_TRANSLATE_NOOP( "ResourceBrowser", "Show/edit properties" ) },
	{ ResourceBrowser::LoadProject, "project_open",
		QT_TRANSLATE_NOOP( "ResourceBrowser", "Load project" ) },
	{ ResourceBrowser::LoadInNewTrackSongEditor, "songeditor",
		QT_TRANSLATE_NOOP( "ResourceBrowser", "Load in new track in Song Editor" ) },
	{ ResourceBrowser::LoadInNewTrackBBEditor, "bb_track",
		QT_TRANSLATE_NOOP( "ResourceBrowser", "Load in new track in B+B Editor" ) },
	{ ResourceBrowser::LoadInActiveInstrumentTrack, "instrument_track",
		QT_TRANSLATE_NOOP( "ResourceBrowser", "Load into active instrument track" ) },
	{ ResourceBrowser::DownloadIntoCollection, "mimetypes/folder-downloads",
		QT_TRANSLATE_NOOP( "ResourceBrowser", "Download into collection" ) },
	{ ResourceBrowser::UploadToWWW, "mimetypes/network-workgroup",
		QT_TRANSLATE_NOOP( "ResourceBrowser", "Upload to WWW" ) },
	{ ResourceBrowser::DeleteLocalResource, "edit-delete",
		QT_TRANSLATE_NOOP( "ResourceBrowser", "Delete resource" ) },
	{ ResourceBrowser::ImportFile, "project_import",
		QT_TRANSLATE_NOOP( "ResourceBrowser", "Import file" ) }
} ;




ResourceBrowser::ResourceBrowser( QWidget * _parent ) :
	sideBarWidget( tr( "Resource Browser" ),
			embed::getIconPixmap( "resource_browser" ),
			_parent )
{
	// create a model which represents our database as a tree
	m_treeModel = new ResourceTreeModel(
				engine::resourceProvider()->database() );

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
	m_treeView = new ResourceTreeView( m_treeModel, contentParent() );

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
	for( int i = 0;i < (int) ( sizeof( resourceBrowserActions ) /
						sizeof( ActionDesc ) ); ++i )
	{
		Actions a = resourceBrowserActions[i].action;
		m_actions[a] = new QAction(
			embed::getIconPixmap(
				resourceBrowserActions[i].pixmap ),
			tr( resourceBrowserActions[i].text ),
			this );
		m_actions[a]->setData( i );
	}
}




ResourceBrowser::~ResourceBrowser()
{
	delete m_treeView;
	delete m_treeModel;
}




void ResourceBrowser::showContextMenu( const QPoint & _pos )
{
	// clicked at a valid position?
	QModelIndex idx = m_treeView->indexAt( _pos );
	if( !idx.isValid() )
	{
		return;
	}

	// construct menu depending on selected item
	QMenu m;

	ResourceItem * item = m_treeModel->item( idx );
	switch( item->type() )
	{
		case ResourceItem::TypeSample:
		case ResourceItem::TypeSoundFont:
		case ResourceItem::TypePreset:
		case ResourceItem::TypePlugin:
			m.addAction( m_actions[LoadInNewTrackSongEditor] );
			m.addAction( m_actions[LoadInNewTrackBBEditor] );
			m.addAction( m_actions[LoadInActiveInstrumentTrack] );
			break;
		case ResourceItem::TypeProject:
			m.addAction( m_actions[LoadProject] );
			break;
		case ResourceItem::TypeForeignProject:
		case ResourceItem::TypeMidiFile:
			m.addAction( m_actions[ImportFile] );
			break;
		case ResourceItem::TypeImage:
		case ResourceItem::TypeDirectory:
		case ResourceItem::TypeUnknown:
		case ResourceItem::NumTypes:
			break;
	}

	if( item->type() != ResourceItem::TypeDirectory )
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




void ResourceBrowser::updateFilterStatus()
{
	m_filterStatusLabel->setText( QString( "%1/%2" ).
					arg( m_treeModel->shownItems() ).
					arg( m_treeModel->totalItems() ) );
}




void ResourceBrowser::triggerAction( Actions _action, ResourceItem * _item )
{
	// TODO
}




#include "moc_ResourceBrowser.cxx"

