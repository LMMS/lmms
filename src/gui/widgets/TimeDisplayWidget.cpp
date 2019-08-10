/*
 * TimeDisplayWidget.cpp - widget for displaying current playback time
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QMouseEvent>

#include "TimeDisplayWidget.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Engine.h"
#include "ToolTip.h"
#include "Song.h"



TimeDisplayWidget::TimeDisplayWidget() :
	QWidget(),
	m_displayMode( MinutesSeconds ),
	m_spinBoxesLayout( this ),
	m_majorLabel( this ),
	m_majorValue( this ),
	m_minorLabel( this ),
	m_minorValue( this ),
	m_milliSecondsLabel( this ),
	m_milliSecondsValue( this )
{
	m_spinBoxesLayout.setSpacing( 0 );
	m_spinBoxesLayout.setMargin( 0 );
	m_spinBoxesLayout.addWidget( &m_majorLabel, 0, 0 );
	m_spinBoxesLayout.addWidget( &m_majorValue, 1, 0 );
	m_spinBoxesLayout.addWidget( &m_minorLabel, 0, 1 );
	m_spinBoxesLayout.addWidget( &m_minorValue, 1, 1 );
	m_spinBoxesLayout.addWidget( &m_milliSecondsLabel, 0, 2 );
	m_spinBoxesLayout.addWidget( &m_milliSecondsValue, 1, 2 );

	m_majorLabel.setAlignment(Qt::AlignRight);
	m_majorValue.setAlignment(Qt::AlignRight);
	m_minorLabel.setAlignment(Qt::AlignRight);
	m_minorValue.setAlignment(Qt::AlignRight);
	m_milliSecondsLabel.setAlignment(Qt::AlignRight);
	m_milliSecondsValue.setAlignment(Qt::AlignRight);

	ToolTip::add( this, tr( "Time units" ) );

	// update labels of LCD spinboxes
	setDisplayMode( m_displayMode );

	connect( gui->mainWindow(), SIGNAL( periodicUpdate() ),
					this, SLOT( updateTime() ) );
}

void TimeDisplayWidget::setDisplayMode( DisplayMode displayMode )
{
	m_displayMode = displayMode;

	switch( m_displayMode )
	{
		case MinutesSeconds:
			m_majorLabel.setText( tr( "MIN" ) );
			m_minorLabel.setText( tr( "SEC" ) );
			m_milliSecondsLabel.setText( tr( "MSEC" ) );
			break;

		case BarsTicks:
			m_majorLabel.setText( tr( "BAR" ) );
			m_minorLabel.setText( tr( "BEAT" ) );
			m_milliSecondsLabel.setText( tr( "TICK" ) );
			break;

		default: break;
	}
}




void TimeDisplayWidget::updateTime()
{
	Song* s = Engine::getSong();

	switch( m_displayMode )
	{
		case MinutesSeconds:
			int msec;
			msec = s->getMilliseconds();
			m_majorValue.setNum(msec / 60000);
			m_minorValue.setNum((msec / 1000 ) % 60);
			m_milliSecondsValue.setNum(msec % 1000);
			break;

		case BarsTicks:
			int tick;
			tick = s->getPlayPos().getTicks();
			m_majorValue.setNum((int)(tick / s->ticksPerTact() ) + 1);
			m_minorValue.setNum( ( tick % s->ticksPerTact() ) /
						 ( s->ticksPerTact() / s->getTimeSigModel().getNumerator() ) +1 );
			m_milliSecondsValue.setNum((tick % s->ticksPerTact()) %
							(s->ticksPerTact() / s->getTimeSigModel().getNumerator()));
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
