/*
 * FadeButton.cpp - implementation of fade-button
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QTimer>
#include <QApplication>
#include <QPainter>

#include "embed.h"
#include "FadeButton.h"
#include "update_event.h"


const float FadeDuration = 300;


FadeButton::FadeButton( const QColor & _normal_color,
			const QColor & _activated_color,
                        const QColor & _hold_color,
                        QWidget * _parent ) :
	QAbstractButton( _parent ),
	m_activateStateTimer(),
        m_holdStateTimer(),
	m_normalColor( _normal_color ),
	m_activatedColor( _activated_color ),
        m_holdColor( _hold_color )
{
	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );
	setFocusPolicy( Qt::NoFocus );
        activeNotes = 0;
}




FadeButton::~FadeButton()
{
}

void FadeButton::setActiveColor( const QColor & activated_color )
{
	m_activatedColor = activated_color;
}




void FadeButton::activate()
{
	m_activateStateTimer.restart();
        activeNotes++;

        qWarning("active notes now %d", activeNotes);
	signalUpdate();
}




void FadeButton::noteEnd()
{
        if(activeNotes <= 0)
        {
                qWarning("noteEnd() triggered without a corresponding activate()!");
                activeNotes = 0;
        }
        else
        {
                activeNotes--;
        }

        if(activeNotes <= 0)
                m_holdStateTimer.restart();

        signalUpdate();
}




void FadeButton::customEvent( QEvent * )
{
	update();
}




void FadeButton::paintEvent( QPaintEvent * _pe )
{
	QColor col = m_normalColor;

	if( ! m_activateStateTimer.isNull() && m_activateStateTimer.elapsed() < FadeDuration )
	{
                // The first part of the fade, when a note is triggered.
		const float state = 1 - m_activateStateTimer.elapsed() / FadeDuration;
		const int r = (int)( m_holdColor.red() *
					( 1.0f - state ) +
			m_activatedColor.red() * state );
		const int g = (int)( m_holdColor.green() *
					( 1.0f - state ) +
			m_activatedColor.green() * state );
		const int b = (int)( m_holdColor.blue() *
					( 1.0f - state ) +
			m_activatedColor.blue() * state );
		col.setRgb( r, g, b );
		QTimer::singleShot( 20, this, SLOT( update() ) );
	}
        else if( ! m_activateStateTimer.isNull()
                && m_activateStateTimer.elapsed() >= FadeDuration
                && activeNotes > 0)
        {
                // The fade is done, but at least one note is still held.
                col = m_holdColor;
        }
        else
        {
                // No fade, no notes. Reset to default color.
                col = m_normalColor;
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




void FadeButton::signalUpdate()
{
	QApplication::postEvent( this, new updateEvent() );
}
