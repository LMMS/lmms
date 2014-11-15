/*
 * ControllerView.cpp - view-component for an controller
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2011-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>
#include <QtGui/QInputDialog>
#include <QtGui/QWhatsThis>

#include "ControllerView.h"

#include "caption_menu.h"
#include "ControllerDialog.h"
#include "gui_templates.h"
#include "embed.h"
#include "engine.h"
#include "led_checkbox.h"
#include "MainWindow.h"
#include "tooltip.h"


ControllerView::ControllerView( Controller * _model, QWidget * _parent ) :
	QWidget( _parent ),
	ModelView( _model, this ),
	m_bg( embed::getIconPixmap( "controller_bg" ) ),
	m_subWindow( NULL ),
	m_controllerDlg( NULL ),
	m_show( true )
{
	setFixedSize( 210, 32 );

	QPushButton * ctls_btn = new QPushButton( tr( "Controls" ), this );
	
	QFont f = ctls_btn->font();
	ctls_btn->setFont( pointSize<8>( f ) );
	ctls_btn->setGeometry( 140, 2, 50, 14 );
	connect( ctls_btn, SIGNAL( clicked() ), 
				this, SLOT( editControls() ) );

	m_controllerDlg = getController()->createDialog( engine::mainWindow()->workspace() );

	m_subWindow = engine::mainWindow()->workspace()->addSubWindow( 
                m_controllerDlg );
	
	Qt::WindowFlags flags = m_subWindow->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	m_subWindow->setWindowFlags( flags );
	m_subWindow->setFixedSize( m_subWindow->size() );

	m_subWindow->setWindowIcon( m_controllerDlg->windowIcon() );

	connect( m_controllerDlg, SIGNAL( closed() ),
		this, SLOT( closeControls() ) );

	m_subWindow->hide();

	setWhatsThis( tr( "Controllers are able to automate the value of a knob, "
				"slider, and other controls."  ) );

	setModel( _model );
}




ControllerView::~ControllerView()
{
	delete m_subWindow;
}




void ControllerView::editControls()
{
	if( m_show )
	{
		m_subWindow->show();
		m_subWindow->raise();
		m_show = false;
	}
	else
	{
		m_subWindow->hide();
		m_show = true;
	}
}




void ControllerView::closeControls()
{
	m_subWindow->hide();
	m_show = true;
}


void ControllerView::deleteController()
{
	emit( deleteController( this ) );
}



void ControllerView::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.drawPixmap( 0, 0, m_bg );

	QFont f = pointSizeF( font(), 7.5f );
	f.setBold( true );
	p.setFont( f );

	Controller * c = castModel<Controller>();

	p.setPen( QColor( 64, 64, 64 ) );
	p.drawText( 7, 13, c->displayName() );
	p.setPen( Qt::white );
	p.drawText( 6, 12, c->displayName() );

	f.setBold( false );
	p.setFont( f );
	p.drawText( 8, 26, c->name() );
}



void ControllerView::mouseDoubleClickEvent( QMouseEvent * event )
{
	bool ok;
	Controller * c = castModel<Controller>();
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



void ControllerView::modelChanged()
{
}



void ControllerView::contextMenuEvent( QContextMenuEvent * )
{
	QPointer<captionMenu> contextMenu = new captionMenu( model()->displayName(), this );
	contextMenu->addAction( embed::getIconPixmap( "cancel" ),
						tr( "&Remove this plugin" ),
						this, SLOT( deleteController() ) );
	contextMenu->addSeparator();
	contextMenu->addHelpAction();
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
}



void ControllerView::displayHelp()
{
	QWhatsThis::showText( mapToGlobal( rect().center() ),
								whatsThis() );
}



#include "moc_ControllerView.cxx"

