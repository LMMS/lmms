/*
 * TimeDisplayWidget.cpp - widget for displaying current playback time
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "TimeDisplayWidget.h"
#include "MainWindow.h"
#include "engine.h"
#include "tooltip.h"
#include "song.h"



TimeDisplayWidget::TimeDisplayWidget() :
	QWidget(),
	m_displayMode( MinutesSeconds ),
	m_spinBoxesLayout( this ),
	m_majorLCD( 3, this ),
	m_minorLCD( 3, this ),
	m_milliSecondsLCD( 3, this )
{
	m_spinBoxesLayout.setSpacing( 0 );
	m_spinBoxesLayout.setMargin( 0 );
	m_spinBoxesLayout.addWidget( &m_majorLCD );
	m_spinBoxesLayout.addWidget( &m_minorLCD );
	m_spinBoxesLayout.addWidget( &m_milliSecondsLCD );

	setMaximumHeight( 32 );

	toolTip::add( this, tr( "click to change time units" ) );

	// update labels of LCD spinboxes
	setDisplayMode( m_displayMode );

	connect( engine::mainWindow(), SIGNAL( periodicUpdate() ),
					this, SLOT( updateTime() ) );
}




TimeDisplayWidget::~TimeDisplayWidget()
{
}





void TimeDisplayWidget::setDisplayMode( DisplayMode displayMode )
{
	m_displayMode = displayMode;

	switch( m_displayMode )
	{
		case MinutesSeconds:
			m_majorLCD.setLabel( "MIN" );
			m_minorLCD.setLabel( "SEC" );
			m_milliSecondsLCD.setLabel( "MSEC" );
			break;

		case BarsTicks:
			m_majorLCD.setLabel( "BAR" );
			m_minorLCD.setLabel( "BEAT" );
			m_milliSecondsLCD.setLabel( "TICK" );
			break;

		default: break;
	}
}




void TimeDisplayWidget::updateTime()
{
	song* s = engine::getSong();

	switch( m_displayMode )
	{
		case MinutesSeconds:
			m_majorLCD.setValue( s->getMilliseconds() / 60000 );
			m_minorLCD.setValue( ( s->getMilliseconds() / 1000 ) % 60 );
			m_milliSecondsLCD.setValue( s->getMilliseconds() % 1000 );
			break;

		case BarsTicks:
			m_majorLCD.setValue( s->getTacts() + 1 );
			m_minorLCD.setValue( ( s->getTicks() % s->ticksPerTact() ) / 
					     ( s->ticksPerTact() / s->getTimeSigModel().getNumerator() ) +1 );
;
			m_milliSecondsLCD.setValue( ( s->getTicks() % s->ticksPerTact() ) %
						    ( s->ticksPerTact() / s->getTimeSigModel().getNumerator() ) );
			break;

		default: break;
	}
}




void TimeDisplayWidget::mousePressEvent( QMouseEvent* mouseEvent )
{
	if( mouseEvent->button() == Qt::LeftButton )
	{
		if( m_displayMode == MinutesSeconds )
		{
			setDisplayMode( BarsTicks );
		}
		else
		{
			setDisplayMode( MinutesSeconds );
		}
	}
}



#include "moc_TimeDisplayWidget.cxx"


