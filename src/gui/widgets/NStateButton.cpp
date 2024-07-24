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


namespace lmms::gui
{


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




void NStateButton::changeState(int state)
{
	if (state >= 0 && state < m_states.size() && state != m_curState)
	{
		m_curState = state;

		const auto& [icon, tooltip] = m_states[m_curState];
		setToolTip(tooltip.isEmpty() ? m_generalToolTip : tooltip);
		setIcon(icon);

		emit changedState(m_curState);
	}
}

void NStateButton::mousePressEvent(QMouseEvent* me)
{
	if (me->button() == Qt::LeftButton && !m_states.empty())
	{
		changeState((m_curState + 1) % m_states.size());
	}
	ToolButton::mousePressEvent(me);
}

} // namespace lmms::gui
