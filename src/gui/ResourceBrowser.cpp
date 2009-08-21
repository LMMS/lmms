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
#include <QtGui/QPushButton>
#include <QKeyEvent>

#include "ResourceBrowser.h"
#include "ResourceFileMapper.h"
#include "ResourceTreeModel.h"
#include "ResourceTreeView.h"
#include "engine.h"
#include "embed.h"
#include "MainWindow.h"
#include "piano.h"
#include "song.h"


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
			_parent ),
	m_previewer(),
	m_filterEdit( NULL ),
	m_treeModel( engine::mergedResourceDB() )
{
	// create a model which represents our database as a tree

	// create filter UI
	QHBoxLayout * filterLayout = new QHBoxLayout;

	QLabel * filterPixmap = new QLabel;
	filterPixmap->setPixmap( embed::getIconPixmap( "edit-find" ) );

	m_filterEdit = new QLineEdit;

	m_filterStatusLabel = new QLabel;

	filterLayout->addWidget( filterPixmap );
	filterLayout->addWidget( m_filterEdit );
	filterLayout->addWidget( m_filterStatusLabel );

	// create an according tree-view for our tree-model
	m_treeView = new ResourceTreeView( &m_treeModel, contentParent() );

	// set up context menu handling
	m_treeView->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( m_treeView,
			SIGNAL( customContextMenuRequested( const QPoint & ) ),
			this, SLOT( showContextMenu( const QPoint & ) ) );

	// track mouse press and release events
	connect( m_treeView,
			SIGNAL( pressed( const QModelIndex & ) ),
			this, SLOT( setFocusAndPreview( const QModelIndex & ) ) );
	connect( m_treeView,
			SIGNAL( clicked( const QModelIndex & ) ),
			this, SLOT( stopItemPreview( const QModelIndex & ) ) );
	// play the noise when a new item is selected
	connect( m_treeView, SIGNAL( treeViewCurrentChanged( const QModelIndex &, 
		const QModelIndex & ) ), this, 
		SLOT( currentChanged( const QModelIndex &, const QModelIndex & ) ) );

	// setup a direct connection so preview is instantly stopped when
	// drag operation begins
	connect( m_treeView,
			SIGNAL( dragStarted() ),
			this, SLOT( stopPreview() ), Qt::DirectConnection );
	connect( m_treeView,
			SIGNAL( doubleClicked( const QModelIndex & ) ),
			this,
			SLOT( triggerDefaultAction( const QModelIndex & ) ) );

	// create buttons below tree-view
	QHBoxLayout * buttonLayout = new QHBoxLayout;

	QPushButton * manageButton = new QPushButton( tr( "Manage locations" ) );
	connect( manageButton, SIGNAL( clicked() ),
			this, SLOT( manageDirectories() ) );

	QPushButton * pianoButton = new QPushButton( tr( "Show piano" ) );
	pianoButton->setCheckable( true );
	pianoButton->setChecked( true );

	buttonLayout->addWidget( manageButton );
	buttonLayout->addWidget( pianoButton );

	// create PianoView allowing the user to test selected resource
	PianoView * pianoView = new PianoView( contentParent() );
	pianoView->setModel( m_previewer.pianoModel() );
	connect( pianoButton, SIGNAL( toggled( bool ) ),
			pianoView, SLOT( setVisible( bool ) ) );

	// add widgets/layouts to us (we're a SideBarWidget)
	addContentLayout( filterLayout );
	addContentWidget( m_treeView );
	addContentLayout( buttonLayout );
	addContentWidget( pianoView );


	// instantly apply filter when typing into m_filterEdit
	connect( m_filterEdit, SIGNAL( textChanged( const QString & ) ),
					m_treeView, SLOT( setFilter( const QString & ) ) );
	connect( m_filterEdit, SIGNAL( textChanged( const QString & ) ),
					this, SLOT( updateFilterStatus() ) );
	connect( &m_treeModel, SIGNAL( itemsChanged() ),
					this, SLOT( updateFilterStatus() ) );

	// setup actions to be used in context menu
	for( int i = 0; i < (int) ( sizeof( resourceBrowserActions ) /
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

	// initialize filter status label
	updateFilterStatus();
}




ResourceBrowser::~ResourceBrowser()
{
	delete m_treeView;
	delete m_filterEdit;
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

	ResourceItem * item = m_treeModel.item( idx );
	switch( item->type() )
	{
		case ResourceItem::TypeSample:
		case ResourceItem::TypePreset:
		case ResourceItem::TypePluginSpecificResource:
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




void ResourceBrowser::startItemPreview( const QModelIndex & _idx )
{
	if( _idx.isValid() )
	{
		m_previewer.preview( m_treeModel.item( _idx ) );
	}
}


void ResourceBrowser::setFocusAndPreview( const QModelIndex & _idx )
{
	// transfor focus to the treeview. for some reason you have to
	// setFocus to the line edit above it first.
	m_filterEdit->setFocus(Qt::MouseFocusReason);
	m_treeView->setFocus(Qt::MouseFocusReason);

	startItemPreview( _idx );
}




void ResourceBrowser::stopItemPreview( const QModelIndex & )
{
	stopPreview();
}




void ResourceBrowser::currentChanged( const QModelIndex & current, 
	const QModelIndex & previous )
{

	// stop previous previews
	stopPreview();

	// start previewing the sound we just changed to
	startItemPreview( current );

	
}



void ResourceBrowser::stopPreview()
{
	m_previewer.stopPreview();
}




void ResourceBrowser::triggerDefaultAction( const QModelIndex & _idx )
{
	ResourceItem * item = m_treeModel.item( _idx );
	Actions action = NumActions;
	switch( item->type() )
	{
		case ResourceItem::TypeSample:
			action = LoadInNewTrackBBEditor;
			break;
		case ResourceItem::TypePreset:
		case ResourceItem::TypePluginSpecificResource:
		case ResourceItem::TypePlugin:
			action = LoadInNewTrackSongEditor;
			break;
		case ResourceItem::TypeProject:
			action = LoadProject;
			break;
		case ResourceItem::TypeForeignProject:
		case ResourceItem::TypeMidiFile:
			action = ImportFile;
			break;
		default:
			break;
	}
	if( action != NumActions )
	{
		triggerAction( action, item );
	}
}




void ResourceBrowser::updateFilterStatus()
{
	m_filterStatusLabel->setText( QString( "%1/%2" ).
					arg( m_treeModel.shownItems() ).
					arg( m_treeModel.totalItems() ) );
}




void ResourceBrowser::manageDirectories()
{
}




void ResourceBrowser::triggerAction( Actions _action, ResourceItem * _item )
{
	switch( _action )
	{
		case LoadProject:
			if( engine::mainWindow()->mayChangeProject() )
			{
				ResourceFileMapper mapper( _item );
				if( _item->isLocalResource() )
				{
					engine::getSong()->loadProject(
							mapper.fileName() );
				}
				else
				{
					engine::getSong()->
						createNewProjectFromTemplate(
							mapper.fileName() );
				}
			}
			break;
	}
}




#include "moc_ResourceBrowser.cxx"

/* vim: set tw=0 noexpandtab: */
