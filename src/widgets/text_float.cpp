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
	m_text( "" )
{
#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif

	reparent( parentWidget() );
	resize( 20, 20 );
	hide();
}




void textFloat::reparent( QWidget * _new_parent )
{
	QPoint position = _new_parent->pos();

	// Get position and reparent to either top level or dialog
	//
	while( _new_parent->parentWidget() && !_new_parent->isTopLevel()
		&&
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




void textFloat::paintEvent( QPaintEvent * _pe )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( size() );
	QPainter p( &draw_pm );
#endif
	p.setPen( QColor( 0, 0, 0 ) );
	p.setBrush( QColor( 255, 255, 255 ) );

	p.setFont( pointSize<8>( p.font() ) );

	QFontMetrics metrics( p.fontMetrics() );
	QRect textBound = metrics.boundingRect( m_text );

	resize( textBound.width() + 5, textBound.height() + 5 );
	p.drawRect( rect() );

	p.setPen( Qt::black );
	p.drawText( 2, 10, m_text );

#ifndef QT4
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}




void textFloat::setText( const QString & _text )
{
	m_text = _text;
	repaint();
}


#undef setParent

