/*
 * graph.h - a QT widget for displaying and manipulating waveforms
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _GRAPH_H
#define _GRAPH_H

#include "qt3support.h"

#ifdef QT4

#include <QtGui/QWidget>
#include <QtGui/QPixmap>
#include <QtGui/QCursor>

#else

#include <qwidget.h>
#include <qpixmap.h>
#include <qcursor.h>

#endif


#include "engine.h"


class graph : public QWidget, public engineObject
{
	Q_OBJECT
public:
	graph( QWidget * _parent, engine * _engine );
	virtual ~graph();

	void setSamplePointer( float * pointer, int length );
	void setForeground( const QPixmap & _pixmap );
	void loadSampleFromFile( const QString & _filename );

signals:
	void sampleSizeChanged( float f );
	void sampleChanged( void );

protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void dropEvent( QDropEvent * _de );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );

private:

	void changeSampleAt(int _x, int _y);

	QPixmap m_foreground;
	
	
	float *samplePointer;
	int sampleLength;

	bool m_mouseDown;
	int m_lastCursorX;

} ;

#endif
