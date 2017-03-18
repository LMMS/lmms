/*
 * NStateButton.cpp - implementation of n-state-button
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "NStateButton.h"
#include "ToolTip.h"



NStateButton::NStateButton( QWidget * _parent ) :
	ToolButton( _parent ),
	m_generalToolTip( "" ),
	m_curState( -1 )
{
}




NStateButton::~NStateButton()
{
	while( m_states.size() )
	{
		m_states.erase( m_states.begin() );
	}
}




void NStateButton::addState( const QPixmap & _pm, const QString & _tooltip )
{
	m_states.push_back( qMakePair( _pm, _tooltip ) );
	// first inserted pixmap?
	if( m_states.size() == 1 )
	{
		// and set state to first pixmap
		changeState( 0 );
	}
}




void NStateButton::changeState( int _n )
{
	if( _n >= 0 && _n < (int) m_states.size() )
	{
		m_curState = _n;

		const QString & _tooltip =
			( m_states[m_curState].second != "" ) ?
				m_states[m_curState].second :
					m_generalToolTip;
		ToolTip::add( this, _tooltip );

		setIcon( m_states[m_curState].first );

		emit changedState( m_curState );
	}
}



void NStateButton::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton && m_states.size() )
	{
		changeState( ( ++m_curState ) % m_states.size() );
	}
	ToolButton::mousePressEvent( _me );
}
