/*
 * nstate_button.cpp - implementation of n-state-button
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
 

#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QMouseEvent>

#else

#include <qpainter.h>

#endif


#include "nstate_button.h"
#include "embed.h"
#include "tooltip.h"



nStateButton::nStateButton( QWidget * _parent ) :
	QWidget( _parent ),
	m_generalToolTip( "" ),
	m_curState( -1 )
{
#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif
}




nStateButton::~nStateButton()
{
	while( m_states.size() )
	{
		delete m_states.front().first;
		m_states.erase( m_states.begin() );
	}
}




void nStateButton::addState( const QPixmap & _pm, const QString & _tooltip )
{
	m_states.push_back( qMakePair( new QPixmap( _pm ), _tooltip ) );
	// first inserted pixmap?
	if( m_states.size() == 1 )
	{
		// then resize ourself
		resize( _pm.width(), _pm.height() );
		// and set state to first pixmap
		changeState( 0 );
	}
}




void nStateButton::changeState( int _n )
{
	if( _n >= 0 && _n < (int) m_states.size() )
	{
		m_curState = _n;

		const QString & _tooltip =
			( m_states[m_curState].second != "" ) ?
				m_states[m_curState].second :
					m_generalToolTip;
		toolTip::add( this, _tooltip );

		emit stateChanged( m_curState );

		update();
	}
}




void nStateButton::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( rect().size() );
	draw_pm.fill( this, rect().topLeft() );

	QPainter p( &draw_pm, this );
#endif

	if( m_curState >= 0 && m_curState < (int) m_states.size() )
	{
		p.drawPixmap( 0, 0, *m_states[m_curState].first );
	}
#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}




void nStateButton::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton && m_states.size() )
	{
		changeState( ( ++m_curState ) % m_states.size() );
	}
}




#include "nstate_button.moc"

