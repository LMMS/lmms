#ifndef SINGLE_SOURCE_COMPILE

/*
 * combobox.cpp - implementation of LMMS-combobox
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 

#include "combobox.h"

#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QDesktopWidget>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include "caption_menu.h"
#include "embed.h"
#include "gui_templates.h"


QPixmap * comboBox::s_background = NULL;
QPixmap * comboBox::s_arrow = NULL;

const int CB_ARROW_BTN_WIDTH = 20;


comboBox::comboBox( QWidget * _parent, const QString & _name ) :
	QWidget( _parent ),
	autoModelView( new comboBoxModel ),
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

	setFont( pointSize<9>( font() ) );
	m_menu.setFont( pointSize<8>( m_menu.font() ) );

	connect( &m_menu, SIGNAL( triggered( QAction * ) ),
				this, SLOT( setItem( QAction * ) ) );

	setAccessibleName( _name );
}




comboBox::~comboBox()
{
}




void comboBox::contextMenuEvent( QContextMenuEvent * _me )
{
	if( model()->nullTrack() || _me->x() <= width() - CB_ARROW_BTN_WIDTH )
	{
		QWidget::contextMenuEvent( _me );
		return;
	}

	captionMenu contextMenu( accessibleName() );
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "&Open in automation editor" ),
					model()->getAutomationPattern(),
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

		m_menu.clear();
		for( int i = 0; i < model()->size(); ++i )
		{
			m_menu.addAction( model()->itemPixmap( i ) ?
						*model()->itemPixmap( i ) :
							QPixmap(),
						model()->itemText( i ) );
		}

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
		model()->setInitValue( model()->value() + 1 );
	}
	else if( _me->button() == Qt::RightButton )
	{
		model()->setInitValue( model()->value() - 1 );
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

	if( model()->size() > 0 )
	{
		p.setFont( font() );
		p.setClipRect( QRect( 5, 2, width() - CB_ARROW_BTN_WIDTH - 8,
							height() - 2 ) );
		const QPixmap * item_pm = model()->currentData();
		int tx = 4;
		if( item_pm != NULL )
		{
			QPixmap pm = *item_pm;
			if( pm.height() > 16 )
			{
				pm = pm.scaledToHeight( 16,
						Qt::SmoothTransformation );
			}
			p.drawPixmap( tx, 3, pm );
			tx += pm.width() + 2;
		}
		p.setPen( QColor( 64, 64, 64 ) );
		p.drawText( tx+1, p.fontMetrics().height()-3,
						model()->currentText() );
		p.setPen( QColor( 224, 224, 224 ) );
		p.drawText( tx, p.fontMetrics().height()-4,
						model()->currentText() );
	}
}




void comboBox::wheelEvent( QWheelEvent * _we )
{
	model()->setInitValue( model()->value() +
					( ( _we->delta() < 0 ) ? 1 : -1 ) );
	_we->accept();
}




void comboBox::deletePixmap( QPixmap * _pixmap )
{
	delete _pixmap;
}




void comboBox::setItem( QAction * _item )
{
	model()->setInitValue( model()->findText( _item->text() ) );
}









void comboBoxModel::addItem( const QString & _item, QPixmap * _pixmap )
{
	m_items.push_back( qMakePair( _item, _pixmap ) );
	setRange( 0, m_items.size() - 1 );
}




void comboBoxModel::clear( void )
{
	setRange( 0, 0 );
	foreach( const item & _i, m_items )
	{
		emit itemPixmapRemoved( _i.second );
	}
	m_items.clear();
	emit propertiesChanged();
}




int comboBoxModel::findText( const QString & _txt ) const
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




#include "combobox.moc"


#endif
