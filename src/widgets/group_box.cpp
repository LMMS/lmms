/*
 * group_box.cpp - groupbox for LMMS
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QTimer>
#include <QMouseEvent>

#else

#include <qpainter.h>
#include <qtimer.h>
#include <qobjectlist.h>

#define setChecked setOn

#endif

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>


#include "group_box.h"
#include "embed.h"
#include "spc_bg_hndl_widget.h"
#include "gui_templates.h"


QPixmap * groupBox::s_ledBg = NULL;


groupBox::groupBox( const QString & _caption, QWidget * _parent ) :
	QWidget( _parent ),
	m_caption( _caption ),
	m_origHeight( height() ),
	m_animating( FALSE )
{
	if( s_ledBg == NULL )
	{
		s_ledBg = new QPixmap( embed::getIconPixmap(
							"groupbox_led_bg" ) );
	}
	updatePixmap();

	m_led = new pixmapButton( this );
	m_led->move( 2, 3 );
	m_led->setActiveGraphic( embed::getIconPixmap( "led_green" ) );
	m_led->setInactiveGraphic( embed::getIconPixmap( "led_off" ) );
	m_led->setBgGraphic( specialBgHandlingWidget::getBackground( m_led ) );
	connect( m_led, SIGNAL( toggled( bool ) ),
					this, SLOT( setState( bool ) ) );
}




groupBox::~groupBox()
{
}




void groupBox::resizeEvent( QResizeEvent * )
{
	updatePixmap();
	if( m_animating == FALSE )
	{
		m_origHeight = height();
	}
}




void groupBox::mousePressEvent( QMouseEvent * _me )
{
	if( _me->y() > 1 && _me->y() < 13 )
	{
		setState( !isActive(), TRUE );
	}
}




void groupBox::setState( bool _on, bool _anim )
{
	m_led->setChecked( _on );
	if( ( _anim == TRUE || ( _on == TRUE && height() < m_origHeight ) ) &&
							m_animating == FALSE )
	{
		m_animating = TRUE;
		animate();
	}
}




void groupBox::animate( void )
{
	float state = (float)( m_origHeight - height() ) /
						(float)( m_origHeight - 19 );
	int dy = static_cast<int>( 3 - 2 * cosf( state * 2 * M_PI ) );
	if( isActive() && height() < m_origHeight )
	{
	}
	else if( !isActive() && height() > 19 )
	{
		dy = -dy;
	}
	else
	{
		m_animating = FALSE;
		return;
	}
	resize( width(), height() + dy );
	QTimer::singleShot( 10, this, SLOT( animate() ) );
#ifdef QT4
	QObjectList ch = parent()->children();
#else
	QObjectList ch = *( parent()->children() );
#endif
	for( csize i = 0; i < ch.count(); ++i )
	{
		QWidget * w = dynamic_cast<QWidget *>( ch.at( i ) );
		if( w == NULL || w->y() < y() + height() )
		{
			continue;
		}
		w->move( w->x(), w->y() + dy );
	}
#ifdef QT4
	ch = children();
#else
	ch = *children();
#endif
	for( csize i = 0; i < ch.count(); ++i )
	{
		QWidget * w = dynamic_cast<QWidget *>( ch.at( i ) );
		if( w == NULL || w == m_led )
		{
			continue;
		}
		w->move( w->x(), w->y() + dy );
		if( w->y() < 14)
		{
			w->hide();
		}
		else if( w->isHidden() == TRUE )
		{
			w->show();
		}
	}
}




void groupBox::updatePixmap( void )
{
	QPixmap pm( size() );
	pm.fill( QColor( 96, 96, 96 ) );

	QPainter p( &pm );

	// outer rect
	p.setPen( QColor( 64, 64, 64 ) );
	p.drawRect( 0, 0, width(), height() );

	// brighter line at bottom/right
	p.setPen( QColor( 160, 160, 160 ) );
	p.drawLine( width() - 1, 0, width() - 1, height() - 1 );
	p.drawLine( 0, height() - 1, width() - 1, height() - 1 );

	// draw our led-pixmap
	p.drawPixmap( 2, 2, *s_ledBg );

	// draw groupbox-titlebar
	p.fillRect( 2, 2, width() - 4, 9, QColor( 30, 45, 60 ) );

	// draw line below titlebar
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawLine( 2 + s_ledBg->width(), 11, width() - 3, 11 );

	// black inner rect
	p.drawRect( 1, 1, width() - 2, height() - 2 );


	p.setPen( QColor( 255, 255, 255 ) );
	p.setFont( pointSize<7>( font() ) );
	p.drawText( 22, 10, m_caption );

#ifdef QT4
	QPalette pal = palette();
	pal.setBrush( backgroundRole(), QBrush( pm ) );
	pal.setColor( QPalette::Background, QColor( 96, 96, 96 ) );
	setPalette( pal );
#else
	setPaletteBackgroundColor( QColor( 96, 96, 96 ) );
	setErasePixmap( pm );
#endif
}



#undef setChecked

#include "group_box.moc"

