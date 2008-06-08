#ifndef SINGLE_SOURCE_COMPILE

/*
 * lfo_controller_dialog.cpp - per-controller-specific view for changing a
 * controller's settings
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


#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QMdiArea>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>

#include "caption_menu.h"
#include "gui_templates.h"
#include "embed.h"
#include "engine.h"
#include "led_checkbox.h"
#include "main_window.h"
#include "tooltip.h"


#include "peak_controller.h"
#include "controller_dialog.h"
#include "mv_base.h"
#include "knob.h"
#include "tempo_sync_knob.h"
#include "pixmap_button.h"

peakControllerDialog::peakControllerDialog( controller * _model, QWidget * _parent ) :
	controllerDialog( _model, _parent )
{
	setWindowTitle( tr( "PEAK" ) );
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setFixedSize( 256, 64 );
	
	toolTip::add( this, tr( "LFO Controller" ) );

	QLabel * l = new QLabel( this );
	l->setText( "Use FX's controls" );
	l->move(10, 10);

	setModel( _model );
}




peakControllerDialog::~peakControllerDialog()
{
}



/*
void effectView::displayHelp( void )
{
	QWhatsThis::showText( mapToGlobal( rect().bottomRight() ),
								whatsThis() );
}




void effectView::closeEffects( void )
{
	m_subWindow->hide();
	m_show = TRUE;
}
*/


void peakControllerDialog::contextMenuEvent( QContextMenuEvent * )
{
}




void peakControllerDialog::paintEvent( QPaintEvent * )
{
	QPainter p( this );
}



void peakControllerDialog::modelChanged( void )
{
	m_peakController = castModel<peakController>();
}


#endif
