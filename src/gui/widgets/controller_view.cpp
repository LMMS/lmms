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
#include <QtGui/QInputDialog>

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
	QWidget( _parent ),
	modelView( _model, this ),
	m_bg( embed::getIconPixmap( "controller_bg" ) ),
	m_subWindow( NULL ),
	m_controllerDlg( NULL ),
	m_show( TRUE )
{
	setFixedSize( 210, 32 );

	setAttribute( Qt::WA_OpaquePaintEvent, TRUE );

	QPushButton * ctls_btn = new QPushButton( tr( "Controls" ),
	            this );
	
	QFont f = ctls_btn->font();
	ctls_btn->setFont( pointSize<7>( f ) );
	ctls_btn->setGeometry( 140, 2, 50, 14 );
	connect( ctls_btn, SIGNAL( clicked() ), 
				this, SLOT( editControls() ) );

	m_controllerDlg = getController()->createDialog( engine::getMainWindow()->workspace() );

	m_subWindow = engine::getMainWindow()->workspace()->addSubWindow( 
                m_controllerDlg );
	
	Qt::WindowFlags flags = m_subWindow->windowFlags();
	flags |= Qt::MSWindowsFixedSizeDialogHint;
	flags &= ~Qt::WindowMaximizeButtonHint;
	m_subWindow->setWindowFlags( flags );
	m_subWindow->setFixedSize( m_subWindow->size() );

	m_subWindow->setWindowIcon( m_controllerDlg->windowIcon() );

    connect( m_controllerDlg, SIGNAL( closed() ),
                this, SLOT( closeControls() ) );

	m_subWindow->hide();


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




void controllerView::closeControls( void )
{
	m_subWindow->hide();
	m_show = TRUE;
}




void controllerView::contextMenuEvent( QContextMenuEvent * )
{
}




void controllerView::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.drawPixmap( 0, 0, m_bg );

	QFont f = pointSizeF( font(), 7.5f );
	f.setBold( TRUE );
	p.setFont( f );

	controller * c = castModel<controller>();

	p.setPen( QColor( 64, 64, 64 ) );
	p.drawText( 7, 13, c->publicName() );
	p.setPen( Qt::white );
	p.drawText( 6, 12, c->publicName() );

    f.setBold( FALSE );
    p.setFont( f );
	p.drawText( 8, 26, c->name() );
}



void controllerView::mouseDoubleClickEvent( QMouseEvent * event )
{
	bool ok;
	controller * c = castModel<controller>();
	QString new_name = QInputDialog::getText( this,
			tr( "Rename controller" ),
			tr( "Enter the new name for this controller" ),
			QLineEdit::Normal, c->name() , &ok );
	if( ok && !new_name.isEmpty() )
	{
		c->setName( new_name );
		update();
	}
}



void controllerView::modelChanged( void )
{
}


#include "controller_view.moc"

#endif
