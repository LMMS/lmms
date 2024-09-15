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
#include "Song.h"


namespace lmms::gui
{

TimeDisplayWidget::TimeDisplayWidget() :
	QWidget(),
	m_displayMode( DisplayMode::MinutesSeconds ),
	m_spinBoxesLayout( this ),
	m_majorLCD( 4, this ),
	m_minorLCD( 2, this ),
	m_milliSecondsLCD( 3, this )
{
	m_spinBoxesLayout.setSpacing( 0 );
	m_spinBoxesLayout.setContentsMargins(0, 0, 0, 0);
	m_spinBoxesLayout.addWidget( &m_majorLCD );
	m_spinBoxesLayout.addWidget( &m_minorLCD );
	m_spinBoxesLayout.addWidget( &m_milliSecondsLCD );

	setMaximumHeight( 32 );

	setToolTip(tr("Time units"));

	// update labels of LCD spinboxes
	setDisplayMode( m_displayMode );

	connect( getGUI()->mainWindow(), SIGNAL(periodicUpdate()),
					this, SLOT(updateTime()));
}

void TimeDisplayWidget::setDisplayMode( DisplayMode displayMode )
{
	m_displayMode = displayMode;

	switch( m_displayMode )
	{
		case DisplayMode::MinutesSeconds:
			m_majorLCD.setLabel( tr( "MIN" ) );
			m_minorLCD.setLabel( tr( "SEC" ) );
			m_milliSecondsLCD.setLabel( tr( "MSEC" ) );
			break;

		case DisplayMode::BarsTicks:
			m_majorLCD.setLabel( tr( "BAR" ) );
			m_minorLCD.setLabel( tr( "BEAT" ) );
			m_milliSecondsLCD.setLabel( tr( "TICK" ) );
			break;

		default: break;
	}
}




void TimeDisplayWidget::updateTime()
{
	Song* s = Engine::getSong();

	switch( m_displayMode )
	{
		case DisplayMode::MinutesSeconds:
		{
			int msec = s->getMilliseconds();
			m_majorLCD.setValue(msec / 60000);
			m_minorLCD.setValue((msec / 1000) % 60);
			m_milliSecondsLCD.setValue(msec % 1000);
			break;
		}
		case DisplayMode::BarsTicks:
		{
			int tick = s->getPlayPos().getTicks();
			m_majorLCD.setValue((int)(tick / s->ticksPerBar()) + 1);
			m_minorLCD.setValue((tick % s->ticksPerBar()) /
						 (s->ticksPerBar() / s->getTimeSigModel().getNumerator() ) +1);
			m_milliSecondsLCD.setValue((tick % s->ticksPerBar()) %
							(s->ticksPerBar() / s->getTimeSigModel().getNumerator()));
			break;
		}
		default:
			break;
	}
}




void TimeDisplayWidget::mousePressEvent( QMouseEvent* mouseEvent )
{
	if( mouseEvent->button() == Qt::LeftButton )
	{
		if( m_displayMode == DisplayMode::MinutesSeconds )
		{
			setDisplayMode( DisplayMode::BarsTicks );
		}
		else
		{
			setDisplayMode( DisplayMode::MinutesSeconds );
		}
	}
}

} // namespace lmms::gui