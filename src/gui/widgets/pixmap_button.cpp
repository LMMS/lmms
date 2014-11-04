/*
 * pixmap_button.cpp - implementation of pixmap-button (often used as "themed"
 *                     checkboxes/radiobuttons etc)
 *
 * Copyright (c) 2004-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QPainter>

#include "pixmap_button.h"
#include "MainWindow.h"
#include "embed.h"



pixmapButton::pixmapButton( QWidget * _parent, const QString & _name ) :
	automatableButton( _parent, _name ),
	m_activePixmap(),
	m_inactivePixmap(),
	m_pressed( false )
{
	setActiveGraphic( embed::getIconPixmap( "led_yellow" ) );
	setInactiveGraphic( embed::getIconPixmap( "led_off" ), false );
}




pixmapButton::~pixmapButton()
{
}




void pixmapButton::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	if( ( model() != NULL && model()->value() ) || m_pressed )
	{
		if( !m_activePixmap.isNull() )
		{
			p.drawPixmap( 0, 0, m_activePixmap );
		}
	}
	else if( !m_inactivePixmap.isNull() )
	{
		p.drawPixmap( 0, 0, m_inactivePixmap );
	}
}





void pixmapButton::mousePressEvent( QMouseEvent * _me )
{
	// Show pressing graphics if this isn't checkable
	if( !isCheckable() )
	{
		m_pressed = true;
		update();
	}

	automatableButton::mousePressEvent( _me );
}




void pixmapButton::mouseReleaseEvent( QMouseEvent * _me )
{
	automatableButton::mouseReleaseEvent( _me );

	if( !isCheckable() )
	{
		m_pressed = false;
		update();
	}
}




void pixmapButton::mouseDoubleClickEvent( QMouseEvent * _me )
{
	emit doubleClicked();
	_me->accept();
}




void pixmapButton::setActiveGraphic( const QPixmap & _pm )
{
	m_activePixmap = _pm;
	resize( m_activePixmap.width(), m_activePixmap.height() );
}




void pixmapButton::setInactiveGraphic( const QPixmap & _pm, bool _update )
{
	m_inactivePixmap = _pm;
	if( _update )
	{
		update();
	}
}




#include "moc_pixmap_button.cxx"


