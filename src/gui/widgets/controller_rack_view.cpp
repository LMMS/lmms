/*
 * controller_rack_view.cpp - view for song's controllers
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
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


#include <QtGui/QApplication>
#include <QtGui/QLayout>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMdiArea>

#include "song.h"
#include "embed.h"
#include "main_window.h"
#include "group_box.h"
#include "controller_rack_view.h"
#include "controller_view.h"
#include "lfo_controller.h"


controllerRackView::controllerRackView( ) :
	QWidget(),
	m_lastY( 0 )
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

	connect( m_addButton, SIGNAL( clicked( void ) ),
			this, SLOT( addController( void ) ) );

	connect( engine::getSong(), SIGNAL( dataChanged( void ) ),
			this, SLOT( update( void ) ) );

	QVBoxLayout * layout = new QVBoxLayout();
	layout->addWidget( m_scrollArea );
	layout->addWidget( m_addButton );
	setLayout( layout );

	QMdiSubWindow * subWin =
			engine::getMainWindow()->workspace()->addSubWindow( this );

	// No maximize button
	Qt::WindowFlags flags = subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );
	

	subWin->layout()->setSizeConstraint( QLayout::SetMaximumSize );

	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, FALSE );
	parentWidget()->move( 880, 310 );
}




controllerRackView::~controllerRackView()
{
	// delete scroll-area with all children
	delete m_scrollArea;
}




void controllerRackView::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	mainWindow::saveWidgetState( this, _this );
}




void controllerRackView::loadSettings( const QDomElement & _this )
{
	mainWindow::restoreWidgetState( this, _this );
}




void controllerRackView::deleteController( controllerView * _view )
{
	
	controller * c = _view->getController();
	m_controllerViews.erase( qFind( m_controllerViews.begin(),
				m_controllerViews.end(), _view ) );
	delete _view;
	delete c;
	update();
}




void controllerRackView::update( void )
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
		controllerView * v = new controllerView( s->m_controllers[i], w );

		connect( v, SIGNAL( deleteController( controllerView * ) ),
			this, SLOT( deleteController( controllerView * ) ),
						Qt::QueuedConnection );

		m_controllerViews.append( v );
		v->move( 0, i*32 );
		v->show();
	}

	w->setFixedSize( 210, i*32 );

	setUpdatesEnabled( true );
	QWidget::update();
}


void controllerRackView::addController( void )
{
	// TODO: Eventually let the user pick from available controller types

	engine::getSong()->addController( new lfoController( engine::getSong() ) );
	update();
}



#include "moc_controller_rack_view.cxx"

