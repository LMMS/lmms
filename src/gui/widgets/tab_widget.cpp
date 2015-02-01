/*
 * tab_widget.cpp - tabwidget for LMMS
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "tab_widget.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QWheelEvent>

#include "gui_templates.h"



tabWidget::tabWidget( const QString & _caption, QWidget * _parent ) :
	QWidget( _parent ),
	m_activeTab( 0 ),
	m_caption( _caption ),
	m_tabheight( _caption.isEmpty() ? 11: 10 )
{
	setFont( pointSize<8>( font() ) );

	setAutoFillBackground( true );
	QColor bg_color = QApplication::palette().color( QPalette::Active,
							QPalette::Background ).
								darker( 132 );
	QPalette pal = palette();
	pal.setColor( QPalette::Background, bg_color );
	setPalette( pal );
}




tabWidget::~tabWidget()
{
}




void tabWidget::addTab( QWidget * _w, const QString & _name, int _idx )
{
	setFont( pointSize<8>( font() ) );
	widgetDesc d = { _w, _name, fontMetrics().width( _name ) + 10 } ;
	if( _idx < 0/* || m_widgets.contains( _idx ) == true*/ )
	{
		while( m_widgets.contains( ++_idx ) == true )
		{
		}
	}
	m_widgets[_idx] = d;
	_w->setFixedSize( width() - 4, height() - 14 );
	_w->move( 2, 13 );
	_w->hide();

	if( m_widgets.contains( m_activeTab ) )
	{
		// make sure new tab doesn't overlap current widget
		m_widgets[m_activeTab].w->show();
		m_widgets[m_activeTab].w->raise();
	}
}




void tabWidget::setActiveTab( int _idx )
{
	if( m_widgets.contains( _idx ) )
	{
		int old_active = m_activeTab;
		m_activeTab = _idx;
		m_widgets[m_activeTab].w->raise();
		m_widgets[m_activeTab].w->show();
		if( old_active != _idx && m_widgets.contains( old_active ) )
		{
			m_widgets[old_active].w->hide();
		}
		update();
	}
}




void tabWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->y() > 1 && _me->y() < 13 )
	{
		int cx = ( ( m_caption == "" ) ? 4 : 14 ) +
					fontMetrics().width( m_caption );
		for( widgetStack::iterator it = m_widgets.begin();
						it != m_widgets.end(); ++it )
		{
			if( _me->x() >= cx &&
					_me->x() <= cx + ( *it ).nwidth )
			{
				setActiveTab( it.key() );
				update();
				return;
			}
			cx += ( *it ).nwidth;
		}
	}
}




void tabWidget::resizeEvent( QResizeEvent * )
{
	for( widgetStack::iterator it = m_widgets.begin();
						it != m_widgets.end(); ++it )
	{
		( *it ).w->setFixedSize( width() - 4, height() - 14 );
	}
}





void tabWidget::paintEvent( QPaintEvent * _pe )
{
	setFont( pointSize<8>( font() ) );
	QPainter p( this );

	QColor bg_color = QApplication::palette().color( QPalette::Active,
							QPalette::Background );
	QLinearGradient g( 0, 0, 0, m_tabheight );
	g.setColorAt( 0, bg_color.darker( 250 ) );
	g.setColorAt( 0.1, bg_color.lighter( 120 ) );
	g.setColorAt( 1, bg_color.darker( 250 ) );
	p.fillRect( 0, 0, width() - 1, height() - 1, bg_color );

	bool big_tab_captions = ( m_caption == "" );

	p.setPen( bg_color.darker( 150 ) );
	p.drawRect( 0, 0, width() - 1, height() - 1 );

	p.setPen( bg_color.light( 150 ) );
	p.drawLine( width() - 1, 0, width() - 1, height() - 1 );
	p.drawLine( 0, height() - 1, width() - 1, height() - 1 );

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawRect( 1, 1, width() - 3, height() - 3 );

	p.fillRect( 2, 2, width() - 4, m_tabheight, g );
	p.drawLine( 2, m_tabheight + 2, width() - 3, m_tabheight + 2);

	if( ! m_caption.isEmpty() )
	{
		p.setPen( QColor( 255, 255, 255 ) );
		p.drawText( 5, 11, m_caption );
	}

	// Calculate the tabs' x (tabs are painted next to the caption)
	int tab_x_offset = m_caption.isEmpty() ? 4 : 14 + fontMetrics().width( m_caption );

	QColor cap_col( 160, 160, 160 );
	if( big_tab_captions )
	{
		p.setFont( pointSize<8>( p.font() ) );
		cap_col = QColor( 224, 224, 224 );
	}
	else
	{
		p.setFont( pointSize<7>( p.font() ) );
	}
	p.setPen( cap_col );


	for( widgetStack::iterator it = m_widgets.begin();
						it != m_widgets.end(); ++it )
	{
		if( it.key() == m_activeTab )
		{
			p.setPen( QColor( 32, 48, 64 ) );
			p.fillRect( tab_x_offset, 2, ( *it ).nwidth - 6, 10, cap_col );
		}
		p.drawText( tab_x_offset + 3, m_tabheight, ( *it ).name );
		p.setPen( cap_col );
		tab_x_offset += ( *it ).nwidth;
	}
}




void tabWidget::wheelEvent( QWheelEvent * _we )
{
	if (_we->y() > m_tabheight)
		return;

	_we->accept();
	int dir = ( _we->delta() < 0 ) ? 1 : -1;
	int tab = m_activeTab;
	while( tab > -1 && static_cast<int>( tab ) < m_widgets.count() )
	{
		tab += dir;
		if( m_widgets.contains( tab ) )
		{
			break;
		}
	}
	setActiveTab( tab );
}




#include "moc_tab_widget.cxx"


