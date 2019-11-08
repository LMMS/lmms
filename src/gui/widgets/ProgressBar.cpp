/*
 * ProgressBar.cpp - widget for displaying CPU-load or volume in the top bar
 *                    (partly based on Hydrogen's CPU-load-widget)
 *
 * Copyright (c) 2019 Lathigos <lathigos/at/tutanota.com>
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - https://lmms.io
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


#include <QPainter>

#include "ProgressBar.h"
#include "embed.h"


ProgressBar::ProgressBar( QWidget * _parent, const QPixmap & background, const QPixmap & leds ) :
	QWidget( _parent ),
	m_value( 0 ),
	m_temp(),
	m_background( background ),
	m_leds( leds ),
	m_changed( true )
{
	setFixedSize( m_background.width(), m_background.height() );

	m_temp = QPixmap( width(), height() );
}




ProgressBar::~ProgressBar()
{
}




void ProgressBar::paintEvent( QPaintEvent *  )
{
	if( m_changed == true )
	{
		m_changed = false;
		
		m_temp.fill( QColor(0,0,0,0) );
		QPainter p( &m_temp );
		p.drawPixmap( 0, 0, m_background );

		// as load-indicator consists of small 2-pixel wide leds with
		// 1 pixel spacing, we have to make sure, only whole leds are
		// shown which we achieve by the following formula
		int w = ( (float)m_leds.width() * m_value );
		if( w > 0 )
		{
			p.drawPixmap( 0, 0, m_leds, 0, 0, w,
							m_leds.height() );
		}
	}
	QPainter p( this );
	p.drawPixmap( 0, 0, m_temp );
}




void ProgressBar::setValue( float value )
{
	if( m_value != value )
	{
		m_value = value;
		m_changed = true;
		update();
	}
}




float ProgressBar::value() const
{
	return m_value;
}




