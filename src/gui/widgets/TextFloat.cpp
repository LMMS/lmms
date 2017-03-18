/*
 * TextFloat.cpp - class textFloat, a floating text-label
 *
 * Copyright (c) 2005-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QTimer>
#include <QPainter>
#include <QStyleOption>

#include "TextFloat.h"
#include "gui_templates.h"
#include "GuiApplication.h"
#include "MainWindow.h"


TextFloat::TextFloat() :
	QWidget( gui->mainWindow(), Qt::ToolTip ),
	m_title(),
	m_text(),
	m_pixmap()
{
	resize( 20, 20 );
	hide();

	setAttribute( Qt::WA_TranslucentBackground, true );
	setStyle( QApplication::style() );
	setFont( pointSize<8>( font() ) );
}




void TextFloat::setTitle( const QString & _title )
{
	m_title = _title;
	updateSize();
}




void TextFloat::setText( const QString & _text )
{
	m_text = _text;
	updateSize();
}




void TextFloat::setPixmap( const QPixmap & _pixmap )
{
	m_pixmap = _pixmap;
	updateSize();
}




void TextFloat::setVisibilityTimeOut( int _msecs )
{
	QTimer::singleShot( _msecs, this, SLOT( hide() ) );
	show();
}




TextFloat * TextFloat::displayMessage( const QString & _msg, int _timeout,
					QWidget * _parent, int _add_y_margin )
{
	QWidget * mw = gui->mainWindow();
	TextFloat * tf = new TextFloat;
	if( _parent != NULL )
	{
		tf->moveGlobal( _parent, QPoint( _parent->width() + 2, 0 ) );
	}
	else
	{
		tf->moveGlobal( mw, QPoint( 32, mw->height() - tf->height() - 8 - _add_y_margin ) );
	}
	tf->setText( _msg );
	tf->show();
	if( _timeout > 0 )
	{
		tf->setAttribute( Qt::WA_DeleteOnClose, true );
		QTimer::singleShot( _timeout, tf, SLOT( close() ) );
	}
	return( tf );
}




TextFloat * TextFloat::displayMessage( const QString & _title,
					const QString & _msg,
					const QPixmap & _pixmap,
					int _timeout, QWidget * _parent )
{
	TextFloat * tf = displayMessage( _msg, _timeout, _parent, 16 );
	tf->setTitle( _title );
	tf->setPixmap( _pixmap );
	return( tf );
}




void TextFloat::paintEvent( QPaintEvent * _pe )
{
	QStyleOption opt;
    opt.init( this );
	QPainter p( this );
	p.fillRect( 0, 0, width(), height(), QColor( 0, 0, 0, 0 ) );

/*	p.setPen( p.pen().brush().color() );
	p.setBrush( p.background() );*/

	p.setFont( pointSize<8>( p.font() ) );
	
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );

/*	p.drawRect( 0, 0, rect().right(), rect().bottom() );*/

	if( m_title.isEmpty() )
	{
		p.drawText( opt.rect, Qt::AlignCenter, m_text );
	}
	else
	{
		int text_x = opt.rect.left() + 2;
		int text_y = opt.rect.top() + 12;
		if( m_pixmap.isNull() == false )
		{
			p.drawPixmap( opt.rect.topLeft() + QPoint( 5, 5 ), m_pixmap );
			text_x += m_pixmap.width() + 8;
		}
		p.drawText( text_x, text_y + 16, m_text );
		QFont f = p.font();
		f.setBold( true );
		p.setFont( f );
		p.drawText( text_x, text_y, m_title );
	}
}




void TextFloat::mousePressEvent( QMouseEvent * )
{
	close();
}




void TextFloat::updateSize()
{
	QFontMetrics metrics( pointSize<8>( font() ) );
	QRect textBound = metrics.boundingRect( m_text );
	if( !m_title.isEmpty() )
	{
		QFont f = pointSize<8>( font() );
		f.setBold( true );
		int title_w = QFontMetrics( f ).boundingRect( m_title ).width();
		if( title_w > textBound.width() )
		{
			textBound.setWidth( title_w );
		}
		textBound.setHeight( textBound.height() * 2 + 8 );
	}
	if( m_pixmap.isNull() == false )
	{
		textBound.setWidth( textBound.width() + m_pixmap.width() + 10 );
	}
	resize( textBound.width() + 5, textBound.height()+2 );
	//move( QPoint( parentWidget()->width() + 5, 5 ) );
	update();
}




