/*
 * ControllerRackView.cpp - view for song's controllers
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2010-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QApplication>
#include <QLayout>
#include <QMdiSubWindow>
#include <QPushButton>
#include <QScrollArea>
#include <QMdiArea>
#include <QMessageBox>
#include <QAction>

#include "Song.h"
#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "GroupBox.h"
#include "ControllerRackView.h"
#include "ControllerView.h"
#include "LfoController.h"


ControllerRackView::ControllerRackView( ) :
	QWidget(),
	m_nextIndex(0)
{
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setWindowTitle( tr( "Controller Rack" ) );

	m_scrollArea = new QScrollArea( this );
	m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
	m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	QWidget * scrollAreaWidget = new QWidget( m_scrollArea );
	m_scrollAreaLayout = new QVBoxLayout( scrollAreaWidget );
	m_scrollAreaLayout->addStretch();
	scrollAreaWidget->setLayout( m_scrollAreaLayout );

	m_scrollArea->setWidget( scrollAreaWidget );
	m_scrollArea->setWidgetResizable( true );

	m_addButton = new QPushButton( this );
	m_addButton->setText( tr( "Add" ) );

	connect( m_addButton, SIGNAL( clicked() ),
			this, SLOT( addController() ) );

	Song * song = Engine::getSong();
	connect( song, SIGNAL( controllerAdded( Controller* ) ), SLOT( onControllerAdded( Controller* ) ) );
	connect( song, SIGNAL( controllerRemoved( Controller* ) ), SLOT( onControllerRemoved( Controller* ) ) );

	QVBoxLayout * layout = new QVBoxLayout();
	layout->addWidget( m_scrollArea );
	layout->addWidget( m_addButton );
	this->setLayout( layout );

	QMdiSubWindow * subWin = getGUI()->mainWindow()->addWindowedWidget( this );

	// No maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );
	
	subWin->setAttribute( Qt::WA_DeleteOnClose, false );
	subWin->move( 680, 310 );
	subWin->resize( 350, 200 );
	subWin->setFixedWidth( 350 );
	subWin->setMinimumHeight( 200 );
}




ControllerRackView::~ControllerRackView()
{
}




void ControllerRackView::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void ControllerRackView::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}




void ControllerRackView::deleteController( ControllerView * _view )
{
	Controller * c = _view->getController();

	if( c->connectionCount() > 0 )
	{
		QMessageBox msgBox;
		msgBox.setIcon( QMessageBox::Question );
		msgBox.setWindowTitle( tr("Confirm Delete") );
		msgBox.setText( tr("Confirm delete? There are existing connection(s) "
				"associated with this controller. There is no way to undo.") );
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		if( msgBox.exec() != QMessageBox::Ok )
		{
			return;
		}
	}

	Song * song = Engine::getSong();
	song->removeController( c );
}


void ControllerRackView::moveUp( ControllerView* view )
{
	if( view != m_controllerViews.first() )
	{
		int i = 0;
		for( QVector<ControllerView *>::Iterator it = m_controllerViews.begin();
					it != m_controllerViews.end(); it++, i++ )
		{
			if( *it == view )
			{
				break;
			}
		}

		ControllerView * temp = m_controllerViews[ i - 1 ];

		m_controllerViews[i - 1] = view;
		m_controllerViews[i] = temp;

		m_scrollAreaLayout->removeWidget( view );
		m_scrollAreaLayout->insertWidget( i - 1, view );
	}
}

void ControllerRackView::moveDown( ControllerView* view )
{
	if( view != m_controllerViews.last() )
	{
		// moving next controller up is the same
		moveUp( *( std::find( m_controllerViews.begin(), m_controllerViews.end(), view ) + 1 ) );
	}
}


void ControllerRackView::onControllerAdded( Controller * controller )
{
	QWidget * scrollAreaWidget = m_scrollArea->widget();

	ControllerView * controllerView = new ControllerView( controller, scrollAreaWidget );

	connect( controllerView, SIGNAL( moveUp( ControllerView * ) ),
		 this, SLOT( moveUp( ControllerView * ) ), Qt::QueuedConnection );
	connect( controllerView, SIGNAL( moveDown( ControllerView * ) ),
		 this, SLOT( moveDown( ControllerView * ) ), Qt::QueuedConnection );
	connect( controllerView, SIGNAL( deleteController( ControllerView * ) ),
		 this, SLOT( deleteController( ControllerView * ) ), Qt::QueuedConnection );

	QAction* moveUpAction = new QAction( controllerView );
	moveUpAction->setShortcut( Qt::Key_Up | Qt::AltModifier );
	moveUpAction->setShortcutContext( Qt::WidgetShortcut );
	connect( moveUpAction, SIGNAL( triggered() ), controllerView, SLOT( moveUp( ) ) );
	controllerView->addAction( moveUpAction );

	QAction* moveDownAction = new QAction( controllerView );
	moveDownAction->setShortcut( Qt::Key_Down | Qt::AltModifier );
	moveDownAction->setShortcutContext( Qt::WidgetShortcut );
	connect( moveDownAction, SIGNAL( triggered() ), controllerView, SLOT( moveDown( ) ) );
	controllerView->addAction( moveDownAction );

	m_controllerViews.append( controllerView );
	m_scrollAreaLayout->insertWidget( m_nextIndex, controllerView );

	++m_nextIndex;
}




void ControllerRackView::onControllerRemoved( Controller * removedController )
{
	ControllerView * viewOfRemovedController = 0;

	QVector<ControllerView *>::const_iterator end = m_controllerViews.end();
	for ( QVector<ControllerView *>::const_iterator it = m_controllerViews.begin(); it != end; ++it)
	{
		ControllerView *currentControllerView = *it;
		if ( currentControllerView->getController() == removedController )
		{
			viewOfRemovedController = currentControllerView;
			break;
		}
	}

	if (viewOfRemovedController )
	{
		m_controllerViews.erase( std::find( m_controllerViews.begin(),
					m_controllerViews.end(), viewOfRemovedController ) );

		delete viewOfRemovedController;
		--m_nextIndex;
	}
}




void ControllerRackView::addController()
{
	// TODO: Eventually let the user pick from available controller types

	Engine::getSong()->addController( new LfoController( Engine::getSong() ) );

	// fix bug which always made ControllerRackView loose focus when adding
	// new controller
	setFocus();
}




void ControllerRackView::closeEvent( QCloseEvent * _ce )
 {
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	_ce->ignore();
 }

