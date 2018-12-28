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

//#include <lilv/lilvmm.hpp>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QShortcut>
#include <QDebug>

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
#include "Lv2Manager.h"
#include "Lv2PluginBrowser.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "EmbeddedPluginFactory.h"
#include "PresetPreviewPlayHandle.h"
#include "SamplePlayHandle.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "TextFloat.h"

Lv2PluginBrowser::Lv2PluginBrowser(
			const QString & title, const QPixmap & pm,
			QWidget * parent) :
	SideBarWidget( title, pm, parent )
{
	setWindowTitle( tr( "Lv2 Plugin Browser" ) );
	m_treeWidget = new Lv2PluginBrowserTreeWidget( contentParent() );
	addContentWidget( m_treeWidget );

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
	//qDebug("filtering items");
	// call with item=NULL to filter the entire tree
	bool anyMatched = false;

	int numChildren = item ? item->childCount() : m_treeWidget->topLevelItemCount();
	for( int i = 0; i < numChildren; ++i )
	{
		QTreeWidgetItem * it = item ? item->child( i ) : m_treeWidget->topLevelItem(i);

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



/**
 * Reloads the tree. Rereads plugins from system */
void Lv2PluginBrowser::reloadTree( void )
{
	const QString text = m_filterEdit->text();

	m_filterEdit->clear();
	m_treeWidget->clear();
	addItems();
	filterItems( text );
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



void Lv2PluginBrowser::addItems()
{
	Lv2Manager & manager =  Lv2Manager::getInstance();
  const LilvPlugins * plugins = manager.plugins;
	LILV_FOREACH (plugins, it, plugins)
    {
      const LilvPlugin* p =
        lilv_plugins_get(plugins, it);

      const char * uri =
	lilv_node_as_string (lilv_plugin_get_uri (p));
			m_treeWidget->addTopLevelItem (
                new Lv2PluginItem (QString::fromUtf8(uri)));
    }
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
}

Lv2PluginBrowserTreeWidget::~Lv2PluginBrowserTreeWidget()
{
}




void Lv2PluginBrowserTreeWidget::contextMenuEvent(QContextMenuEvent * e )
{
	Lv2PluginItem * f = dynamic_cast<Lv2PluginItem *>( itemAt( e->pos() ) );
	if( f != NULL )
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
	//qDebug("mouse press event");
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

	Lv2PluginItem * f = dynamic_cast<Lv2PluginItem *>( i );
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

		Lv2PluginItem * item = dynamic_cast<Lv2PluginItem *>( itemAt( m_pressPos ) );
		if( item != NULL )
		{
			new StringPairDrag( "instrument",
			item->get_uri (),
			embed::getIconPixmap( "vst_plugin_file" ), this );
		}
	}
}

void Lv2PluginBrowserTreeWidget::mouseReleaseEvent(QMouseEvent * me )
{
	m_mousePressed = false;

	m_pphMutex.lock();
	m_pphMutex.unlock();
}

void Lv2PluginBrowserTreeWidget::handlePlugin( Lv2PluginItem * f, InstrumentTrack * it )
{
	Engine::mixer()->requestChangeInModel();

	Instrument * i = it->instrument();
	if( i == NULL )
	{
		i = it->loadInstrument(f->get_uri(), Plugin::Lv2);
	}

	Engine::mixer()->doneChangeInModel();
}

void Lv2PluginBrowserTreeWidget::activateListItem(QTreeWidgetItem * item,
								int column )
{
	Lv2PluginItem * lv2pi = dynamic_cast<Lv2PluginItem *>( item );
	if( lv2pi == NULL )
	{
		return;
	}

	InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
	Track::create( Track::InstrumentTrack,
		Engine::getBBTrackContainer() ) );
	handlePlugin( lv2pi, it );
}

void Lv2PluginBrowserTreeWidget::openInNewInstrumentTrack( TrackContainer* tc )
{
  InstrumentTrack * it = dynamic_cast<InstrumentTrack *>(
    Track::create( Track::InstrumentTrack, tc ) );
  handlePlugin( m_contextMenuItem, it );
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
	//qDebug("sending to active inst track");
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
			handlePlugin( m_contextMenuItem, itw->model() );
			break;
		}
	}
}

Lv2PluginItem::Lv2PluginItem(const QString & uri) :
    QTreeWidgetItem(QStringList(uri)),
	plugin_uri(uri)
{
	initPixmaps();
}

QPixmap * Lv2PluginItem::s_Lv2PluginPixmap = NULL;

void Lv2PluginItem::initPixmaps( void )
{
	Lv2PluginItem::s_Lv2PluginPixmap = new QPixmap( embed::getIconPixmap(
						"vst_plugin_file", 16, 16 ) );
}

