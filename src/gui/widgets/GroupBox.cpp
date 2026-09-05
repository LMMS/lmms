/*
 * GroupBox.cpp - groupbox for LMMS
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

#include <QMouseEvent>
#include <QPainter>

#include "GroupBox.h"
#include "DeprecationHelper.h"
#include "embed.h"
#include "FontHelper.h"

namespace lmms::gui
{


GroupBox::GroupBox( const QString & _caption, QWidget * _parent ) :
	QWidget( _parent ),
	BoolModelView( nullptr, this ),
	m_caption( _caption ),
	m_titleBarHeight( 11 )
{
	m_led = new PixmapButton( this, _caption );
	m_led->setCheckable( true );
	m_led->move( 3, 0 );
	m_led->setActiveGraphic( embed::getIconPixmap( "led_green" ) );
	m_led->setInactiveGraphic( embed::getIconPixmap( "led_off" ) );

	setModel( new BoolModel( false, nullptr, _caption, true ) );
	setAutoFillBackground( true );
	unsetCursor();
}




GroupBox::~GroupBox()
{
	delete m_led;
}




void GroupBox::modelChanged()
{
	m_led->setModel( model() );
}


bool GroupBox::ledButtonShown() const
{
	return m_led->isVisible();
}


void GroupBox::setLedButtonShown(bool value)
{
	m_led->setVisible(value);
}


void GroupBox::mousePressEvent( QMouseEvent * _me )
{
	const auto pos = position(_me);

	if (ledButtonShown() && pos.y() > 1 && pos.y() < 13 && _me->button() == Qt::LeftButton)
	{
		model()->setValue(!model()->value());
	}
}




void GroupBox::paintEvent( QPaintEvent * pe )
{
	QPainter p( this );

	// Draw background
	p.fillRect( 0, 0, width() - 1, height() - 1, p.background() );

	// outer rect
	p.setPen( p.background().color().darker( 150 ) );
	p.drawRect( 0, 0, width() - 1, height() - 1 );

	// draw line below titlebar
	p.fillRect( 1, 1, width() - 2, m_titleBarHeight + 1, p.background().color().darker( 150 ) );

	// draw text
	p.setPen( palette().color( QPalette::Active, QPalette::Text ) );
	p.setFont(adjustedToPixelSize(font(), DEFAULT_FONT_SIZE));

	int const captionX = ledButtonShown() ? 22 : 6;
	p.drawText(captionX, m_titleBarHeight, m_caption);
}


} // namespace lmms::gui
