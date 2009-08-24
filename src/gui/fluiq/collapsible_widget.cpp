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
#include <QtGui/QMenu> 
#include <QtGui/QMouseEvent> 
#include <QtGui/QPainter> 

#include "fluiq/collapsible_widget.h"
#include "fluiq/splitter.h"

#include "embed.h"


// implementation of FLUIQ::CollapsibleWidgetHeader

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
		setFixedWidth( MinimalHeight );
	}
	else
	{
		setFixedHeight( MinimalHeight );
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




QSize FLUIQ::CollapsibleWidgetHeader::sizeHint() const
{
	if( m_parent->orientation() == Qt::Horizontal )
	{
		return QSize( MinimalHeight,
				MinimalHeight +
					fontMetrics().width(
						m_parent->windowTitle() ) );
	}
	return QSize( MinimalHeight +
			fontMetrics().width( m_parent->windowTitle() ),
							MinimalHeight );
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
		m_origMousePos = _ev->globalPos();
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

		QPoint pos = _event->globalPos();

		Splitter * s = NULL;
		if( m_parent->parentWidget() &&
			( s = qobject_cast<Splitter *>
					( m_parent->parentWidget() ) ) )
		{
			int idx = s->indexOf( m_parent );
			QWidget * sibling = NULL;
			CollapsibleWidget * collapsibleSibling;

			// look whether we have expanded left-hand siblings
			while( idx > 0 && sibling == NULL )
			{
				--idx;
				sibling = s->widget( idx );
				collapsibleSibling =
					qobject_cast<CollapsibleWidget *>(
								sibling );
				if( collapsibleSibling &&
					collapsibleSibling->isCollapsed() )
				{
					sibling = NULL;
				}
			}

			// found one?
			if( sibling )
			{
				QSize minSizeHint = sibling->minimumSizeHint();
				QSize sizeHint = sibling->sizeHint();
				QSize minSize = sibling->size();
				QSize maxSize = sibling->maximumSize();
				// then increase size according to orientation
				if( m_parent->orientation() == Qt::Vertical )
				{
const int dy = pos.y() - m_origMousePos.y();
minSize.setHeight( qMax( minSizeHint.height(), minSize.height() + dy ) );
// implement snapping when reaching size hint
if( dy < 0 && ( ( minSize.height() < sizeHint.height() &&
				sizeHint.height() - minSize.height() < 20 )
		 || ( sibling->size().height() == minSize.height() ) ) )
{
	return;
}
maxSize.setHeight( minSize.height() );
				}
				else
				{
const int dx = pos.x() - m_origMousePos.x();
minSize.setWidth( qMax( minSizeHint.width(), minSize.width() + dx ) );
// implement snapping when reaching size hint
if( dx < 0 && ( ( minSize.width() < sizeHint.width() &&
				sizeHint.width() - minSize.width() < 20 )
		 || ( sibling->size().width() == minSize.width()  ) ) )
{
	return;
}
maxSize.setWidth( minSize.width() );
				}
				sibling->setMinimumSize( minSize );
				sibling->setMaximumSize( maxSize );
				s->updateGeometry();
			}
			m_origMousePos = pos;
		}
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
	p.drawText( 20, 10+r.height()-p.fontMetrics().height(),
						m_parent->windowTitle() );
}






// implementation of FLUIQ::CollapsibleWidget


FLUIQ::CollapsibleWidget::CollapsibleWidget( Qt::Orientation _or,
							Widget * _parent ) :
	Widget( _parent ),
	m_orientation( _or ),
	m_origMinSize(),
	m_origMaxSize(),
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
	setSizePolicy( QSizePolicy::Expanding,
					QSizePolicy::Expanding );
	m_header = new CollapsibleWidgetHeader( this );

	m_masterLayout->addWidget( m_header );
	m_masterLayout->insertStretch( 100, 0 );

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
	m_masterLayout->insertWidget( m_masterLayout->count()-1, _w, 1 );
}




void FLUIQ::CollapsibleWidget::insertWidget( int _idx, QWidget * _w )
{
	m_masterLayout->insertWidget( _idx, _w, 1 );
}




void FLUIQ::CollapsibleWidget::expand()
{
	m_header->setCollapsed( false );

	// set size properties
	setMaximumSize( m_origMaxSize );

	// show all children
	foreach( QWidget * w, findChildren<QWidget *>() )
	{
		if( w != m_header && w->parentWidget() == this )
		{
			w->show();
		}
	}

	// parent has gotten smaller while we were collapsed?
	if( m_orientation == Qt::Horizontal )
	{
		// then re-adjust height
		if( m_origMinSize.height() > sizeHint().height() )
		{
			m_origMinSize.setHeight( sizeHint().height() );
		}
	}
	else
	{
		// then re-adjust width
		if( m_origMinSize.width() > sizeHint().width() )
		{
			m_origMinSize.setWidth( sizeHint().width() );
		}
	}
	setMinimumSize( m_origMinSize );
}




void FLUIQ::CollapsibleWidget::collapse()
{
	m_header->setCollapsed( true );

	m_origMinSize = minimumSize();
	m_origMaxSize = maximumSize();

	// hide all children
	foreach( QWidget * w, findChildren<QWidget *>() )
	{
		if( w != m_header && w->parentWidget() == this &&
				qobject_cast<QMenu *>( w ) == NULL )
		{
			w->hide();
		}
	}

	// set size properties
	const QSize origSize = size();
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


/*	// now try to redistribute freed space amongst sibling widgets
	Splitter * s = NULL;
	if( parentWidget() &&
			( s = qobject_cast<Splitter *>( parentWidget() ) ) )
	{
		int numExpanded = 0;
		for( int i = 0; i < s->count(); ++i )
		{
			CollapsibleWidget * cw =
				qobject_cast<CollapsibleWidget *>(
							s->widget( i ) );
			if( cw && cw->isCollapsed() == false )
			{
				++numExpanded;
			}
		}
		if( numExpanded == 0 )
		{
			return;
		}
		QSize addSizePerSibling = ( origSize - size() ) / numExpanded;
		if( m_orientation == Qt::Vertical )
		{
			addSizePerSibling.setWidth( 0 );
		}
		else
		{
			addSizePerSibling.setHeight( 0 );
		}
		for( int i = 0; i < s->count(); ++i )
		{
			CollapsibleWidget * cw =
				qobject_cast<CollapsibleWidget *>(
							s->widget( i ) );
			if( cw && cw->isCollapsed() == false )
			{
				cw->setMaximumSize( cw->maximumSize() +
							addSizePerSibling );
			}
		}
	}*/
}




#include "fluiq/moc_collapsible_widget.cxx"

