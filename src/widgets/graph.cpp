/*
 * graph.cpp - a QT widget for displaying and manipulating waveforms
 *
 * Copyright (c) 2006-2007 Andreas Brandmaier <andy/at/brandmaier/dot/de>
 *               2008 Paul Giblock            <drfaygo/at/gmail/dot/com>
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


#include <QtGui/QPaintEvent>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>

#include "graph.h"
#include "string_pair_drag.h"
#include "sample_buffer.h"
#include "oscillator.h"
//#include <cstdlib>
//#include <math.h>

using namespace std;



graph::graph( QWidget * _parent ) :
	QWidget( _parent ),
	/* TODO: size, background? */ 
	modelView( new graphModel( -1.0, 1.0, 128, NULL, NULL, TRUE ) )
{
	m_mouseDown = false;

	resize( 132, 104 );
	setAcceptDrops( TRUE );

	graphModel * gModel = castModel<graphModel>();

	QObject::connect( gModel, SIGNAL( samplesChanged( Uint32, Uint32 ) ),
			this, SLOT( updateGraph( Uint32, Uint32 ) ) );

	QObject::connect( gModel, SIGNAL( lengthChanged( ) ),
			this, SLOT( updateGraph( ) ) );
}


graph::~graph()
{
}



void graph::setForeground( const QPixmap &_pixmap )
{
	m_foreground = _pixmap;
}



/*
void graph::loadSampleFromFile( const QString & _filename )
{
	
	int i;

	// zero sample_shape
	for( i = 0; i < sampleLength; i++ )
	{
		samplePointer[i] = 0;
	}
	
	// load user shape
	sampleBuffer buffer( _filename );

	// copy buffer data
	int trimSize = fmin( size(), static_cast<int>(buffer.frames()) );


	for( i = 0; i < trimSize; i++ )
	{
		samplePointer[i] = (float)*buffer.data()[i];
	}
}
*/



void graph::mouseMoveEvent ( QMouseEvent * _me )
{
	// get position
	int x = _me->x();
	int y = _me->y();

	static bool skip = false;

	if( skip )
	{
		skip = false;
		return;
	}

	// avoid mouse leaps
	int diff = x - m_lastCursorX;
	
	if( diff >= 1 )
	{
		x = min( width() - 2, m_lastCursorX + 1);
	}
	else if( diff <= 1 )
	{
		x = max( 2, m_lastCursorX - 1 );
	}
	else
	{
		x = m_lastCursorX;
	}

	y = max( 2, min( y, height()-3 ) );

	changeSampleAt( x, y );

	// update mouse
	m_lastCursorX = x;

	QPoint pt = mapToGlobal( QPoint( x, y ) );

	QCursor::setPos( pt.x(), pt.y() );

	skip = true;
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

		changeSampleAt( x, y );

		// toggle mouse state
		m_mouseDown = true;
		setCursor( Qt::BlankCursor );
		m_lastCursorX = x;
	}
}



void graph::changeSampleAt(int _x, int _y)
{
	float minVal = model()->minValue();
	float maxVal = model()->maxValue();

	if ( width() <= 4 )
	{
		return;
	}

	float xscale = static_cast<float>( model()->length() ) /
					( width()-4 );

	// consider border of background image
	_x -= 2;
	_y -= 2;

	// subtract max from min because Qt's Y-axis is backwards
	float range = minVal - maxVal;
	float val = ( _y*range/( height()-4 ) ) + maxVal;

	model()->setSampleAt( _x*xscale, val );
}



void graph::mouseReleaseEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		// toggle mouse state
		m_mouseDown = false;
		setCursor( Qt::ArrowCursor );
		update();
	}
}	



void graph::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.setPen( QPen(QColor( 0xFF, 0xAA, 0x00 ), 1) );

	QVector<float> * samps = &(model()->m_samples);
	int length = model()->length();
	int maxVal = model()->maxValue();

	float xscale = (float)( width()-4 ) / length;
	float yscale = (float)( height()-4 ) / ( model()->minValue() - maxVal );

	// Max index, more useful below
	length--;

	p.setRenderHints( QPainter::Antialiasing, TRUE );
	for( int i=0; i < length; i++ )
	{
		// Needs to be rewritten
		p.drawLine(2+static_cast<int>(i*xscale), 
			2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale ),
			2+static_cast<int>((i+1)*xscale), 
			2+static_cast<int>( ( (*samps)[i+1] - maxVal ) * yscale )
			);
	}

	// Draw last segment flat
	p.drawLine(2+static_cast<int>(length*xscale), 
		2+static_cast<int>( ( (*samps)[length] - maxVal ) * yscale ),
		width()-2,
		2+static_cast<int>( ( (*samps)[0] - maxVal ) * yscale ) );

	p.setRenderHints( QPainter::Antialiasing, FALSE );

	// draw Pointer
	if( m_mouseDown ) 
	{
		QPoint cursor = mapFromGlobal( QCursor::pos() );
		p.setPen( QColor( 0xAA, 0xFF, 0x00 ) );
		p.drawLine( 2, cursor.y(), width()-2, cursor.y() );
		p.drawLine( cursor.x(), 2, cursor.x(), height()-2 );
	}
	p.drawPixmap( 0, 0, m_foreground );
}



void graph::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );

	if( type == "samplefile" )
	{
		// TODO: call setWaveToUser
		// loadSampleFromFile( value );
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



void graph::modelChanged( void )
{
	graphModel * gModel = castModel<graphModel>();

	QObject::connect( gModel, SIGNAL( samplesChanged( Uint32, Uint32 ) ),
			this, SLOT( updateGraph( Uint32, Uint32 ) ) );

	QObject::connect( gModel, SIGNAL( lengthChanged( ) ),
			this, SLOT( updateGraph( ) ) );
}


void graph::updateGraph( Uint32 _startPos, Uint32 _endPos )
{
	// Can optimize by only drawing changed position
	update();
}


void graph::updateGraph( void )
{
    updateGraph( 0, model()->length() - 1 );
}


graphModel::graphModel( float _min, float _max, Uint32 _length,
		::model * _parent, track * _track,
		bool _default_constructed ) :
	model( _parent, _default_constructed ),
	m_samples( _length ),
	m_minValue( _min ),
	m_maxValue( _max )
{
}



graphModel::~graphModel()
{
}



void graphModel::setRange( float _min, float _max )
{
	if( _min != m_minValue || _max != m_maxValue )
	{
		m_minValue = _min;
		m_maxValue = _max;

		if( !m_samples.isEmpty() )
		{
			// Trim existing values 
			for( Uint32 i=0; i < length(); i++ )
			{
				m_samples[i] = fmaxf( _min, fminf( m_samples[i], _max ) );
			}
		}

		emit rangeChanged();
	}
}



void graphModel::setLength( Uint32 _length )
{
	if( _length != length() )
	{
		m_samples.resize( _length );
		emit lengthChanged();
	}
}



void graphModel::setSampleAt( Uint32 _x, float _val )
{
	// boundary check
	if ( _x >= 0 && _x < length() &&
			_val >= minValue() && _val < maxValue() )
	{

		// change sample shape
		m_samples[_x] = _val;
		emit samplesChanged( _x, _x );
	}
}



void graphModel::setSamples( const float * _samples )
{
	qCopy( _samples, _samples + length(), m_samples.begin());

	emit samplesChanged( 0, length()-1 );
}



void graphModel::setWaveToSine( void )
{
	for( Uint32 i = 0; i < length(); i++ )
	{
		m_samples[i] = oscillator::sinSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};



void graphModel::setWaveToTriangle( void )
{
	for( Uint32 i = 0; i < length(); i++ )
	{
		m_samples[i] = oscillator::triangleSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};



void graphModel::setWaveToSaw( void )
{
	for( Uint32 i = 0; i < length(); i++ )
	{
		m_samples[i] = oscillator::sawSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};



void graphModel::setWaveToSquare( void )
{
	for( Uint32 i = 0; i < length(); i++ )
	{
		m_samples[i] = oscillator::squareSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};



void graphModel::setWaveToNoise( void )
{
	for( Uint32 i = 0; i < length(); i++ )
	{
		m_samples[i] = oscillator::noiseSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};



void graphModel::smooth( void )
{
	// store values in temporary array
	QVector<float> temp = m_samples;

	// Smoothing
	m_samples[0] = ( temp[0] + temp[length()-1] ) * 0.5f;
	for ( Uint32 i=1; i < length(); i++ )
	{
		m_samples[i] = ( temp[i-1] + temp[i] ) * 0.5f; 	
	}

	emit samplesChanged(0, length()-1);
}



void graphModel::normalize( void )
{
	float max = 0.0001f;
	for( Uint32 i = 0; i < length(); i++ )
	{
		if( fabsf(m_samples[i]) > max && m_samples[i] != 0.0f )
		{
			max = fabs( m_samples[i] );
		}
	}

	for( Uint32 i = 0; i < length(); i++ )
	{
		m_samples[i] /= max;
	}

	if( max != 1.0f ) {
		emit samplesChanged( 0, length()-1 );
	}
}	




#include "graph.moc"
