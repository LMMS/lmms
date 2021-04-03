/*
 * CPULoadWidget.cpp - widget for displaying CPU-load (partly based on
 *                      Hydrogen's CPU-load-widget)
 *
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

#include "CPULoadWidget.h"
#include "embed.h"
#include "Engine.h"
#include "Mixer.h"

CPULoadWidget::CPULoadWidget( QWidget * _parent ) :
	QWidget( _parent ),
	m_currentLoad(0),
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




CPULoadWidget::~CPULoadWidget()
{
}




void CPULoadWidget::paintEvent( QPaintEvent *  )
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




void CPULoadWidget::updateCpuLoad()
{
	// Additional display smoothing for the main load-value. Stronger averaging
	// cannot be used directly in the profiler: cpuLoad() must react fast enough
	// to be useful as overload indicator in Mixer::criticalXRuns().
	const int new_load = (m_currentLoad + Engine::mixer()->cpuLoad()) / 2;

	if (new_load != m_currentLoad)
	{
		setToolTip(
			tr("DSP total: ") + QString::number(new_load) + " %\n" +
			tr(" - Notes and setup: ") + QString::number(Engine::mixer()->detailLoad(0)) + " %\n" +
			tr(" - Instruments: ") + QString::number(Engine::mixer()->detailLoad(1)) + " %\n" +
			tr(" - Effects: ") + QString::number(Engine::mixer()->detailLoad(2)) + " %\n" +
			tr(" - Mixing: ") + QString::number(Engine::mixer()->detailLoad(3)) + " %"
		);
		m_currentLoad = new_load;
		m_changed = true;
		update();
	}
}

