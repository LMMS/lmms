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
#include "types.h"

class graphModel;
class track;

class graph : public QWidget, public modelView
{
	Q_OBJECT
public:
	graph( QWidget * _parent );
	virtual ~graph();

	void setForeground( const QPixmap & _pixmap );
//	void loadSampleFromFile( const QString & _filename );

	virtual inline graphModel * model( void ) {
		return castModel<graphModel>();
	}


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void dropEvent( QDropEvent * _de );
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );

protected slots:
	void updateGraph( Uint32 _startPos, Uint32 _endPos );
    void updateGraph( void );

private:
	virtual void modelChanged( void );

	void changeSampleAt(int _x, int _y);


	QPixmap m_foreground;
	
	graphModel * m_graphModel;
	
	bool m_mouseDown;
	int m_lastCursorX;

} ;


class graphModel : public model
{
	Q_OBJECT
public:
	graphModel( float _min,
			float _max,
			Uint32 _size,
			:: model * _parent, track * _track = NULL,
			bool _default_constructed = FALSE );

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

	inline Uint32 length( void ) const
	{
		return( m_samples.count() );
	}
	
	inline const float* samples( void ) const
	{
		return( m_samples.data() );
	}

public slots:
	void setRange( float _min, float _max );

	void setLength( Uint32 _size );

	void setSampleAt( Uint32 _samplePos, float _value );
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
	void samplesChanged( Uint32 startPos, Uint32 endPos );
	void rangeChanged( void );

private:

	QVector<float> m_samples;
	float m_minValue;
	float m_maxValue;

	friend class graph;

};

#endif
