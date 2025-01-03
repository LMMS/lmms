/*
 * SideBarWidget.cpp - implementation of base-widget for side-bar
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "SideBarWidget.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPushButton>

#include "embed.h"
#include "FontHelper.h"

namespace lmms::gui
{


SideBarWidget::SideBarWidget( const QString & _title, const QPixmap & _icon,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_title( _title ),
	m_icon(_icon),
	m_buttonSize(17, 17)
{
	m_contents = new QWidget( this );
	m_layout = new QVBoxLayout( m_contents );
	m_layout->setSpacing( 5 );
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_closeBtn = new QPushButton(embed::getIconPixmap("close"), QString(), this);
	m_closeBtn->resize(m_buttonSize);
	m_closeBtn->setToolTip(tr("Close"));
	connect(m_closeBtn, &QPushButton::clicked,
		[this]() { this->closeButtonClicked(); });
}




void SideBarWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );
	p.fillRect( 0, 0, width(), 27, palette().highlight().color() );

	QFont f = p.font();
	f.setBold( true );
	f.setUnderline(false);
	p.setFont(adjustedToPixelSize(f, LARGE_FONT_SIZE));

	p.setPen( palette().highlightedText().color() );

	const int tx = m_icon.width() + 8;

	QFontMetrics metrics( f );
	const int ty = (metrics.ascent() + m_icon.height()) / 2;
	p.drawText( tx, ty, m_title );

	p.drawPixmap( 2, 2, m_icon.transformed( QTransform().rotate( -90 ) ) );
}



void SideBarWidget::resizeEvent( QResizeEvent * )
{
	const int MARGIN = 6;
	m_contents->setGeometry( MARGIN, 40 + MARGIN, width() - MARGIN * 2,
						height() - MARGIN * 2 - 40 );
	m_closeBtn->move(m_contents->geometry().width() - MARGIN - 5, 5);
}


} // namespace lmms::gui
