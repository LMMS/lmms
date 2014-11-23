/*
 * SideBarWidget.cpp - implementation of base-widget for side-bar
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QApplication>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>

#include "SideBarWidget.h"
#include "gui_templates.h"



SideBarWidget::SideBarWidget( const QString & _title, const QPixmap & _icon,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_title( _title ),
	m_icon( _icon )
{
	m_contents = new QWidget( this );
	m_layout = new QVBoxLayout( m_contents );
	m_layout->setSpacing( 5 );
	m_layout->setMargin( 0 );
}




SideBarWidget::~SideBarWidget()
{
}




void SideBarWidget::paintEvent( QPaintEvent * )
{
	const int TITLE_FONT_HEIGHT = 13;

	QPainter p( this );
	p.fillRect( 0, 0, width(), 27, palette().highlight().color() );

	QFont f = p.font();
	f.setBold( true );
	p.setFont( pointSize<TITLE_FONT_HEIGHT>( f ) );

	p.setPen( palette().highlightedText().color() );

	const int tx = m_icon.width()+4;
	const int ty = 2+TITLE_FONT_HEIGHT;
	p.drawText( tx, ty, m_title );
	p.drawLine( tx, ty+4, width()-4, ty+4 );

	p.drawPixmap( 2, 2, m_icon.transformed( QTransform().rotate( -90 ) ) );
}



void SideBarWidget::resizeEvent( QResizeEvent * )
{
	const int MARGIN = 6;
	m_contents->setGeometry( MARGIN, 40 + MARGIN, width() - MARGIN * 2,
						height() - MARGIN * 2 - 40 );
}



#include "moc_SideBarWidget.cxx"

