#ifndef SINGLE_SOURCE_COMPILE

/*
 * controller_view.cpp - view-component for an controller
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


#include "controller_view.h"

#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>

#include "caption_menu.h"
#include "controller_dialog.h"
#include "gui_templates.h"
#include "embed.h"
#include "engine.h"
#include "led_checkbox.h"
#include "main_window.h"
#include "tooltip.h"
#include "mv_base.h"


controllerView::controllerView( controller * _model, QWidget * _parent ) :
	modelView( _model ),
	QWidget( _parent ),
	m_bg( embed::getIconPixmap( "controller_bg" ) ),
	m_show( TRUE ),
	m_subWindow( NULL ),
	m_controllerDlg( NULL )
{
	setFixedSize( 210, 20 );

	setAttribute( Qt::WA_OpaquePaintEvent, TRUE );

	m_bypass = new ledCheckBox( "", this, tr( "Turn the controller off" ) );
	m_bypass->move( 3, 3 );
	m_bypass->setWhatsThis( tr( "Toggles the controller on or off." ) );
	toolTip::add( m_bypass, tr( "On/Off" ) );
	
	QPushButton * ctls_btn = new QPushButton( tr( "Controls" ),
	            this );
	
    QFont f = ctls_btn->font();
	ctls_btn->setFont( pointSize<7>( f ) );
	ctls_btn->setGeometry( 140, 2, 50, 14 );
	connect( ctls_btn, SIGNAL( clicked() ), 
				this, SLOT( editControls() ) );

	m_controllerDlg = getController()->createDialog( NULL );

	m_subWindow = engine::getMainWindow()->workspace()->addSubWindow( 
                m_controllerDlg );

    connect( m_controllerDlg, SIGNAL( closed() ),
                this, SLOT( closeControls() ) );

	m_subWindow->hide();

/*
	if( getEffect()->getControls()->getControlCount() > 0 )
	{
		QPushButton * ctls_btn = new QPushButton( tr( "Controls" ),
									this );
		QFont f = ctls_btn->font();
		ctls_btn->setFont( pointSize<7>( f ) );
		ctls_btn->setGeometry( 140, 14, 50, 20 );
		connect( ctls_btn, SIGNAL( clicked() ), 
					this, SLOT( editControls() ) );
	}


	m_controlView = getEffect()->getControls()->createView();
	connect( m_controlView, SIGNAL( closed() ),
				this, SLOT( closeEffects() ) );
*/

	setModel( _model );
}




controllerView::~controllerView()
{
	delete m_subWindow;
}




void controllerView::editControls( void )
{
	if( m_show )
	{
		m_subWindow->show();
		m_subWindow->raise();
		m_show = FALSE;
	}
	else
	{
		m_subWindow->hide();
		m_show = TRUE;
	}
}

/*
		//engine::getMainWindow()->workspace()->addSubWindow( NULL );

	controller * c = castModel<controller>();

	if( m_controllerDlg == NULL )
		m_controllerDlg = c->createDialog( NULL );

	m_subWindow = engine::getMainWindow()->workspace()->addSubWindow( m_controllerDlg );

	m_subWindow->show();
	m_subWindow->raise();

}
*/

/*
void effectView::displayHelp( void )
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
								whatsThis() );
}

*/


void controllerView::closeControls( void )
{
	m_subWindow->hide();
	m_show = TRUE;
}



void controllerView::contextMenuEvent( QContextMenuEvent * )
{
	/*
	QPointer<captionMenu> contextMenu = new captionMenu(
						getEffect()->publicName() );
	contextMenu->addAction( embed::getIconPixmap( "arp_up_on" ),
						tr( "Move &up" ),
						this, SLOT( moveUp() ) );
	contextMenu->addAction( embed::getIconPixmap( "arp_down_on" ),
						tr( "Move &down" ),
						this, SLOT( moveDown() ) );
	contextMenu->addSeparator();
	contextMenu->addAction( embed::getIconPixmap( "cancel" ),
						tr( "&Remove this plugin" ),
						this, SLOT( deletePlugin() ) );
	contextMenu->addSeparator();
	contextMenu->addAction( embed::getIconPixmap( "help" ),
						tr( "&Help" ),
						this, SLOT( displayHelp() ) );
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
	*/
}




void controllerView::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.drawPixmap( 0, 0, m_bg );

	QFont f = pointSizeF( font(), 7.5f );
	f.setBold( TRUE );
	p.setFont( f );

	p.setPen( QColor( 64, 64, 64 ) );
	p.drawLine(0,0,20,20);
	p.drawText( 6, 55, castModel<controller>()->publicName() );
	p.setPen( Qt::white );
	p.drawText( 5, 54, castModel<controller>()->publicName() );
}



void controllerView::modelChanged( void )
{
/*
	m_bypass->setModel( &getEffect()->m_enabledModel );
	m_wetDry->setModel( &getEffect()->m_wetDryModel );
	m_autoQuit->setModel( &getEffect()->m_autoQuitModel );
	m_gate->setModel( &getEffect()->m_gateModel );
	*/
}


#include "controller_view.moc"

#endif
