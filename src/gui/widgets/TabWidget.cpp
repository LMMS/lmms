/*
 * TabWidget.cpp - tabwidget for LMMS
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


#include "TabWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QToolTip>
#include <QWheelEvent>

#include "gui_templates.h"
#include "embed.h"

TabWidget::TabWidget( const QString & _caption, QWidget * _parent, bool usePixmap ) :
	QWidget( _parent ),
	m_activeTab( 0 ),
	m_caption( _caption ),
	m_tabheight( _caption.isEmpty() ? 11: 10 ),
	m_usePixmap( usePixmap )
{

	m_tabheight = _caption.isEmpty() ? TAB_HEIGHT - 3 : TAB_HEIGHT - 4;

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




void TabWidget::addTab( QWidget * _w, const QString & _name, const QString & _tooltip, const char *activePixmap, const char *inactivePixmap, int _idx )
{
	setFont( pointSize<8>( font() ) );

        // Append tab when position is not given
	if( _idx < 0/* || m_widgets.contains( _idx ) == true*/ )
	{
		while( m_widgets.contains( ++_idx ) == true )
		{
		}
	}

	// Tab's width when it is a text tab. This isn't correct for artwork tabs, but it's fixed later during the PaintEvent
	int w = fontMetrics().width( _name ) + 10;

        // Register new tab
	widgetDesc d = { _w, activePixmap, inactivePixmap, _name, _tooltip, w } ;
	m_widgets[_idx] = d;

        // Position tab's window
	_w->setFixedSize( width() - 4, height() - TAB_HEIGHT );
	_w->move( 2, TAB_HEIGHT - 1 );
	_w->hide();

	// Show tab's window if it's active
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


// Return the index of the tab at position "pos"
int TabWidget::findTabAtPos( const QPoint *pos )
{

	if( pos->y() > 1 && pos->y() < TAB_HEIGHT - 1 )
	{
               int cx = ( ( m_caption == "" ) ? 4 : 14 ) + fontMetrics().width( m_caption );

		for( widgetStack::iterator it = m_widgets.begin(); it != m_widgets.end(); ++it )
		{
			if( pos->x() >= cx && pos->x() <= cx + ( *it ).nwidth )
			{
				return( it.key() );
			}
			cx += ( *it ).nwidth;
		}
	}

	// Haven't found any tab at position "pos"
	return( -1 );
}


// Overload the QWidget::event handler to display tooltips (from https://doc.qt.io/qt-4.8/qt-widgets-tooltips-example.html)
bool TabWidget::event(QEvent *event)
{

	if ( event->type() == QEvent::ToolTip )
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

		int idx = findTabAtPos( & helpEvent->pos() );

		if ( idx != -1 )
		{
			// Display tab's tooltip
			QToolTip::showText( helpEvent->globalPos(), m_widgets[idx].tooltip );
		} else
		{
			// The tooltip event doesn't relate to any tab, let's ignore it
			QToolTip::hideText();
			event->ignore();
		}

		return true;
	}

	// not a Tooltip event, let's propagate it to the other event handlers
	return QWidget::event(event);	
}


// Activate tab when clicked
void TabWidget::mousePressEvent( QMouseEvent * _me )
{

	// Find index of tab that has been clicked
	QPoint pos = _me->pos();
	int idx = findTabAtPos( &pos );

	// When found, activate tab that has been clicked 
	if ( idx != -1 )
        {
		setActiveTab( idx );
                update();
                return;
	}
}




void TabWidget::resizeEvent( QResizeEvent * )
{
	for( widgetStack::iterator it = m_widgets.begin();
						it != m_widgets.end(); ++it )
	{
		( *it ).w->setFixedSize( width() - 4, height() - TAB_HEIGHT );
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

	// Draw all tabs
  	widgetStack::iterator first = m_widgets.begin();
  	widgetStack::iterator last = m_widgets.end();
	int tab_width = ( width() - tab_x_offset ) / std::distance( first, last ); // Correct tab's width for artwork tabs
        for( widgetStack::iterator it = first ; it != last ; ++it )
        {

		// Draw a text tab or a artwork tab.
		if ( m_usePixmap )
		{	

			// Fixes tab's width, because original size is only correct for text tabs
			( *it ).nwidth = tab_width;

			// Get active or inactive artwork
                	QPixmap *artwork;
			if( it.key() == m_activeTab )
                        {
	                	artwork = new QPixmap( embed::getIconPixmap( ( *it ).activePixmap ) );
				p.fillRect( tab_x_offset, 2, ( *it ).nwidth, TAB_HEIGHT - 3, cap_col );
			} else 
			{
	                	artwork = new QPixmap( embed::getIconPixmap( ( *it ).inactivePixmap ) );
			}

			// Draw artwork
	                p.drawPixmap(tab_x_offset + ( ( *it ).nwidth - ( *artwork ).width() ) / 2, 0, *artwork );

		} else
		{

			// Highlight tab when active
                	if( it.key() == m_activeTab )
	                {
				p.setPen( QColor( 32, 48, 64 ) );
				p.fillRect( tab_x_offset, 2, ( *it ).nwidth - 6, TAB_HEIGHT - 4, cap_col );
			}

			// Draw text
			p.drawText( tab_x_offset + 3, m_tabheight, ( *it ).name );

			// Reset text color
			p.setPen( cap_col );
		}

		// Next tab's horizontal position
		tab_x_offset += ( *it ).nwidth;
        }
}




// Switch between tabs with mouse wheel
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
