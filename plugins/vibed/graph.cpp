/*
 * graph.cpp - a QT widget for displaying and manipulating waveforms
 *
 * Copyright (c) 2006-2007 Andreas Brandmaier <andy/at/brandmaier/dot/de>
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QPaintEvent>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>

#else

#include <qfontmetrics.h>
#include <qpainter.h>
#include <qcursor.h>

#endif


#include "graph.h"
#include "string_pair_drag.h"
#include "sample_buffer.h"
#include <iostream>
#include <cstdlib>

using namespace std;



graph::graph( QWidget * _parent ) :
	QWidget( _parent )
{
	m_mouseDown = false;

	setFixedSize( 132, 104 );
	
	setAcceptDrops( TRUE );

#ifndef QT4
	setBackgroundMode( NoBackground );
#endif
		
}




graph::~graph()
{
}

void graph::setForeground( const QPixmap &_pixmap )
{
	m_foreground = _pixmap;
}

void graph::setSamplePointer( float * _pointer, int _length )
{
	samplePointer = _pointer;
	sampleLength = _length;
	update();
}

void graph::loadSampleFromFile( const QString & _filename )
{
	// zero sample_shape
	for (int i = 0; i < sampleLength; i++)
	{
		samplePointer[i] = 0;
	}
	
	// load user shape
	sampleBuffer buffer( _filename );

	// copy buffer data
	sampleLength = min( sampleLength, static_cast<int>(buffer.frames()) );
	for ( int i = 0; i < sampleLength; i++ )
	{
		samplePointer[i] = (float)*buffer.data()[i];
	}
	
}

void graph::mouseMoveEvent ( QMouseEvent * _me )
{
	// get position
	int x = _me->x();
	int y = _me->y();


	// avoid mouse leaps
	int diff = x - m_lastCursorX;
	
	if( diff >= 1 ) 
	{
		x = m_lastCursorX + 1;
	} 
	else if( diff <= 1 )
	{
		x = m_lastCursorX - 1;
	}
	else 
	{
		x = m_lastCursorX;
	}

	changeSampleAt( x, y );

	// update mouse
	m_lastCursorX = x;
	
}

void graph::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		// toggle mouse state
		m_mouseDown = true;

		// get position
		int x = _me->x();
		int y = _me->y();

		changeSampleAt( x,y );

		// toggle mouse state
		m_mouseDown = true;	
#ifndef QT3
		setCursor( Qt::BlankCursor );
#else
		setCursor( QCursor::BlankCursor );
#endif
		m_lastCursorX = x;
	}
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
	if( _me->button() == Qt::LeftButton )
	{
		// toggle mouse state
		m_mouseDown = false;
#ifndef QT3
		setCursor( Qt::ArrowCursor );
#else
		setCursor( QCursor::ArrowCursor );
#endif
		update();
	}
}	
	


void graph::paintEvent( QPaintEvent * )
{

#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( rect().size() );
	draw_pm.fill( this, rect().topLeft() );
	
	QPainter p( &draw_pm, this );
#endif

	p.setPen( QColor( 0xFF, 0xAA, 0x00 ) );

	p.drawLine( 1+sampleLength, 2, 1+sampleLength, 102);

	float xscale = 128.0 / sampleLength;

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
	p.drawPixmap( 0, 0, m_foreground );

#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif

}


void graph::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );

	if( type == "samplefile" )
	{
		loadSampleFromFile( value );
		_de->accept();
	}
}

void graph::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( stringPairDrag::processDragEnterEvent( _dee,
		QString( "samplefile" ) ) == FALSE )
	{
		_dee->ignore();
	}
}


#include "graph.moc"
