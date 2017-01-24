/*
 * ControllerRackView.cpp - view for song's controllers
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2010-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include "ControllerRackView.h"

#include <QApplication>
#include <QLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

#include "ControllerView.h"
#include "embed.h"
#include "GroupBox.h"
#include "GuiApplication.h"
#include "LfoController.h"
#include "MainWindow.h"
#include "Song.h"


ControllerRackView::ControllerRackView( ) :
	QWidget(),
	m_nextIndex(0)
{
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setWindowTitle( tr( "Controller Rack" ) );

	m_scrollArea = new QScrollArea( this );
	m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
	m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_scrollArea->setFrameStyle( QFrame::Plain );
	m_scrollArea->setFrameShadow( QFrame::Plain );


	QWidget * scrollAreaWidget = new QWidget( m_scrollArea );
	m_scrollAreaLayout = new QVBoxLayout( scrollAreaWidget );
	m_scrollAreaLayout->addStretch();
	m_scrollAreaLayout->setMargin( 0 );
	m_scrollAreaLayout->setSpacing( 0 );
	scrollAreaWidget->setLayout( m_scrollAreaLayout );

	m_scrollArea->setWidget( scrollAreaWidget );
	m_scrollArea->setWidgetResizable( true );

	m_addButton = new QPushButton( this );
	m_addButton->setText( tr( "Add LFO" ) );

	connect( m_addButton, SIGNAL( clicked() ), this, SLOT( addController() ) );

	Song * song = Engine::getSong();
	connect( song, SIGNAL( controllerAdded( Controller* ) ), SLOT( onControllerAdded( Controller* ) ) );
	connect( song, SIGNAL( controllerRemoved( Controller* ) ), SLOT( onControllerRemoved( Controller* ) ) );

	QVBoxLayout * layout = new QVBoxLayout();
	layout->addWidget( m_scrollArea );
	layout->addWidget( m_addButton );
	layout->setMargin( 0 );
	setLayout( layout );

	m_subWin = gui->mainWindow()->addWindowedWidget( this );

	// No maximize button
	Qt::WindowFlags flags = m_subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	m_subWin->setWindowFlags( flags );

	m_subWin->setAttribute( Qt::WA_DeleteOnClose, false );
	m_subWin->move( 680, 60 );
	m_subWin->resize( 400, 249 );
	m_subWin->setFixedWidth( 249 );
}




ControllerRackView::~ControllerRackView()
{
}




void ControllerRackView::saveSettings( QDomDocument & _doc,	QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this, QSize( 400, 300 ) );
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




void ControllerRackView::onControllerAdded( Controller * controller )
{
	QWidget * scrollAreaWidget = m_scrollArea->widget();

	ControllerView * controllerView = new ControllerView( controller, scrollAreaWidget );
	connect( controllerView, SIGNAL( deleteController( ControllerView * ) ), this, SLOT( deleteController( ControllerView * ) ), Qt::QueuedConnection );
	connect( controllerView, SIGNAL( controllerCollapsed() ), this, SLOT( onControllerCollapsed() ) );
	m_controllerViews.append( controllerView );
	m_scrollAreaLayout->insertWidget( m_nextIndex, controllerView );

	++m_nextIndex;
	update();
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

	if ( viewOfRemovedController )
	{
		m_controllerViews.erase( qFind( m_controllerViews.begin(),
					m_controllerViews.end(), viewOfRemovedController ) );

		delete viewOfRemovedController;
		--m_nextIndex;
		m_scrollArea->verticalScrollBar()->hide();
		update();
	}
}




void ControllerRackView::onControllerCollapsed()
{
	m_scrollArea->verticalScrollBar()->hide();
	update();
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




void ControllerRackView::resizeEvent( QResizeEvent *re )
{
	m_subWin->setFixedWidth( m_scrollArea->verticalScrollBar()->isVisible() ? 262 : 249 );
}




void ControllerRackView::paintEvent( QPaintEvent *pe )
{
	m_subWin->setFixedWidth( m_scrollArea->verticalScrollBar()->isVisible() ? 262 : 249 );
	m_scrollArea->verticalScrollBar()->show();
}
