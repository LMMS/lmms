/*
 * fade_button.cpp - implementation of fade-button
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
 

#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include "embed.h"
#include "fade_button.h"
#include "update_event.h"


const float FadeDuration = 300;


fadeButton::fadeButton( const QColor & _normal_color,
			const QColor & _activated_color, QWidget * _parent ) :
	QAbstractButton( _parent ),
	m_stateTimer(),
	m_normalColor( _normal_color ),
	m_activatedColor( _activated_color )
{
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );
	setFocusPolicy( Qt::NoFocus );
}




fadeButton::~fadeButton()
{
}




void fadeButton::activate()
{
	m_stateTimer.restart();
	signalUpdate();
}




void fadeButton::customEvent( QEvent * )
{
	update();
}




void fadeButton::paintEvent( QPaintEvent * _pe )
{
	QColor col = m_normalColor;
	if( m_stateTimer.elapsed() < FadeDuration )
	{
		const float state = 1 - m_stateTimer.elapsed() / FadeDuration;
		const int r = (int)( m_normalColor.red() *
					( 1.0f - state ) +
			m_activatedColor.red() * state );
		const int g = (int)( m_normalColor.green() *
					( 1.0f - state ) +
			m_activatedColor.green() * state );
		const int b = (int)( m_normalColor.blue() *
					( 1.0f - state ) +
			m_activatedColor.blue() * state );
		col.setRgb( r, g, b );
		QTimer::singleShot( 20, this, SLOT( update() ) );
	}

	QPainter p( this );
	p.fillRect( rect(), col );

	int w = rect().right();
	int h = rect().bottom();
	p.setPen( m_normalColor.darker(130) );
	p.drawLine( w, 1, w, h );
	p.drawLine( 1, h, w, h );
	p.setPen( m_normalColor.lighter(130) );
	p.drawLine( 0, 0, 0, h-1 );
	p.drawLine( 0, 0, w, 0 );
}




void fadeButton::signalUpdate()
{
	QApplication::postEvent( this, new updateEvent() );
}




#include "moc_fade_button.cxx"


