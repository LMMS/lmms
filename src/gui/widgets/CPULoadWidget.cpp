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

#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>

#include "CPULoadWidget.h"
#include "embed.h"
#include "Engine.h"
#include "Mixer.h"
#include "ProgressBar.h"
#include "ToolTip.h"

CPULoadWidget::CPULoadWidget( QWidget * _parent ) :
	QWidget( _parent ),
	m_currentLoad( 0.0f ),
	m_updateTimer()
{
	QHBoxLayout * mainLayout = new QHBoxLayout( this );
	mainLayout->setSpacing( 4 );
	mainLayout->setContentsMargins( 0, 0, 0, 0 );
	
	QLabel * label = new QLabel( this );
	label->setObjectName( "integerDisplayTitle" );
	label->setText( tr( "CPU" ) );
	mainLayout->addWidget( label );
	
	m_progressBar = new ProgressBar( this,
					embed::getIconPixmap( "cpuload_bg" ),
					embed::getIconPixmap( "cpuload_leds" ) );
	mainLayout->addWidget( m_progressBar );

	ToolTip::add( this, tr( "CPU load" ) );

	connect( &m_updateTimer, SIGNAL( timeout() ),
					this, SLOT( updateCpuLoad() ) );
	m_updateTimer.start( 100 );	// update cpu-load at 10 fps
}




CPULoadWidget::~CPULoadWidget()
{
}




void CPULoadWidget::updateCpuLoad()
{
	// smooth load-values a bit
	float new_load = ( m_currentLoad + (float)Engine::mixer()->cpuLoad() ) / 2.0f;
	if( new_load != m_currentLoad )
	{
		m_currentLoad = new_load;
		m_progressBar->setValue((float)new_load / 100.0f);
	}
}






