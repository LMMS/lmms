#ifndef SINGLE_SOURCE_COMPILE

/*
 * controller_rack_view.cpp - view for song's controllers
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
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
	QWidget()
{
	setFixedSize( 250, 250 );
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setWindowTitle( tr( "Controller Rack" ) );

	m_scrollArea = new QScrollArea( this );
	m_scrollArea->setFixedSize( 230, 184 );
	m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_scrollArea->setPalette( QApplication::palette( m_scrollArea ) );
	m_scrollArea->move( 6, 22 );

	m_addButton = new QPushButton( this/*, "Add Effect"*/ );
	m_addButton->setText( tr( "Add" ) );
	m_addButton->move( 75, 210 );
	connect( m_addButton, SIGNAL( clicked( void ) ), 
			this, SLOT( addController( void ) ) );

	connect( engine::getSong(), SIGNAL( dataChanged( void ) ),
			this, SLOT( update( void ) ) );

	QWidget * w = new QWidget();
	m_scrollArea->setWidget( w );

	m_lastY = 0;

	QMdiSubWindow * subWin = 
		engine::getMainWindow()->workspace()->addSubWindow( this );
	Qt::WindowFlags flags = subWin->windowFlags();
	flags |= Qt::MSWindowsFixedSizeDialogHint;
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );
	setWindowFlags( flags );
	subWin->layout()->setSizeConstraint( QLayout::SetFixedSize );

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
//	QVector<bool> view_map( fxChain()->m_effects.size(), FALSE );

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
	/*
	effectSelectDialog esd( this );
	esd.exec();

	if( esd.result() == QDialog::Rejected )
	{
		return;
	}
	*/

	engine::getSong()->addController( new lfoController( engine::getSong() ) );
	update();
}



#include "moc_controller_rack_view.cxx"

#endif
