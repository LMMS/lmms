/*
 * group_box.cpp - groupbox for LMMS
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>

#include "group_box.h"
#include "embed.h"
#include "gui_templates.h"



groupBox::groupBox( const QString & _caption, QWidget * _parent ) :
	QWidget( _parent ),
	BoolModelView( NULL, this ),
	m_caption( _caption ),
	m_titleBarHeight( 11 )
{
	updatePixmap();

	m_led = new pixmapButton( this, _caption );
	m_led->setCheckable( true );
	m_led->move( 3, 0 );
	m_led->setActiveGraphic( embed::getIconPixmap( "led_green" ) );
	m_led->setInactiveGraphic( embed::getIconPixmap( "led_off" ) );

	setModel( new BoolModel( false, NULL, _caption, true ) );
	setAutoFillBackground( true );
	unsetCursor();
}




groupBox::~groupBox()
{
	delete m_led;
}




void groupBox::modelChanged()
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



void groupBox::updatePixmap()
{
	QColor bg_color = QApplication::palette().color( QPalette::Active,
							QPalette::Background );
	QPixmap pm( size() );
	pm.fill( bg_color/*.dark( 132 )*/ );

	QPainter p( &pm );

	// outer rect
	p.setPen( bg_color.dark( 150 ) );
	p.drawRect( 0, 0, width() - 1, height() - 1 );

	// brighter line at bottom/right
	p.setPen( bg_color.light( 150 ) );
	p.drawLine( width() - 1, 0, width() - 1, height() - 1 );
	p.drawLine( 0, height() - 1, width() - 1, height() - 1 );

	// draw groupbox-titlebar
	QLinearGradient g( 0, 0, 0, m_titleBarHeight );
	g.setColorAt( 0, bg_color.darker( 250 ) );
	g.setColorAt( 0.1, bg_color.lighter( 120 ) );
	g.setColorAt( 1, bg_color.darker( 250 ) );
	p.fillRect( 2, 2, width() - 4, m_titleBarHeight, g );

	// draw line below titlebar
	p.setPen( bg_color.dark( 400 ) );
	p.drawLine( 1, m_titleBarHeight + 1, width() - 3, m_titleBarHeight + 1 );

	// black inner rect
	p.drawRect( 1, 1, width() - 3, height() - 3 );


	//p.setPen( QColor( 255, 255, 255 ) );
	p.setPen( palette().color( QPalette::Active, QPalette::Text ) );
	p.setFont( pointSize<8>( font() ) );
	p.drawText( 22, m_titleBarHeight, m_caption );

	QPalette pal = palette();
	pal.setBrush( backgroundRole(), QBrush( pm ) );
	setPalette( pal );
}




#include "moc_group_box.cxx"


