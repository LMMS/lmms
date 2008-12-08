/*
 * collapsible_widget.cpp - implementation of FLUIQ::CollapsibleWidget
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
#include <QtGui/QMouseEvent> 
#include <QtGui/QPainter> 

#include "fluiq/collapsible_widget.h"

#include "embed.h"


// implementation of FLUIQ::CollapsibleWidgetHeader

const int HEADER_MIN_HEIGHT = 20;

FLUIQ::CollapsibleWidgetHeader::CollapsibleWidgetHeader(
						CollapsibleWidget * _parent ) :
	Widget( _parent ),
	m_parent( _parent ),
	m_hovered( false ),
	m_pressed( false ),
	m_collapsed( false ),
	m_moved( false ),
	m_arrowCollapsed( embed::getIconPixmap( "fluiq/arrow-right" ) ),
	m_arrowExpanded( embed::getIconPixmap( "fluiq/arrow-down" ) )
{
	QFont f = font();
	f.setBold( true );
	setFont( f );

	setAttribute( Qt::WA_OpaquePaintEvent, true );

	if( m_parent->orientation() == Qt::Horizontal )
	{
		setFixedWidth( HEADER_MIN_HEIGHT );
	}
	else
	{
		setFixedHeight( HEADER_MIN_HEIGHT );
	}
}




FLUIQ::CollapsibleWidgetHeader::~CollapsibleWidgetHeader()
{
}




void FLUIQ::CollapsibleWidgetHeader::setCollapsed( bool _c )
{
	if( _c != m_collapsed )
	{
		m_collapsed = _c;
		update();
	}
}




QSize FLUIQ::CollapsibleWidgetHeader::sizeHint( void ) const
{
	if( m_parent->orientation() == Qt::Horizontal )
	{
		return QSize( HEADER_MIN_HEIGHT,
				HEADER_MIN_HEIGHT +
					fontMetrics().width( windowTitle() ) );
	}
	return QSize( HEADER_MIN_HEIGHT + fontMetrics().width( windowTitle() ),
							HEADER_MIN_HEIGHT );
}




void FLUIQ::CollapsibleWidgetHeader::enterEvent( QEvent * _ev )
{
	m_hovered = true;
	update();

	Widget::enterEvent( _ev );
}




void FLUIQ::CollapsibleWidgetHeader::leaveEvent( QEvent * _ev )
{
	m_hovered = false;
	update();

	Widget::leaveEvent( _ev );
}




void FLUIQ::CollapsibleWidgetHeader::mousePressEvent( QMouseEvent * _ev )
{
	if( _ev->button() == Qt::LeftButton )
	{
		QApplication::setOverrideCursor(
			m_parent->orientation() == Qt::Vertical ?
					Qt::SizeVerCursor : Qt::SizeHorCursor );
		m_pressed = true;
		m_moved = false;
		m_origMousePos = _ev->pos();
		update();
		_ev->accept();
	}
	else
	{
		Widget::mousePressEvent( _ev );
	}
}




void FLUIQ::CollapsibleWidgetHeader::mouseMoveEvent( QMouseEvent * _event )
{
	if( m_pressed )
	{
		m_moved = true;

		QSize ms = m_parent->minimumSize();

		if( m_parent->orientation() == Qt::Vertical )
		{
			const int dy = _event->y() - m_origMousePos.y();
			ms.setHeight( qMax( 0, ms.height() + dy ) );
		}
		else
		{
			const int dx = _event->x() - m_origMousePos.x();
			ms.setWidth( qMax( 0, ms.width() + dx ) );
		}
		m_parent->setMinimumSize( ms );
		m_origMousePos = _event->pos();
	}
}




void FLUIQ::CollapsibleWidgetHeader::mouseReleaseEvent( QMouseEvent * _ev )
{
	if( _ev->button() == Qt::LeftButton && m_pressed )
	{
		QApplication::restoreOverrideCursor();
		m_pressed = false;
		update();
		_ev->accept();

		if( m_moved == false )
		{
			if( m_collapsed )
			{
				m_collapsed = false;
				emit expanded();
			}
			else
			{
				m_collapsed = true;
				emit collapsed();
			}
		}
	}
	else
	{
		Widget::mousePressEvent( _ev );
	}
}




void FLUIQ::CollapsibleWidgetHeader::paintEvent( QPaintEvent * _ev )
{
	QPainter p( this );
	QRect r = rect();
	if( m_parent->orientation() == Qt::Horizontal )
	{
		r = QRect( 0, 0, height(), width() );
		p.rotate( -90 );
		p.translate( -height(), 0 );
	}

	QLinearGradient grad( 0, 0, 0, r.height() );
	QColor c = palette().color( QPalette::Window );
	if( m_hovered )
	{
		c = c.lighter( 200 );
	}
	grad.setColorAt( 0, c.lighter( 280 ) );
	grad.setColorAt( 1, c );

	p.fillRect( r, grad );

	p.setPen( palette().color( QPalette::Text ) );
	p.drawPixmap( 8, 5, m_collapsed ? m_arrowCollapsed : m_arrowExpanded );
	p.drawText( 20, 10+r.height()-p.fontMetrics().height(), windowTitle() );
}






// implementation of FLUIQ::CollapsibleWidget


FLUIQ::CollapsibleWidget::CollapsibleWidget( Qt::Orientation _or,
							Widget * _parent ) :
	Widget( _parent ),
	m_orientation( _or ),
	m_masterLayout( NULL )
{
	if( m_orientation == Qt::Horizontal )
	{
		m_masterLayout = new QHBoxLayout( this );
	}
	else
	{
		m_masterLayout = new QVBoxLayout( this );
	}
	m_masterLayout->setMargin( 0 );
	m_masterLayout->setSpacing( 0 );

	m_header = new CollapsibleWidgetHeader( this );

	m_masterLayout->addWidget( m_header );

	connect( m_header, SIGNAL( expanded() ),
			this, SLOT( expand() ) );
	connect( m_header, SIGNAL( collapsed() ),
			this, SLOT( collapse() ) );
}




FLUIQ::CollapsibleWidget::~CollapsibleWidget()
{
}




void FLUIQ::CollapsibleWidget::addWidget( QWidget * _w )
{
	m_masterLayout->addWidget( _w );
}




void FLUIQ::CollapsibleWidget::insertWidget( int _idx, QWidget * _w )
{
	m_masterLayout->insertWidget( _idx, _w );
}




void FLUIQ::CollapsibleWidget::expand( void )
{
	m_header->setCollapsed( false );

	// set size properties
	setMaximumSize( QSize( 1 << 16, 1 << 16 ) );

	// show all children
	foreach( QWidget * w, findChildren<QWidget *>() )
	{
		if( w != m_header && w->parentWidget() == this )
		{
			w->show();
		}
	}
	setMinimumSize( sizeHint() );
}



void FLUIQ::CollapsibleWidget::collapse( void )
{
	m_header->setCollapsed( true );

	// hide all children
	foreach( QWidget * w, findChildren<QWidget *>() )
	{
		if( w != m_header && w->parentWidget() == this )
		{
			w->hide();
		}
	}

	// set size properties
	const QSize headerSize = m_header->sizeHint();
	if( m_orientation == Qt::Vertical )
	{
		setMaximumSize( QSize( 1 << 16, headerSize.height() ) );
	}
	else
	{
		setMaximumSize( QSize( headerSize.width(), 1 << 16 ) );
	}
	setMinimumSize( headerSize );
}




#include "fluiq/moc_collapsible_widget.cxx"

