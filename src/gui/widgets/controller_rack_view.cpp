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
	modelView( NULL )
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

	setModel( engine::getSong() );

	engine::getMainWindow()->workspace()->addSubWindow( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, FALSE );
}




controllerRackView::~controllerRackView()
{
	clear();
}





void controllerRackView::clear( void )
{
/*	for( QVector<effectView *>::iterator it = m_controllerViews.begin();
					it != m_controllerViews.end(); ++it )
	{
		delete *it;
	}
	m_controllerViews.clear();*/
}



/*
void controllerRackView::moveUp( controllerView * _view )
{
	fxChain()->moveUp( _view->getEffect() );
	if( _view != m_effectViews.first() )
	{
		int i = 0;
		for( QVector<controllerView *>::iterator it = 
						m_effectViews.begin(); 
					it != m_effectViews.end(); it++, i++ )
		{
			if( *it == _view )
			{
				break;
			}
		}
		
		effectView * temp = m_effectViews[ i - 1 ];
		
		m_effectViews[i - 1] = _view;
		m_effectViews[i] = temp;
		
		update();
	}
}*/



/*
void controllerRackView::moveDown( effectView * _view )
{
	if( _view != m_effectViews.last() )
	{
		// moving next effect up is the same
		moveUp( *( qFind( m_effectViews.begin(), m_effectViews.end(),
							_view ) + 1 ) );
	}
}*/


/*

void controllerRackView::deletePlugin( effectView * _view )
{
	
	effect * e = _view->getEffect();
	m_effectViews.erase( qFind( m_effectViews.begin(), m_effectViews.end(),
								_view ) );
	delete _view;
	fxChain()->m_effects.erase( qFind( fxChain()->m_effects.begin(),
						fxChain()->m_effects.end(),
									e ) );
	delete e;
	update();
	
}
*/



void controllerRackView::update( void )
{
	QWidget * w = m_scrollArea->widget();
	song * s = engine::getSong();
//	QVector<bool> view_map( fxChain()->m_effects.size(), FALSE );

	setUpdatesEnabled( false );

	int i = 0;
	for( i = 0; i < m_controllerViews.size(); ++i )
	{
		delete  m_controllerViews[i];
	}

	m_controllerViews.clear();

	for( i = 0; i < s->m_controllers.size(); ++i )
	{
		controllerView * v = new controllerView( s->m_controllers[i], w );
		m_controllerViews.append( v );
		v->move( 0, i*32 );
		v->show();
	}

	w->setFixedSize( 210, i*32 );

	/*
	for( QVector<effect *>::iterator it = fxChain()->m_effects.begin();
					it != fxChain()->m_effects.end(); ++it )
	{
		int i = 0;
		for( QVector<effectView *>::iterator vit =
							m_effectViews.begin();
				vit != m_effectViews.end(); ++vit, ++i )
		{
			if( ( *vit )->getEffect() == *it )
			{
				view_map[i] = TRUE;
				break;
			}
		}
		if( i >= m_effectViews.size() )
		{
			effectView * view = new effectView( *it, w );
			connect( view, SIGNAL( moveUp( effectView * ) ), 
					this, SLOT( moveUp( effectView * ) ) );
			connect( view, SIGNAL( moveDown( effectView * ) ),
				this, SLOT( moveDown( effectView * ) ) );
			connect( view, SIGNAL( deletePlugin( effectView * ) ),
				this, SLOT( deletePlugin( effectView * ) ) );
			view->show();
			m_effectViews.append( view );
			view_map[i] = TRUE;

		}
	}

	int i = m_lastY = 0;
	for( QVector<effectView *>::iterator it = m_effectViews.begin(); 
					it != m_effectViews.end(); )
	{
		if( i < view_map.size() && i < m_effectViews.size() &&
							view_map[i] == FALSE )
		{
			delete m_effectViews[i];
			m_effectViews.erase( it );
		}
		else
		{
			( *it )->move( 0, m_lastY );
			m_lastY += ( *it )->height();
			++it;
			++i;
		}
	}
	w->setFixedSize( 210, m_lastY );
*/
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


/*

void effectRackView::modelChanged( void )
{
	clear();
	m_effectsGroupBox->setModel( &fxChain()->m_enabledModel );
	update();
}
*/


#include "controller_rack_view.moc"

#endif
