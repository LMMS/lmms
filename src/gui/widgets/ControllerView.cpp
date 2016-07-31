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

#include "ControllerView.h"

#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPainter>
#include <QPushButton>
#include <QWhatsThis>


#include "CaptionMenu.h"
#include "ControllerDialog.h"
#include "embed.h"
#include "Engine.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "LedCheckbox.h"
#include "MainWindow.h"
#include "Song.h"
#include "ToolTip.h"


ControllerView::ControllerView( Controller * _model, QWidget * _parent ) :
	QFrame( _parent ),
	ModelView( _model, this ),
	m_controllerDlg( NULL ),
	m_show( true ),
	m_titleBarHeight( 24 )
{
	const QSize buttonsize( 17, 17 );
	setFrameStyle( QFrame::Plain );
	setFrameShadow( QFrame::Plain );
	setLineWidth( 0 );
	setContentsMargins( 0, 0, 0, 0 );
	setWhatsThis( tr( "Controllers are able to automate the value of a knob, "
	"slider, and other controls."  ) );
		
	m_controllerDlg = getController()->createDialog( this );
	m_controllerDlg->move( 1, m_titleBarHeight );
	
	m_nameLineEdit = new QLineEdit( this );
	m_nameLineEdit->setText( _model->name() );
	m_nameLineEdit->setReadOnly( true );
	m_nameLineEdit->setAttribute( Qt::WA_TransparentForMouseEvents );
	m_nameLineEdit->move( 3, 3 );
	// REMOVE THIS IF "rename FxLine directly in QLineEdit" IS MERGED!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	m_nameLineEdit->setStyleSheet( "border-style: none; "
								   "background: transparent; " );
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	connect( m_nameLineEdit, SIGNAL( editingFinished() ), this, SLOT( renameFinished() ) );
	setFixedWidth( m_controllerDlg->width() + 2 );
	setFixedHeight( m_controllerDlg->height() + m_titleBarHeight + 1 );
	
	m_collapse = new QPushButton( embed::getIconPixmap( "stepper-down" ), QString::null, this );
	m_collapse->resize( buttonsize );
	m_collapse->setFocusPolicy( Qt::NoFocus );
	m_collapse->setCursor( Qt::ArrowCursor );
	m_collapse->setAttribute( Qt::WA_NoMousePropagation );
	m_collapse->setToolTip( tr( "collapse" ) );
	m_collapse->move( width() - buttonsize.width() - 3, 3 );
	connect( m_collapse, SIGNAL( clicked() ), this, SLOT( collapseController() ) );
		
	setModel( _model );
}




ControllerView::~ControllerView()
{
}




void ControllerView::deleteController()
{
	emit( deleteController( this ) );
}




void ControllerView::collapseController()
{
	if( m_controllerDlg->isHidden() )
	{
		m_controllerDlg->show();
		setFixedHeight( m_controllerDlg->height() + m_titleBarHeight + 1 );
		m_collapse->setIcon( embed::getIconPixmap( "stepper-down" ) );
	}
	else
	{
		m_collapse->setIcon( embed::getIconPixmap( "stepper-left" ) );
		m_controllerDlg->hide();
		setFixedHeight( m_titleBarHeight );
	}
}




void ControllerView::renameFinished()
{
	m_nameLineEdit->setReadOnly( true );
	Controller * c = castModel<Controller>();
	QString new_name = m_nameLineEdit->text();
	if( new_name != c->name() )
	{
		c->setName( new_name );
		Engine::getSong()->setModified();
	}
}




void ControllerView::rename()
{
	m_nameLineEdit->setReadOnly( false );
	m_nameLineEdit->setFocus();
	m_nameLineEdit->selectAll();
}




void ControllerView::mouseDoubleClickEvent( QMouseEvent * event )
{
	rename();
}




void ControllerView::modelChanged()
{
}




void ControllerView::paintEvent(QPaintEvent* event)
{
	QPainter p( this );
	QRect rect( 1, 1, width()-2, m_titleBarHeight );
	p.fillRect( rect, p.background() );
}




void ControllerView::contextMenuEvent( QContextMenuEvent * )
{
	QPointer<CaptionMenu> contextMenu = new CaptionMenu( model()->displayName(), this );
	contextMenu->addAction( embed::getIconPixmap( "cancel" ), tr( "&Remove this plugin" ), this, SLOT( deleteController() ) );
	contextMenu->addSeparator();
	contextMenu->addHelpAction();
	contextMenu->exec( QCursor::pos() );
	delete contextMenu;
}




void ControllerView::displayHelp()
{
	QWhatsThis::showText( mapToGlobal( rect().center() ), whatsThis() );
}
