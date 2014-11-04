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

#include <QtGui/QApplication>
#include <QtGui/QLayout>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMdiArea>
#include <QMessageBox>

#include "song.h"
#include "embed.h"
#include "MainWindow.h"
#include "group_box.h"
#include "ControllerRackView.h"
#include "ControllerView.h"
#include "LfoController.h"


ControllerRackView::ControllerRackView( ) :
	QWidget()
{
	setMinimumWidth( 250 );
	setMaximumWidth( 250 );
	resize( 250, 160 );

	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setWindowTitle( tr( "Controller Rack" ) );

	m_scrollArea = new QScrollArea( this );
	m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
	m_scrollArea->setMinimumHeight( 64 );

	m_addButton = new QPushButton( this );
	m_addButton->setText( tr( "Add" ) );

	QWidget * w = new QWidget();
	m_scrollArea->setWidget( w );

	connect( m_addButton, SIGNAL( clicked() ),
			this, SLOT( addController() ) );

	connect( engine::getSong(), SIGNAL( dataChanged() ),
			this, SLOT( update() ) );

	QVBoxLayout * layout = new QVBoxLayout();
	layout->addWidget( m_scrollArea );
	layout->addWidget( m_addButton );
	this->setLayout( layout );

	QMdiSubWindow * subWin =
			engine::mainWindow()->workspace()->addSubWindow( this );

	// No maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );
	
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );
	parentWidget()->move( 880, 310 );
}




ControllerRackView::~ControllerRackView()
{
	// delete scroll-area with all children
	delete m_scrollArea;
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

	int connectionCount = c->connectionCount();
	if( connectionCount > 0 )
	{
		QMessageBox msgBox;
		msgBox.setIcon( QMessageBox::Question );
		msgBox.setWindowTitle( tr("Confirm Delete") );
		msgBox.setText( tr("Confirm delete? There are existing connection(s) "
				"associted with this controller. There is no way to undo.") );
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		if( msgBox.exec() != QMessageBox::Ok )
		{
			return;
		}
	}


	m_controllerViews.erase( qFind( m_controllerViews.begin(),
				m_controllerViews.end(), _view ) );
	delete _view;
	delete c;
	update();
}




void ControllerRackView::update()
{
	QWidget * w = m_scrollArea->widget();
	song * s = engine::getSong();

	setUpdatesEnabled( false );

	int i = 0;
	for( i = 0; i < m_controllerViews.size(); ++i )
	{
		delete m_controllerViews[i];
	}

	m_controllerViews.clear();

	for( i = 0; i < s->m_controllers.size(); ++i )
	{
		ControllerView * v = new ControllerView( s->m_controllers[i], w );

		connect( v, SIGNAL( deleteController( ControllerView * ) ),
			this, SLOT( deleteController( ControllerView * ) ),
						Qt::QueuedConnection );

		m_controllerViews.append( v );
		v->move( 0, i*32 );
		v->show();
	}

	w->setFixedSize( 210, i*32 );

	setUpdatesEnabled( true );
	QWidget::update();
}


void ControllerRackView::addController()
{
	// TODO: Eventually let the user pick from available controller types

	engine::getSong()->addController( new LfoController( engine::getSong() ) );
	update();

	// fix bug which always made ControllerRackView loose focus when adding
	// new controller
	setFocus();
}



#include "moc_ControllerRackView.cxx"

