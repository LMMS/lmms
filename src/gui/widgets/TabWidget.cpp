/*
 * TabWidget.cpp - tabwidget for LMMS
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


#include "TabWidget.h"

#include <QMouseEvent>
#include <QPainter>

#include "gui_templates.h"
#include "embed.h"



TabWidget::TabWidget( const QString & _caption, QWidget * _parent ) :
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




TabWidget::~TabWidget()
{
}




void TabWidget::addTab( QWidget * _w, const QString & _name,  const char * pixmapName, int _idx )
{
	setFont( pointSize<8>( font() ) );

        // Append tab when position is not given
	if( _idx < 0/* || m_widgets.contains( _idx ) == true*/ )
	{
		while( m_widgets.contains( ++_idx ) == true )
		{
		}
	}

        fprintf( stderr, "adding tab %s. idx=%d\n", pixmapName, _idx);

        // Register new tab
	widgetDesc d = { _w, pixmapName, _name, fontMetrics().width( _name ) + 10 } ;
	m_widgets[_idx] = d;

        // Initialize tab's window
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




void TabWidget::setActiveTab( int _idx )
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




void TabWidget::mousePressEvent( QMouseEvent * _me )
{

        fprintf( stderr, "TabWidget::mousePressEvent x=%d y=%d\n", _me->x(), _me->y() );

	if( _me->y() > 1 && _me->y() < 13 )
	{
		for( widgetStack::iterator it = m_widgets.begin();
						it != m_widgets.end(); ++it )
		{
			if( _me->x() >= 8 + it.key() * 30 &&
					_me->x() <= 8 + it.key() * 30 + 14 )
			{
        			fprintf( stderr, "TabWidget::mousePressEvent Pressed tab %d\n", it.key() );
				setActiveTab( it.key() );
				update();
				return;
			}
		}
	}
}




void TabWidget::resizeEvent( QResizeEvent * )
{
	for( widgetStack::iterator it = m_widgets.begin();
						it != m_widgets.end(); ++it )
	{
		( *it ).w->setFixedSize( width() - 4, height() - 14 );
	}
}





void TabWidget::paintEvent( QPaintEvent * _pe )
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

	// Draw all tabs
	int tab_x_offset = 8;
        for( widgetStack::iterator it = m_widgets.begin();
                                               it != m_widgets.end(); ++it )
        {

		// Get active or inactive artwork
		std::string tab = string( ( *it).pixmapName );
                if( it.key() == m_activeTab )
                {
			tab += "_active";
                } else 
		{
			tab += "_inactive";
		}
                QPixmap *artwork = new QPixmap(  embed::getIconPixmap( tab.c_str() ) );

		// Draw tab
                p.drawPixmap(tab_x_offset, 0, *artwork );

		// Next tab's horizontal position
                tab_x_offset += 30;
        }
}




void TabWidget::wheelEvent( QWheelEvent * _we )
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







