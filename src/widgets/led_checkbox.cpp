/*
 * led_checkbox.cpp - class ledCheckBox, an improved QCheckBox
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

#include <QPaintEvent>
#include <QFontMetrics>
#include <QPainter>

#else

#include <qfontmetrics.h>
#include <qpainter.h>

#endif


#include "led_checkbox.h"
#include "embed.h"
#include "gui_templates.h"
#include "spc_bg_hndl_widget.h"


static const QString names[ledCheckBox::TOTAL_COLORS] =
{
	"led_yellow", "led_green"
} ;



ledCheckBox::ledCheckBox( const QString & _text, QWidget * _parent,
							ledColors _color ) :
	QCheckBox( _text, _parent )
{
	if( _color >= TOTAL_COLORS || _color < YELLOW )
	{
		_color = YELLOW;
	}
	m_ledOnPixmap = new QPixmap( embed::getIconPixmap( names[_color]
#ifdef QT4
							.toAscii().constData()
#endif
							 ) );
	m_ledOffPixmap = new QPixmap( embed::getIconPixmap( "led_off" ) );

#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif

	setFont( pointSize<7>( font() ) );
	setFixedSize( m_ledOffPixmap->width() + 6 +
					QFontMetrics( font() ).width( text() ),
			m_ledOffPixmap->height() );
}




ledCheckBox::~ledCheckBox()
{
	delete m_ledOnPixmap;
	delete m_ledOffPixmap;
}




void ledCheckBox::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( rect().size() );
	//draw_pm.fill( this, rect().topLeft() );

	QPainter p( &draw_pm, this );
#endif
	p.drawPixmap( 0, 0, specialBgHandlingWidget::getBackground( this ) );

	if( isChecked() == TRUE )
	{
		p.drawPixmap( 0, 0, *m_ledOnPixmap );
	}
	else
	{
		p.drawPixmap( 0, 0, *m_ledOffPixmap );
	}

	p.drawText( m_ledOffPixmap->width() + 2, 10, text() );

#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}


