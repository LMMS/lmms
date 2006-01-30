/*
 * graph.cpp - a QT widget for displaying and manipulating waveforms
 *
 * Copyright (c) 2006 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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

#include <QPaintEvent>
#include <QFontMetrics>
#include <QPainter>

#else

#include <qfontmetrics.h>
#include <qpainter.h>
#include <qcursor.h>

#endif


#include "graph.h"

#include <iostream>
#include <cstdlib>

using namespace std;



graph::graph( const QString & _text, QWidget * _parent) :
	QWidget( _parent )
{

	m_background = NULL;
	m_mouseDown = false;

	setFixedSize( 132, 104 );
	

#ifndef QT4
	setBackgroundMode( NoBackground );
#endif
		
}




graph::~graph()
{
}

void graph::setBackground( const QPixmap &_pixmap )
{
	m_background = _pixmap;
//	setErasePixmap ( m_background );

}

void graph::setSamplePointer( float * _pointer, int _length )
{
	samplePointer = _pointer;
	sampleLength = _length;
	update();
}

void graph::mouseMoveEvent ( QMouseEvent * _me )
{

        // get position
        int x = _me->x();
	int y = _me->y();


	// avoid mouse leaps
	int diff = x - m_lastCursorX;
	
	if (diff >= 1) {
		x = m_lastCursorX + 1;
	} else if (diff <= 1) {
		x = m_lastCursorX - 1;
	} else {
		x = m_lastCursorX;
	}
//	QCursor::setPos( 1, 1 );


	

	changeSampleAt( x, y );

	// update mouse
	m_lastCursorX = x;
	
}

void graph::mousePressEvent( QMouseEvent * _me )
{
	// toggle mouse state
	m_mouseDown = true;

	// get position
	int x = _me->x();
	int y = _me->y();

	changeSampleAt( x,y );

	// toggle mouse state
	m_mouseDown = true;	
	setCursor( QCursor::BlankCursor );
	m_lastCursorX = x;
}

void graph::changeSampleAt(int _x, int _y)
{
	// consider border of background image
	_x -= 2;
	_y -= 2;

        // boundary check
        if (_x < 0) { return; }
        if (_x > sampleLength) { return; }
	if (_y < 0) { return; }
	if (_y >= 100) { return; }
	_y = 100 - _y;

	// change sample shape
        samplePointer[_x] = (_y-50.0)/50.0;
	emit sampleChanged();


}

void graph::mouseReleaseEvent( QMouseEvent * _me )
{
	// toggle mouse state
	m_mouseDown = false;
	setCursor( QCursor::ArrowCursor );
	update();
}	
	


void graph::paintEvent( QPaintEvent * )
{

#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( rect().size() );
//	draw_pm.fill( this, rect().topLeft() );
	
	QPainter p( &draw_pm, this );
#endif

//	if (m_background != NULL) {
		p.drawPixmap( 0, 0, m_background );
//	}
	
	p.setPen( QColor( 0xFF, 0xAA, 0x00 ) );

	p.drawLine( 1+sampleLength, 2, 1+sampleLength, 102);

//	float xscale = 200.0 / sampleLength;
	float xscale = 1.0;

	for (int i=0; i < sampleLength-1; i++)
	{
		p.drawLine(2+static_cast<int>(i*xscale), 
			   2+static_cast<int>(-samplePointer[i]*50) + 50, 
			2+static_cast<int>((i+1)*xscale), 
			2+static_cast<int>(-samplePointer[i+1]*50 + 50)
			);
	}

	// draw Pointer
	if (m_mouseDown) {
		QPoint cursor = mapFromGlobal( QCursor::pos() );
		p.setPen( QColor( 0xAA, 0xFF, 0x00 ) );
		p.drawLine( 2, cursor.y(), 130, cursor.y() );
		p.drawLine( cursor.x(), 2, cursor.x(), 102 );
	}

#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif

}



#include "graph.moc"
