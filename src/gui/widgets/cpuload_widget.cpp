/*
 * cpuload_widget.cpp - widget for displaying CPU-load (partly based on
 *                      Hydrogen's CPU-load-widget)
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtGui/QPainter>

#include "cpuload_widget.h"
#include "embed.h"
#include "engine.h"
#include "Mixer.h"


cpuloadWidget::cpuloadWidget( QWidget * _parent ) :
	QWidget( _parent ),
	m_currentLoad( 0 ),
	m_temp(),
	m_background( embed::getIconPixmap( "cpuload_bg" ) ),
	m_leds( embed::getIconPixmap( "cpuload_leds" ) ),
	m_changed( true ),
	m_updateTimer()
{
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setFixedSize( m_background.width(), m_background.height() );

	m_temp = QPixmap( width(), height() );
	

	connect( &m_updateTimer, SIGNAL( timeout() ),
					this, SLOT( updateCpuLoad() ) );
	m_updateTimer.start( 100 );	// update cpu-load at 10 fps
}




cpuloadWidget::~cpuloadWidget()
{
}




void cpuloadWidget::paintEvent( QPaintEvent *  )
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
		int w = ( m_leds.width() * m_currentLoad / 300 ) * 3;
		if( w > 0 )
		{
			p.drawPixmap( 23, 3, m_leds, 0, 0, w,
							m_leds.height() );
		}
	}
	QPainter p( this );
	p.drawPixmap( 0, 0, m_temp );
}




void cpuloadWidget::updateCpuLoad()
{
	// smooth load-values a bit
	int new_load = ( m_currentLoad + engine::mixer()->cpuLoad() ) / 2;
	if( new_load != m_currentLoad )
	{
		m_currentLoad = new_load;
		m_changed = true;
		update();
	}
}



#include "moc_cpuload_widget.cxx"


