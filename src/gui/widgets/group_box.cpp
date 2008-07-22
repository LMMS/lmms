#ifndef SINGLE_SOURCE_COMPILE

/*
 * group_box.cpp - groupbox for LMMS
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>


#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>


#include "group_box.h"
#include "embed.h"
#include "gui_templates.h"


QPixmap * groupBox::s_ledBg = NULL;


groupBox::groupBox( const QString & _caption, QWidget * _parent ) :
	QWidget( _parent ),
	boolModelView( NULL, this ),
	m_caption( _caption )
{
	if( s_ledBg == NULL )
	{
		s_ledBg = new QPixmap( embed::getIconPixmap(
							"groupbox_led_bg" ) );
	}

	updatePixmap();

	m_led = new pixmapButton( this, _caption );
	m_led->setCheckable( TRUE );
	m_led->move( 2, 3 );
	m_led->setActiveGraphic( embed::getIconPixmap( "led_green" ) );
	m_led->setInactiveGraphic( embed::getIconPixmap( "led_off" ) );

	setModel( new boolModel( FALSE, NULL, _caption, TRUE ) );
	setAutoFillBackground( TRUE );
	unsetCursor();
}




groupBox::~groupBox()
{
	delete m_led;
}




void groupBox::modelChanged( void )
{
	m_led->setModel( model() );
}




void groupBox::mousePressEvent( QMouseEvent * _me )
{
	if( _me->y() > 1 && _me->y() < 13 && _me->button() == Qt::LeftButton )
	{
		model()->setValue( !model()->value() );
	}
}




void groupBox::resizeEvent( QResizeEvent * _ev )
{
	updatePixmap();
	QWidget::resizeEvent( _ev );
}



void groupBox::updatePixmap( void )
{
	const int c = 0;
	QColor bg_color = QApplication::palette().color( QPalette::Active,
							QPalette::Background );
	QPixmap pm( size() );
	pm.fill( bg_color.dark( 132 ) );

	QPainter p( &pm );

	// outer rect
	p.setPen( bg_color.dark( 200 ) );
	p.drawRect( 0, 0, width() - 1 + c, height() - 1 + c );

	// brighter line at bottom/right
	p.setPen( bg_color.light( 125 ) );
	p.drawLine( width() - 1, 0, width() - 1, height() - 1 );
	p.drawLine( 0, height() - 1, width() - 1, height() - 1 );

	// draw our led-pixmap
	p.drawPixmap( 2, 2, *s_ledBg );

	// draw groupbox-titlebar
	p.fillRect( 2, 2, width() - 4, 9, bg_color.dark( 300 ) );

	// draw line below titlebar
	p.setPen( bg_color.dark( 400 ) );
	p.drawLine( 2 + s_ledBg->width(), 11, width() - 3, 11 );

	// black inner rect
	p.drawRect( 1, 1, width() - 3 + c, height() - 3 + c );


	//p.setPen( QColor( 255, 255, 255 ) );
	p.setPen( palette().color( QPalette::Active, QPalette::ButtonText ) );
	p.setFont( pointSize<7>( font() ) );
	p.drawText( 22, 10, m_caption );

	QPalette pal = palette();
	pal.setBrush( backgroundRole(), QBrush( pm ) );
	setPalette( pal );
}




#include "moc_group_box.cxx"


#endif
