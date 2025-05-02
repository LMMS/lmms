/*
 * PixmapButton.cpp - implementation of pixmap-button (often used as "themed"
 *                     checkboxes/radiobuttons etc)
 *
 * Copyright (c) 2004-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QPainter>

#include "PixmapButton.h"
#include "embed.h"

namespace lmms::gui
{


PixmapButton::PixmapButton( QWidget * _parent, const QString & _name ) :
	AutomatableButton( _parent, _name ),
	m_activePixmap(),
	m_inactivePixmap(),
	m_pressed( false )
{
	setActiveGraphic( embed::getIconPixmap( "led_yellow" ) );
	setInactiveGraphic( embed::getIconPixmap( "led_off" ), false );
}







void PixmapButton::paintEvent(QPaintEvent*)
{
	QPainter p(this);

	QPixmap* pixmapToDraw = isActive() ? &m_activePixmap : &m_inactivePixmap;

	if (!pixmapToDraw->isNull())
	{
		p.drawPixmap(0, 0, *pixmapToDraw);
	}
}





void PixmapButton::mousePressEvent( QMouseEvent * _me )
{
	// Show pressing graphics if this isn't checkable
	if( !isCheckable() )
	{
		m_pressed = true;
		update();
	}

	AutomatableButton::mousePressEvent( _me );
}




void PixmapButton::mouseReleaseEvent( QMouseEvent * _me )
{
	AutomatableButton::mouseReleaseEvent( _me );

	if( !isCheckable() )
	{
		m_pressed = false;
		update();
	}
}




void PixmapButton::mouseDoubleClickEvent( QMouseEvent * _me )
{
	emit doubleClicked();
	_me->accept();
}


void PixmapButton::setActiveGraphic(const QPixmap &pm)
{
	m_activePixmap = pm;
	resize(embed::logicalSize(m_activePixmap));
}


void PixmapButton::setInactiveGraphic( const QPixmap & _pm, bool _update )
{
	m_inactivePixmap = _pm;
	if( _update )
	{
		update();
	}
}

QSize PixmapButton::sizeHint() const
{
	return minimumSizeHint();
}

QSize PixmapButton::minimumSizeHint() const
{
	return embed::logicalSize(m_activePixmap)
		.expandedTo(embed::logicalSize(m_inactivePixmap));
}

bool PixmapButton::isActive() const
{
	return (model() != nullptr && model()->value()) || m_pressed;
}

} // namespace lmms::gui
