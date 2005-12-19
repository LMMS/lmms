/*
 * text_float.cpp - class textFloat, a floating text-label
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QTimer>

#else

#include <qpainter.h>
#include <qpixmap.h>
#include <qtimer.h>

#define setParent reparent

#endif


#include "text_float.h"
#include "gui_templates.h"



textFloat::textFloat( QWidget * _parent ) :
	QWidget( _parent
#ifndef QT4
	, "textFloat", WStyle_Customize  | WStyle_NoBorder | WStyle_StaysOnTop
#endif
		),
	m_title( "" ),
	m_text( "" ),
	m_pixmap()
{
#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif

	reparent( parentWidget() );
	resize( 20, 20 );
	hide();

	setFont( pointSize<8>( font() ) );
}




void textFloat::setTitle( const QString & _title )
{
	m_title = _title;
	updateSize();
}




void textFloat::setText( const QString & _text )
{
	m_text = _text;
	updateSize();
}




void textFloat::setPixmap( const QPixmap & _pixmap )
{
	m_pixmap = _pixmap;
	updateSize();
}




void textFloat::reparent( QWidget * _new_parent )
{
	if( _new_parent == NULL )
	{
		return;
	}
	QPoint position = _new_parent->pos();

	// Get position and reparent to either top level or dialog
	//
	while( _new_parent->parentWidget() && !_new_parent->isTopLevel() &&
#ifdef QT4
			!_new_parent->windowType() == Qt::Dialog
#else
			!_new_parent->isDialog()
#endif
						)
	{
		_new_parent = _new_parent->parentWidget();
		position += _new_parent->pos();
	}

	// Position this widget to the right of the parent
	//
	//move(pos + QPoint(parent->width() + 5, 5));

	QWidget::setParent( _new_parent,
#ifdef QT4
				Qt::FramelessWindowHint |
						Qt::WindowStaysOnTopHint );
#else
				WStyle_Customize | WStyle_NoBorder |
							WStyle_StaysOnTop,
				position + QPoint( 20, 5 ) );
#endif
}




void textFloat::setVisibilityTimeOut( int _msecs )
{
	QTimer::singleShot( _msecs, this, SLOT( hide() ) );
	show();
}




textFloat * textFloat::displayMessage( const QString & _msg, int _timeout,
					QWidget * _parent, int _add_y_margin )
{
#ifdef QT4
	QWidget * mw = QApplication::activeWindow();
#else
	QWidget * mw = qApp->mainWidget();
#endif
	textFloat * tf = new textFloat( ( _parent == NULL ) ? mw : _parent );
	if( _parent != NULL )
	{
		tf->move( _parent->mapTo( _parent->topLevelWidget(),
							QPoint( 0, 0 ) ) +
				QPoint( _parent->width() + 2, 0 ) );
	}
	else
	{
		tf->move( 32, mw->height() - 28 - _add_y_margin );
	}
	tf->setText( _msg );
	tf->show();
	if( _timeout > 0 )
	{
#ifdef QT4
		tf->setAttribute( Qt::WA_DeleteOnClose, TRUE );
#else
		tf->setWFlags( tf->getWFlags() | Qt::WDestructiveClose );
#endif
		QTimer::singleShot( _timeout, tf, SLOT( close() ) );
	}
	return( tf );
}




textFloat * textFloat::displayMessage( const QString & _title,
					const QString & _msg,
					const QPixmap & _pixmap,
					int _timeout, QWidget * _parent )
{
	textFloat * tf = displayMessage( _msg, _timeout, _parent, 16 );
	tf->setTitle( _title );
	tf->setPixmap( _pixmap );
	return( tf );
}




void textFloat::paintEvent( QPaintEvent * _pe )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( size() );
	QPainter p( &draw_pm );
#endif
	p.setPen( QColor( 0, 0, 0 ) );
	p.setBrush( QColor( 224, 224, 224 ) );

	p.setFont( pointSize<8>( p.font() ) );

	p.drawRect( rect() );

//	p.setPen( Qt::black );
	// small message?
	if( m_title == "" )
	{
		p.drawText( 2, 10, m_text );
	}
	else
	{
		int text_x = 2;
		if( m_pixmap.isNull() == FALSE )
		{
			p.drawPixmap( 5, 5, m_pixmap );
			text_x += m_pixmap.width() + 8;
		}
		p.drawText( text_x, 28, m_text );
		QFont f = p.font();
		f.setBold( TRUE );
		p.setFont( f );
		p.drawText( text_x, 12, m_title );
	}

#ifndef QT4
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}




void textFloat::mousePressEvent( QMouseEvent * )
{
	close();
}




void textFloat::updateSize( void )
{
	QFontMetrics metrics( font() );
	QRect textBound = metrics.boundingRect( m_text );
	if( m_title != "" )
	{
		QFont f = font();
		f.setBold( TRUE );
		int title_w = QFontMetrics( f ).boundingRect( m_title ).width();
		if( title_w > textBound.width() )
		{
			textBound.setWidth( title_w );
		}
		textBound.setHeight( textBound.height() * 2 + 14 );
	}
	if( m_pixmap.isNull() == FALSE )
	{
		textBound.setWidth( textBound.width() + m_pixmap.width() + 10 );
	}
	resize( textBound.width() + 5, textBound.height() + 5 );
	//move( QPoint( parentWidget()->width() + 5, 5 ) );
	repaint();
}



#undef setParent

