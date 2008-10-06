/*
 * graph.h - a QT widget for displaying and manipulating waveforms
 *
 * Copyright (c) 2006-2007 Andreas Brandmaier <andy/at/brandmaier/dot/de>
 *               2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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

#include <QtGui/QWidget>
#include <QtGui/QPixmap>
#include <QtGui/QCursor>

#include "mv_base.h"
#include "lmms_basics.h"

class graphModel;


class EXPORT graph : public QWidget, public modelView
{
	Q_OBJECT
public:
	enum graphStyle
	{
		NearestStyle,
		LinearStyle,
		NumGraphStyles
	};

	graph( QWidget * _parent, graphStyle _style = graph::LinearStyle );
	virtual ~graph();

	void setForeground( const QPixmap & _pixmap );

	void setGraphColor( const QColor );

	inline graphModel * model( void )
	{
		return castModel<graphModel>();
	}

	inline graphStyle getGraphStyle( void )
	{
		return m_graphStyle;
	}

	inline void setGraphStyle( graphStyle _s )
	{
		m_graphStyle = _s;
		update();
	}


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void dropEvent( QDropEvent * _de );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );

protected slots:
	void updateGraph( int _startPos, int _endPos );
	void updateGraph( void );

private:
	virtual void modelChanged( void );

	void changeSampleAt(int _x, int _y);


	QPixmap m_foreground;
	QColor m_graphColor;
	
	graphModel * m_graphModel;
	graphStyle m_graphStyle;
	
	bool m_mouseDown;
	int m_lastCursorX;

} ;


class EXPORT graphModel : public model
{
	Q_OBJECT
public:
	graphModel( float _min,
			float _max,
			int _size,
			:: model * _parent,
			bool _default_constructed = FALSE,
			float _step = 0.0 );

	virtual ~graphModel();

	// TODO: saveSettings, loadSettings?
	
	inline float minValue( void ) const
	{
		return( m_minValue );
	}

	inline float maxValue( void ) const
	{
		return( m_maxValue );
	}

	inline int length( void ) const
	{
		return( m_samples.count() );
	}
	
	inline const float * samples( void ) const
	{
		return( m_samples.data() );
	}

public slots:
	void setRange( float _min, float _max );

	void setLength( int _size );

	void setSampleAt( int _samplePos, float _value );
	void setSamples( const float * _value );

	void setWaveToSine( void );
	void setWaveToTriangle( void );
	void setWaveToSaw( void );
	void setWaveToSquare( void );
	void setWaveToNoise( void );
	//void setWaveToUser( );
	
	void smooth( void );
	void normalize( void );

signals:
	void lengthChanged( void );
	void samplesChanged( int startPos, int endPos );
	void rangeChanged( void );

private:

	QVector<float> m_samples;
	float m_minValue;
	float m_maxValue;
	float m_step;

	friend class graph;

};

#endif
