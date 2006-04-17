#ifndef SINGLE_SOURCE_COMPILE

/*
 * fade_button.cpp - implementation of fade-button
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 

#include "fade_button.h"

#ifdef QT4

#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#else

#include <qtimer.h>
#include <qpainter.h>
#include <qpixmap.h>

#endif


fadeButton::fadeButton( const QColor & _normal_color,
			const QColor & _activated_color, QWidget * _parent ) :
	QAbstractButton( _parent ),
	m_state( 0.0f ),
	m_normalColor( _normal_color ),
	m_activatedColor( _activated_color )
{
#ifndef QT4
	setBackgroundMode( NoBackground );
#endif
}




fadeButton::~fadeButton()
{
}




void fadeButton::activate( void )
{
	if( m_state > 0.0f )
	{
		m_state = 1.00f;
	}
	else
	{
		m_state = 1.1f;
		nextState();
	}
	update();
}



void fadeButton::reset( void )
{
	m_state = 0.0f;
	update();
}





void fadeButton::paintEvent( QPaintEvent * _pe )
{
	QColor col = m_normalColor;
	if( m_state > 0.0f )
	{
		const int r = (int)( m_normalColor.red() *
					( 1.0f - m_state ) +
			m_activatedColor.red() * m_state );
		const int g = (int)( m_normalColor.green() *
					( 1.0f - m_state ) +
			m_activatedColor.green() * m_state );
		const int b = (int)( m_normalColor.blue() *
					( 1.0f - m_state ) +
			m_activatedColor.blue() * m_state );
		col.setRgb( r, g, b );
	}
#ifdef QT4
	QPainter p( this );
	p.fillRect( rect(), col );
#else
	QPixmap draw_pm( rect().size() );
	draw_pm.fill( col );

	QPainter p( &draw_pm, this );
#endif
	p.setPen( QColor( 0, 0, 0 ) );
#ifndef QT3
	p.drawRect( 0, 0, rect().right(), rect().bottom() );
#else
	p.drawRect( rect() );
#endif
#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
	if( m_state > 0.0f )
	{
		// we might be called out of another thread than the GUI-/
		// event-loop-thread, so let the timer update ourself
		QTimer::singleShot( 20, this, SLOT( update( void ) ) );
	}
}




void fadeButton::nextState( void )
{
	if( m_state > 0.0f )
	{
		m_state -= 0.1f;
		QTimer::singleShot( 20, this, SLOT( nextState( void ) ) );
	}
}




#include "fade_button.moc"


#endif
