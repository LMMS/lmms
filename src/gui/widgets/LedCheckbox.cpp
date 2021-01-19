/*
 * LedCheckbox.cpp - class LedCheckBox, an improved QCheckBox
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QFontMetrics>
#include <QPainter>

#include "LedCheckbox.h"
#include "DeprecationHelper.h"
#include "embed.h"
#include "gui_templates.h"


static const QString names[LedCheckBox::NumColors] =
{
	"led_yellow", "led_green", "led_red"
} ;




LedCheckBox::LedCheckBox( const QString & _text, QWidget * _parent,
				const QString & _name, LedColors _color ) :
	AutomatableButton( _parent, _name ),
	m_text( _text )
{
	initUi( _color );
}




LedCheckBox::LedCheckBox( QWidget * _parent,
				const QString & _name, LedColors _color ) :
	LedCheckBox( QString(), _parent, _name, _color )
{
}



LedCheckBox::~LedCheckBox()
{
	delete m_ledOnPixmap;
	delete m_ledOffPixmap;
}




void LedCheckBox::setText( const QString &s )
{
	m_text = s;
	onTextUpdated();
}




void LedCheckBox::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.setFont( pointSize<7>( font() ) );

	if( model()->value() == true )
		{
			p.drawPixmap( 0, 0, *m_ledOnPixmap );
	}
	else
	{
		p.drawPixmap( 0, 0, *m_ledOffPixmap );
	}

	p.setPen( QColor( 64, 64, 64 ) );
	p.drawText( m_ledOffPixmap->width() + 4, 11, text() );
	p.setPen( QColor( 255, 255, 255 ) );
	p.drawText( m_ledOffPixmap->width() + 3, 10, text() );
}




void LedCheckBox::initUi( LedColors _color )
{
	setCheckable( true );

	if( _color >= NumColors || _color < Yellow )
	{
		_color = Yellow;
	}
	m_ledOnPixmap = new QPixmap( embed::getIconPixmap(
					names[_color].toUtf8().constData() ) );
	m_ledOffPixmap = new QPixmap( embed::getIconPixmap( "led_off" ) );

	setFont( pointSize<7>( font() ) );
	setText( m_text );
}




void LedCheckBox::onTextUpdated()
{
	setFixedSize(m_ledOffPixmap->width() + 5 + horizontalAdvance(QFontMetrics(font()),
				text()),
				m_ledOffPixmap->height());
}





