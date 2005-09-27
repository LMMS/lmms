/*
 * side_bar_widget.cpp - implementation of base-widget for side-bar
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

#include <QApplication>
#include <QPainter>
#include <QFontMetrics>

#else

#include <qapplication.h>
#include <qpainter.h>
#include <qfontmetrics.h>

#endif


#include "side_bar_widget.h"
#include "types.h"
#include "gui_templates.h"



sideBarWidget::sideBarWidget( const QString & _title, const QPixmap & _icon,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_title( _title ),
	m_icon( _icon )
{
	m_contents = new QWidget( this );
	m_layout = new QVBoxLayout( m_contents );
	m_layout->setSpacing( 5 );
}




sideBarWidget::~sideBarWidget()
{
}




void sideBarWidget::paintEvent( QPaintEvent * )
{
	const Uint16 TITLE_FONT_HEIGHT = 16;

#ifdef QT4
	QPainter p( this );
	p.fillRect( 0, 0, width(), 27, palette().highlight().color() );
#else
	QPixmap draw_pm( rect().size() );
	draw_pm.fill( QApplication::palette().color( QPalette::Normal,
							QColorGroup::Background ) );

	QPainter p( &draw_pm, this );
	p.fillRect( 0, 0, width(), 27, QApplication::palette().color(
							QPalette::Normal,
						QColorGroup::Highlight ) );
#endif

	QFont f = p.font();
	f.setBold( TRUE );
	p.setFont( pointSize<TITLE_FONT_HEIGHT>( f ) );

#ifdef QT4
	p.setPen( palette().highlightedText().color() );
#else
	p.setPen( QApplication::palette().color( QPalette::Normal,
					QColorGroup::HighlightedText ) );
#endif
	const Uint16 tx = m_icon.width()+4;
	const Uint16 ty = 2+TITLE_FONT_HEIGHT;
	p.drawText( tx, ty, m_title );
	p.drawLine( tx, ty+4, width()-4, ty+4 );

	p.drawPixmap( 2, 2, m_icon );

#ifndef QT4
	// and blit all drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}



void sideBarWidget::resizeEvent( QResizeEvent * )
{
	const Uint16 MARGIN = 6;
	m_contents->setGeometry( MARGIN, 40 + MARGIN, width() - MARGIN * 2,
							height() - MARGIN * 2 - 40 );
}



#include "side_bar.moc"
#include "side_bar_widget.moc"

