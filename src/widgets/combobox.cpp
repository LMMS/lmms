/*
 * combobox.cpp - implementation of LMMS-combobox
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
 

#include "combobox.h"
#include "templates.h"
#include "embed.h"
#include "gui_templates.h"

#ifndef QT3

#include <QPainter>
#include <QPixmap>
#include <QApplication>
#include <QDesktopWidget>

#else

#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qapplication.h>

#endif


QPixmap * comboBox::s_background = NULL;
QPixmap * comboBox::s_arrow = NULL;

const int CB_ARROW_BTN_WIDTH = 20;


comboBox::comboBox( QWidget * _parent ) :
	QWidget( _parent ),
	m_menu( this ),
	m_currentIndex( 0 ),
	m_pressed( FALSE )
{
	if( s_background == NULL )
	{
		s_background = new QPixmap( embed::getIconPixmap(
							"combobox_bg" ) );
	}

	if( s_arrow == NULL )
	{
		s_arrow = new QPixmap( embed::getIconPixmap(
							"combobox_arrow" ) );
	}

	setFont( pointSize<8>( font() ) );
	m_menu.setFont( pointSize<8>( m_menu.font() ) );

#ifndef QT3
	connect( &m_menu, SIGNAL( triggered( QAction * ) ),
				this, SLOT( setItem( QAction * ) ) );
#else
	connect( &m_menu, SIGNAL( activated( int ) ),
				this, SLOT( setItem( int ) ) );
#endif

#ifdef QT3
	setBackgroundMode( NoBackground );
#endif
}




comboBox::~comboBox()
{
}




void comboBox::addItem( const QString & _item, const QPixmap & _pixmap )
{
	QPixmap pm = _pixmap;
	if( pm.height() > 16 )
	{
#ifndef QT3
		pm = pm.scaledToHeight( 16, Qt::SmoothTransformation );
#else
		pm.convertFromImage( pm.convertToImage().smoothScale(
							pm.width(), 16,
							QImage::ScaleMin ) );
#endif
	}
	m_items.push_back( qMakePair( _item, pm ) );
	m_menu.clear();
	for( vvector<item>::iterator it = m_items.begin();
						it != m_items.end(); ++it )
	{
		m_menu.addItem( ( *it ).second, ( *it ).first
		// when using Qt3, we pass item-index as id for using
		// it in setItem( int ) as index
#ifdef QT3
				, it - m_items.begin()
#endif
				);
	}
}




int comboBox::findText( const QString & _txt ) const
{
	for( vvector<item>::const_iterator it = m_items.begin();
						it != m_items.end(); ++it )
	{
		if( ( *it ).first == _txt )
		{
			return( it - m_items.begin() );
		}
	}
	return( -1 ); 
}




void comboBox::setCurrentIndex( int _idx )
{
	m_currentIndex = tLimit<int>( _idx, 0, ( m_items.size() > 0 ) ?
						m_items.size() - 1 : 0 );
	emit( currentIndexChanged( m_currentIndex ) );
	emit( activated( ( m_items.size() > 0 ) ?
					m_items[m_currentIndex].first : "" ) );
	update();
}




void comboBox::mousePressEvent( QMouseEvent * _me )
{
	if( _me->x() > width() - CB_ARROW_BTN_WIDTH )
	{
		m_pressed = TRUE;
		update();

		QPoint gpos = mapToGlobal( QPoint( 0, height() ) );
		if( gpos.y() + m_menu.sizeHint().height() <
						qApp->desktop()->height() )
		{
			m_menu.exec( gpos );
		}
		else
		{
			m_menu.exec( mapToGlobal( QPoint( width(), 0 ) ) );
		}
		m_pressed = FALSE;
		update();
	}
	else if( _me->button() == Qt::LeftButton )
	{
		setCurrentIndex( currentIndex() + 1 );
	}
	else if( _me->button() == Qt::RightButton )
	{
		setCurrentIndex( currentIndex() - 1 );
	}
}




void comboBox::paintEvent( QPaintEvent * _pe )
{
#ifndef QT3
	QPainter p( this );
#else
	QPixmap draw_pm( rect().size() );
	QPainter p( &draw_pm, this );
#endif
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	for( int x = 2; x < width() - 2; x += s_background->width() )
	{
		p.drawPixmap( x, 2, *s_background );
	}

	p.setPen( QColor( 0, 0, 0 ) );
	p.drawLine( width() - 2, 1, width() - 2, height() - 2 );

	// outer rect
	p.setPen( QColor( 64, 64, 64 ) );
	p.drawRect( 0, 0, width(), height() );

	// button-separator
	p.setPen( QColor( 64, 64, 64 ) );
	p.drawLine( width() - CB_ARROW_BTN_WIDTH - 1, 0, width() -
					CB_ARROW_BTN_WIDTH - 1, height() - 2 );
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawLine( width() - CB_ARROW_BTN_WIDTH, 0, width() -
					CB_ARROW_BTN_WIDTH, height() - 2 );

	// brighter line at bottom/right
	p.setPen( QColor( 160, 160, 160 ) );
	p.drawLine( width() - 1, 0, width() - 1, height() - 1 );
	p.drawLine( 0, height() - 1, width() - 1, height() - 1 );

	const int dxy = ( m_pressed == TRUE ) ? 1 : 0;
	p.drawPixmap( width() - CB_ARROW_BTN_WIDTH + 4 + dxy, 4 + dxy,
								*s_arrow );

	if( m_items.size() > 0 )
	{
		p.setPen( QColor( 224, 224, 224 ) );
		p.setFont( font() );
		p.setClipRect( QRect( 5, 2, width() - CB_ARROW_BTN_WIDTH - 8,
							height() - 2 ) );
		const QPixmap & item_pm = m_items[currentIndex()].second;
		int tx = 4;
		if( item_pm.isNull() == FALSE )
		{
			p.drawPixmap( tx, 3, item_pm );
			tx += item_pm.width() + 2;
		}
		p.drawText( tx, p.fontMetrics().height(),
						m_items[currentIndex()].first );
	}

#ifdef QT3
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}




void comboBox::wheelEvent( QWheelEvent * _we )
{
	setCurrentIndex( currentIndex() + ( ( _we->delta() < 0 ) ? 1 : -1 ) );
	_we->accept();
}



#ifndef QT3

void comboBox::setItem( QAction * _item )
{
	setCurrentIndex( findText( _item->text() ) );
}


void comboBox::setItem( int )
{
}

#else

void comboBox::setItem( QAction * )
{
}


void comboBox::setItem( int _item )
{
	setCurrentIndex( _item );
}


#endif



#include "combobox.moc"

