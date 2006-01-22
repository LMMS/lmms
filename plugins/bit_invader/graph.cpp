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

#endif


#include "graph.h"

#include <iostream>
#include <cstdlib>

using namespace std;

graph::graph( const QString & _text, QWidget * _parent) :
	QWidget( _parent )
{

	m_background = NULL;

	setFixedSize( 132, 104 );
		

	
}




graph::~graph()
{
//	delete m_background;
}

void graph::setBackground( const QPixmap &_pixmap )
{
	m_background = _pixmap;
	setErasePixmap ( m_background );

}

void graph::setSamplePointer( float * _pointer, int _length )
{
	samplePointer = _pointer;
	sampleLength = _length;
}

void graph::mouseMoveEvent ( QMouseEvent * _me )
{


        // get position
        int x = _me->x();
        if (x < 0) { return; }
        if (x > sampleLength) { return; }

	int y = _me->y();
	if (y < 0) { return; }
	if (y >= 100) { return; }

	y = 100 - y;

        samplePointer[x] = (y-50.0)/50.0;

	emit sampleChanged();

	
}

void graph::mousePressEvent( QMouseEvent * _me )
{
/*	if( _me->button() == Qt::LeftButton )
	{
		toggle();
	}*/


	// get position
	int x = _me->x();
	if (x < 0) { return; }
	if (x > sampleLength) { return; }

	int y = _me->y();
	if (y < 0) { return; }
	if (y >= 100) { return; }

	y = 100 - y;

	samplePointer[x] = (y-50.0)/50.0;

	emit sampleChanged();
	

}
	
	
void sampleChanged() {}	


void graph::paintEvent( QPaintEvent * )
{

#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( rect().size() );
	draw_pm.fill( this, rect().topLeft() );
	
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


#ifndef QT4
	// and blit all the drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif

}



#include "graph.moc"
