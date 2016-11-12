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

#include <QMouseEvent>
#include <QMessageBox>

#include "TimeDisplayWidget.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Engine.h"
#include "ToolTip.h"
#include "Song.h"
#include "CaptionMenu.h"



TimeDisplayWidget::TimeDisplayWidget() :
	QWidget(),
	m_displayMode( MinutesSeconds ),
	m_spinBoxesLayout( this ),
	m_majorLCD( 4, this ),
	m_minorLCD( 2, this ),
	m_milliSecondsLCD( 3, this )
{
	m_spinBoxesLayout.setSpacing( 0 );
	m_spinBoxesLayout.setMargin( 0 );
	m_spinBoxesLayout.addWidget( &m_majorLCD );
	m_spinBoxesLayout.addWidget( &m_minorLCD );
	m_spinBoxesLayout.addWidget( &m_milliSecondsLCD );

	setMaximumHeight( 32 );

    ToolTip::add( this, tr( "left click to change time units, \nright click to jump to specified time" ) );

	// update labels of LCD spinboxes
	setDisplayMode( m_displayMode );

	connect( gui->mainWindow(), SIGNAL( periodicUpdate() ),
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
			m_majorLCD.setLabel( tr( "MIN" ) );
			m_minorLCD.setLabel( tr( "SEC" ) );
			m_milliSecondsLCD.setLabel( tr( "MSEC" ) );
			break;

		case BarsTicks:
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
		case MinutesSeconds:
			m_majorLCD.setValue( s->getMilliseconds() / 60000 );
			m_minorLCD.setValue( ( s->getMilliseconds() / 1000 ) % 60 );
			m_milliSecondsLCD.setValue( s->getMilliseconds() % 1000 );
			break;

		case BarsTicks:
            qint64 tick;
            tick = ( static_cast<qint64>(s->getMilliseconds()) * s->getTempo() * (DefaultTicksPerTact / 4 ) ) / 60000 ;
            m_majorLCD.setValue( (tick / s->ticksPerTact() ) + 1);
			m_minorLCD.setValue( ( tick % s->ticksPerTact() ) /
						 ( s->ticksPerTact() / s->getTimeSigModel().getNumerator() ) +1 );
			m_milliSecondsLCD.setValue( ( tick % s->ticksPerTact() ) %
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

void TimeDisplayWidget::contextMenuEvent(QContextMenuEvent *event)
{
    CaptionMenu contextMenu( tr("Timeline") );
    contextMenu.addAction( tr("Go to...") );
    QAction* action  = contextMenu.exec( QCursor::pos() );
    if (action)
    {
        Song* s = Engine::getSong();
         m_timeinputbox = new TimeInputDialog( this );
         m_timeinputbox->setTimeModel( this->m_displayMode );
         m_timeinputbox->setMilliSeconds( s->getMilliseconds() );
         m_timeinputbox->show();
         connect(m_timeinputbox, SIGNAL(accepted()), this, SLOT(timeJump()));
    }
    return;
}

void TimeDisplayWidget::timeJump()
{
    Song* s = Engine::getSong();
    if (s->isPlaying()) {
        s->setPlayPos( m_timeinputbox->getTicks(), s->playMode());
    }else{
        s->setPlayPos( m_timeinputbox->getTicks(), Song::PlayModes::Mode_PlaySong);
    }
}
