/*
 * Lv2PluginBrowser.cpp - implementation of the Lv2 plugin browser
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <lilv/lilvmm.hpp>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QShortcut>

#include "Lv2PluginBrowser.h"
#include "BBTrackContainer.h"
#include "ConfigManager.h"
#include "embed.h"
#include "Engine.h"
#include "FileBrowser.h"
#include "GuiApplication.h"
#include "gui_templates.h"
#include "ImportFilter.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "Lv2PluginBrowser.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "PluginFactory.h"
#include "PresetPreviewPlayHandle.h"
#include "SamplePlayHandle.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "TextFloat.h"

enum TreeWidgetItemTypes
{
	TypeFileItem = QTreeWidgetItem::UserType,
	TypeDirectoryItem
} ;



Lv2PluginBrowser::Lv2PluginBrowser(
			const QString & title, const QPixmap & pm,
			QWidget * parent) :
	SideBarWidget( title, pm, parent )
{
	setWindowTitle( tr( "Lv2 Plugin Browser" ) );
	m_l = new Lv2PluginBrowserTreeWidget( contentParent() );
	addContentWidget( m_l );

	QWidget * ops = new QWidget( contentParent() );
	ops->setFixedHeight( 24 );

	QHBoxLayout * opl = new QHBoxLayout( ops );
	opl->setMargin( 0 );
	opl->setSpacing( 0 );

	m_filterEdit = new QLineEdit( ops );
#if QT_VERSION >= 0x050000
	m_filterEdit->setClearButtonEnabled( true );
#endif
	connect( m_filterEdit, SIGNAL( textEdited( const QString & ) ),
			this, SLOT( filterItems( const QString & ) ) );

	QPushButton * reload_btn = new QPushButton(
				embed::getIconPixmap( "reload" ),
						QString::null, ops );
	connect( reload_btn, SIGNAL( clicked() ), this, SLOT( reloadTree() ) );

	opl->addWidget( m_filterEdit );
	opl->addSpacing( 5 );
	opl->addWidget( reload_btn );

	addContentWidget( ops );

	// Whenever the FileBrowser has focus, Ctrl+F should direct focus to its filter box.
	QShortcut *filterFocusShortcut = new QShortcut( QKeySequence( QKeySequence::Find ), this, SLOT(giveFocusToFilter()) );
	filterFocusShortcut->setContext(Qt::WidgetWithChildrenShortcut);

	reloadTree();
	show();
}




Lv2PluginBrowser::~Lv2PluginBrowser()
{
}




bool Lv2PluginBrowser::filterItems( const QString & filter, QTreeWidgetItem * item )
{
	// call with item=NULL to filter the entire tree
	bool anyMatched = false;

	int numChildren = item ? item->childCount() : m_l->topLevelItemCount();
	for( int i = 0; i < numChildren; ++i )
	{
		QTreeWidgetItem * it = item ? item->child( i ) : m_l->topLevelItem(i);

		// is directory?
		if( it->childCount() )
		{
			// matches filter?
			if( it->text( 0 ).
				contains( filter, Qt::CaseInsensitive ) )
			{
				// yes, then show everything below
				it->setHidden( false );
				filterItems( QString::null, it );
				anyMatched = true;
			}
			else
			{
				// only show if item below matches filter
				bool didMatch = filterItems( filter, it );
				it->setHidden( !didMatch );
				anyMatched = anyMatched || didMatch;
			}
		}
		// a standard item (i.e. no file or directory item?)
		else if( it->type() == QTreeWidgetItem::Type )
		{
			// hide if there's any filter
			it->setHidden( !filter.isEmpty() );
		}
		else
		{
			// file matches filter?
			bool didMatch = it->text( 0 ).
				contains( filter, Qt::CaseInsensitive );
			it->setHidden( !didMatch );
			anyMatched = anyMatched || didMatch;
		}
	}

	return anyMatched;
}



void Lv2PluginBrowser::reloadTree( void )
{
	//const QString text = m_filterEdit->text();
	//m_filterEdit->clear();
	//m_l->clear();
	//QStringList paths = m_directories.split( '*' );
	//for( QStringList::iterator it = paths.begin(); it != paths.end(); ++it )
	//{
		//addItems( *it );
	//}
	//expandItems();
	//m_filterEdit->setText( text );
	//filterItems( text );
}



void Lv2PluginBrowser::giveFocusToFilter()
{
	if (!m_filterEdit->hasFocus())
	{
		// give focus to filter text box and highlight its text for quick editing if not previously focused
		m_filterEdit->setFocus();
		m_filterEdit->selectAll();
	}
}



void Lv2PluginBrowser::addItems(const QString & path )
{
}




void Lv2PluginBrowser::keyPressEvent(QKeyEvent * ke )
{
	if( ke->key() == Qt::Key_F5 )
	{
		reloadTree();
	}
	else
	{
		ke->ignore();
	}
}








Lv2PluginBrowserTreeWidget::Lv2PluginBrowserTreeWidget(QWidget * parent ) :
	QTreeWidget( parent ),
	m_mousePressed( false ),
	m_pressPos(),
	m_pphMutex( QMutex::Recursive ),
	m_contextMenuItem( NULL )
{
	setColumnCount( 1 );
	headerItem()->setHidden( true );
	setSortingEnabled( false );

	connect( this, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ),
			SLOT( activateListItem( QTreeWidgetItem *, int ) ) );
	connect( this, SIGNAL( itemCollapsed( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );
	connect( this, SIGNAL( itemExpanded( QTreeWidgetItem * ) ),
				SLOT( updateDirectory( QTreeWidgetItem * ) ) );

}




Lv2PluginBrowserTreeWidget::~Lv2PluginBrowserTreeWidget()
{
}




void Lv2PluginBrowserTreeWidget::contextMenuEvent(QContextMenuEvent * e )
{
	FileItem * f = dynamic_cast<FileItem *>( itemAt( e->pos() ) );
	if( f != NULL && ( f->handling() == FileItem::LoadAsPreset ||
				 f->handling() == FileItem::LoadByPlugin ) )
	{
		m_contextMenuItem = f;
		QMenu contextMenu( this );
		contextMenu.addAction( tr( "Send to active instrument-track" ),
						this,
					SLOT( sendToActiveInstrumentTrack() ) );
		contextMenu.addAction( tr( "Open in new instrument-track/"
								"Song Editor" ),
						this,
					SLOT( openInNewInstrumentTrackSE() ) );
		contextMenu.addAction( tr( "Open in new instrument-track/"
								"B+B Editor" ),
						this,
					SLOT( openInNewInstrumentTrackBBE() ) );
		contextMenu.exec( e->globalPos() );
		m_contextMenuItem = NULL;
	}
}




void Lv2PluginBrowserTreeWidget::mousePressEvent(QMouseEvent * me )
{
	QTreeWidget::mousePressEvent( me );
	if( me->button() != Qt::LeftButton )
	{
		return;
	}

	QTreeWidgetItem * i = itemAt( me->pos() );
	if ( i )
	{
		// TODO: Restrict to visible selection
//		if ( _me->x() > header()->cellPos( header()->mapToActual( 0 ) )
//			+ treeStepSize() * ( i->depth() + ( rootIsDecorated() ?
//						1 : 0 ) ) + itemMargin() ||
//				_me->x() < header()->cellPos(
//						header()->mapToActual( 0 ) ) )
//		{
			m_pressPos = me->pos();
			m_mousePressed = true;
//		}
	}

	FileItem * f = dynamic_cast<FileItem *>( i );
	if( f != NULL )
	{
		m_pphMutex.lock();
		m_pphMutex.unlock();
	}
}




void Lv2PluginBrowserTreeWidget::mouseMoveEvent( QMouseEvent * me )
{
	if( m_mousePressed == true &&
		( m_pressPos - me->pos() ).manhattanLength() >
					QApplication::startDragDistance() )
	{
		// make sure any playback is stopped
		mouseReleaseEvent( NULL );

		FileItem * f = dynamic_cast<FileItem *>( itemAt( m_pressPos ) );
		if( f != NULL )
		{
			switch( f->type() )
			{
				case FileItem::PresetFile:
					new StringPairDrag( f->handling() == FileItem::LoadAsPreset ?
							"presetfile" : "pluginpresetfile",
							f->fullName(),
							embed::getIconPixmap( "preset_file" ), this );
					break;

				case FileItem::SampleFile:
					new StringPairDrag( "samplefile", f->fullName(),
							embed::getIconPixmap( "sample_file" ), this );
					break;
				case FileItem::SoundFontFile:
					new StringPairDrag( "soundfontfile", f->fullName(),
							embed::getIconPixmap( "soundfont_file" ), this );
					break;
				case FileItem::PatchFile:
					new StringPairDrag( "patchfile", f->fullName(),
							embed::getIconPixmap( "sample_file" ), this );
					break;
				case FileItem::VstPluginFile:
					new StringPairDrag( "vstpluginfile", f->fullName(),
							embed::getIconPixmap( "vst_plugin_file" ), this );
					break;
				case FileItem::MidiFile:
					new StringPairDrag( "importedproject", f->fullName(),
							embed::getIconPixmap( "midi_file" ), this );
					break;
				case FileItem::ProjectFile:
					new StringPairDrag( "projectfile", f->fullName(),
							embed::getIconPixmap( "project_file" ), this );
					break;

				default:
					break;
			}
		}
	}
}




void Lv2PluginBrowserTreeWidget::mouseReleaseEvent(QMouseEvent * me )
{
	m_mousePressed = false;

	m_pphMutex.lock();
	m_pphMutex.unlock();
}





void Lv2PluginBrowserTreeWidget::handleFile(FileItem * f, InstrumentTrack * it )
{
	Engine::mixer()->requestChangeInModel();
	switch( f->handling() )
	{
		case FileItem::LoadAsProject:
			if( gui->mainWindow()->mayChangeProject(true) )
			{
				Engine::getSong()->loadProject( f->fullName() );
			}
			break;

		case FileItem::LoadByPlugin:
		{
			const QString e = f->extension();
			Instrument * i = it->instrument();
			if( i == NULL ||
				!i->descriptor()->supportsFileType( e ) )
			{
				i = it->loadInstrument(
					pluginFactory->pluginSupportingExtension(e).name() );
			}
			i->loadFile( f->fullName() );
			break;
		}

		case FileItem::LoadAsPreset:
		{
			DataFile dataFile( f->fullName() );
			InstrumentTrack::removeMidiPortNode( dataFile );
			it->setSimpleSerializing();
			it->loadSettings( dataFile.content().toElement() );
			break;
		}

		case FileItem::ImportAsProject:
			ImportFilter::import( f->fullName(),
							Engine::getSong() );
			break;

		case FileItem::NotSupported:
		default:
			break;

	}
	Engine::mixer()->doneChangeInModel();
}




void Lv2PluginBrowserTreeWidget::activateListItem(QTreeWidgetItem * item,
								int column )
{
	FileItem * f = dynamic_cast<FileItem *>( item );
	if( f == NULL )
	{
		return;
	}

enum TreeWidgetItemTypes
{
	TypeFileItem = QTreeWidgetItem::UserType,
	TypeDirectoryItem
} ;


	if( f->handling() == FileItem::LoadAsProject ||
		f->handling() == FileItem::ImportAsProject )
	{
		handleFile( f, NULL );
	}
	else if( f->handling() != FileItem::NotSupported )
	{
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack,
					Engine::getBBTrackContainer() ) );
		handleFile( f, it );
	}
}




void Lv2PluginBrowserTreeWidget::openInNewInstrumentTrack( TrackContainer* tc )
{
	if( m_contextMenuItem->handling() == FileItem::LoadAsPreset ||
		m_contextMenuItem->handling() == FileItem::LoadByPlugin )
	{
		InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack, tc ) );
		handleFile( m_contextMenuItem, it );
	}
}




void Lv2PluginBrowserTreeWidget::openInNewInstrumentTrackBBE( void )
{
	openInNewInstrumentTrack( Engine::getBBTrackContainer() );
}




void Lv2PluginBrowserTreeWidget::openInNewInstrumentTrackSE( void )
{
	openInNewInstrumentTrack( Engine::getSong() );
}




void Lv2PluginBrowserTreeWidget::sendToActiveInstrumentTrack( void )
{
	// get all windows opened in the workspace
	QList<QMdiSubWindow*> pl =
			gui->mainWindow()->workspace()->
				subWindowList( QMdiArea::StackingOrder );
	QListIterator<QMdiSubWindow *> w( pl );
	w.toBack();
	// now we travel through the window-list until we find an
	// instrument-track
	while( w.hasPrevious() )
	{
		InstrumentTrackWindow * itw =
			dynamic_cast<InstrumentTrackWindow *>(
						w.previous()->widget() );
		if( itw != NULL && itw->isHidden() == false )
		{
			handleFile( m_contextMenuItem, itw->model() );
			break;
		}
	}
}




void Lv2PluginBrowserTreeWidget::updateDirectory(QTreeWidgetItem * item )
{
	Directory * dir = dynamic_cast<Directory *>( item );
	if( dir != NULL )
	{
		dir->update();
	}
}


