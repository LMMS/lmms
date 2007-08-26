#ifndef SINGLE_SOURCE_COMPILE

/*
 * combobox.cpp - implementation of LMMS-combobox
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QtGui/QCursor>
#include <QtGui/QDesktopWidget>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include "combobox.h"
#include "automatable_object_templates.h"
#include "templates.h"
#include "embed.h"
#include "gui_templates.h"


QPixmap * comboBox::s_background = NULL;
QPixmap * comboBox::s_arrow = NULL;

const int CB_ARROW_BTN_WIDTH = 20;


comboBox::comboBox( QWidget * _parent, const QString & _name, track * _track ) :
	QWidget( _parent ),
	automatableObject<int>( _track ),
	m_menu( this ),
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

	connect( &m_menu, SIGNAL( triggered( QAction * ) ),
				this, SLOT( setItem( QAction * ) ) );

	if( _track != NULL )
	{
		getAutomationPattern();
	}
	setInitValue( 0 );
	setAccessibleName( _name );
}




comboBox::~comboBox()
{
}




void comboBox::addItem( const QString & _item, const QPixmap & _pixmap )
{
	QPixmap pm = _pixmap;
	if( pm.height() > 16 )
	{
		pm = pm.scaledToHeight( 16, Qt::SmoothTransformation );
	}
	m_items.push_back( qMakePair( _item, pm ) );
	m_menu.clear();
	for( QVector<item>::iterator it = m_items.begin();
						it != m_items.end(); ++it )
	{
		m_menu.addAction( ( *it ).second, ( *it ).first );
	}
	setRange( 0, m_items.size() - 1 );
}




int comboBox::findText( const QString & _txt ) const
{
	for( QVector<item>::const_iterator it = m_items.begin();
						it != m_items.end(); ++it )
	{
		if( ( *it ).first == _txt )
		{
			return( it - m_items.begin() );
		}
	}
	return( -1 ); 
}




void comboBox::setValue( const int _idx )
{
	automatableObject<int>::setValue( _idx );
/*	m_value = tLimit<int>( _idx, 0, ( m_items.size() > 0 ) ?
						m_items.size() - 1 : 0 );*/
	emit( valueChanged( value() ) );
	emit( activated( ( m_items.size() > 0 ) ?
					m_items[value()].first : "" ) );
	update();
}




void comboBox::contextMenuEvent( QContextMenuEvent * _me )
{
	if( nullTrack() || _me->x() <= width() - CB_ARROW_BTN_WIDTH )
	{
		QWidget::contextMenuEvent( _me );
		return;
	}

	QMenu contextMenu( this );
	contextMenu.setTitle( accessibleName() );
#warning TODO: add css-formatting
#if 0
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( accessibleName() ) + "</b></font>",
			this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "&Open in automation editor" ),
					getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.exec( QCursor::pos() );
}




void comboBox::mousePressEvent( QMouseEvent * _me )
{
	if( _me->x() > width() - CB_ARROW_BTN_WIDTH )
	{
		if( _me->button() == Qt::RightButton )
		{
			return;
		}

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
		setInitValue( value() + 1 );
	}
	else if( _me->button() == Qt::RightButton )
	{
		setInitValue( value() - 1 );
	}
}




void comboBox::paintEvent( QPaintEvent * _pe )
{
	QPainter p( this );

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
		p.setFont( font() );
		p.setClipRect( QRect( 5, 2, width() - CB_ARROW_BTN_WIDTH - 8,
							height() - 2 ) );
		const QPixmap & item_pm = m_items[value()].second;
		int tx = 4;
		if( item_pm.isNull() == FALSE )
		{
			p.drawPixmap( tx, 3, item_pm );
			tx += item_pm.width() + 2;
		}
		p.setPen( QColor( 64, 64, 64 ) );
		p.drawText( tx+1, p.fontMetrics().height()+1,
						m_items[value()].first );
		p.setPen( QColor( 224, 224, 224 ) );
		p.drawText( tx, p.fontMetrics().height(),
						m_items[value()].first );
	}
}




void comboBox::wheelEvent( QWheelEvent * _we )
{
	setInitValue( value() + ( ( _we->delta() < 0 ) ? 1 : -1 ) );
	_we->accept();
}



void comboBox::setItem( QAction * _item )
{
	setInitValue( findText( _item->text() ) );
}




#include "combobox.moc"


#endif
