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
#include "SampleBuffer.h"
#include "Oscillator.h"
#include "engine.h"


graph::graph( QWidget * _parent, graphStyle _style ) :
	QWidget( _parent ),
	/* TODO: size, background? */
	ModelView( new graphModel( -1.0, 1.0, 128, NULL, true ), this ),
	m_graphStyle( _style )
{
	m_mouseDown = false;
	m_graphColor = QColor( 0xFF, 0xAA, 0x00 );

	resize( 132, 104 );
	setAcceptDrops( true );
	setCursor( Qt::CrossCursor );

	graphModel * gModel = castModel<graphModel>();

	QObject::connect( gModel, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( updateGraph( int, int ) ) );

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

void graph::setGraphColor( QColor _graphcol )
{
	m_graphColor = _graphcol;
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
		x = qMin( width() - 2, m_lastCursorX + 1);
	}
	else if( diff <= 1 )
	{
		x = qMax( 2, m_lastCursorX - 1 );
	}
	else
	{
		x = m_lastCursorX;
	}

	y = qMax( 2, qMin( y, height()-3 ) );

	changeSampleAt( x, y );

	// update mouse
	if( diff != 0 )
	{
		m_lastCursorX = x;

		QPoint pt = mapToGlobal( QPoint( x, y ) );

		QCursor::setPos( pt.x(), pt.y() );
	}

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

	model()->setSampleAt( (int)( _x*xscale ), val );
}



void graph::mouseReleaseEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		// toggle mouse state
		m_mouseDown = false;
		setCursor( Qt::CrossCursor );
		update();
	}
}	



void graph::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.setPen( QPen( m_graphColor, 1 ) );

	QVector<float> * samps = &(model()->m_samples);
	int length = model()->length();
	const float maxVal = model()->maxValue();

	float xscale = (float)( width()-4 ) / length;
	float yscale = (float)( height()-4 ) / ( model()->minValue() - maxVal );

	// Max index, more useful below
	length--;


	switch( m_graphStyle )
	{
		case graph::LinearStyle:
			p.setRenderHints( QPainter::Antialiasing, true );

			for( int i=0; i < length; i++ )
			{
				// Needs to be rewritten
				p.drawLine(2+static_cast<int>(i*xscale), 
					2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale ),
					2+static_cast<int>((i+1)*xscale), 
					2+static_cast<int>( ( (*samps)[i+1] - maxVal ) * yscale )
					);
			}

			// Draw last segment wrapped around
			p.drawLine(2+static_cast<int>(length*xscale), 
				2+static_cast<int>( ( (*samps)[length] - maxVal ) * yscale ),
				width()-2,
				2+static_cast<int>( ( (*samps)[0] - maxVal ) * yscale ) );

			p.setRenderHints( QPainter::Antialiasing, false );
			break;


		case graph::NearestStyle:
			for( int i=0; i < length; i++ )
			{
				p.drawLine(2+static_cast<int>(i*xscale), 
					2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale ),
					2+static_cast<int>((i+1)*xscale), 
					2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale )
					);
				p.drawLine(2+static_cast<int>((i+1)*xscale), 
					2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale ),
					2+static_cast<int>((i+1)*xscale), 
					2+static_cast<int>( ( (*samps)[i+1] - maxVal ) * yscale )
					);
			}

			p.drawLine(2+static_cast<int>(length*xscale), 
				2+static_cast<int>( ( (*samps)[length] - maxVal ) * yscale ),
				width()-2,
				2+static_cast<int>( ( (*samps)[length] - maxVal ) * yscale ) );
			break;

		default:
			break;
	}


	// draw Pointer
	if( m_mouseDown ) 
	{
		QPoint cursor = mapFromGlobal( QCursor::pos() );
		p.setPen( QColor( 0xAA, 0xFF, 0x00, 0x70 ) );
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
		QString( "samplefile" ) ) == false )
	{
		_dee->ignore();
	}
}



void graph::modelChanged()
{
	graphModel * gModel = castModel<graphModel>();

	QObject::connect( gModel, SIGNAL( samplesChanged( int, int ) ),
			this, SLOT( updateGraph( int, int ) ) );

	QObject::connect( gModel, SIGNAL( lengthChanged( ) ),
			this, SLOT( updateGraph( ) ) );
}


void graph::updateGraph( int _startPos, int _endPos )
{
	// Can optimize by only drawing changed position
	update();
}


void graph::updateGraph()
{
    updateGraph( 0, model()->length() - 1 );
}


graphModel::graphModel( float _min, float _max, int _length,
			::Model * _parent, bool _default_constructed,  float _step ) :
	Model( _parent, tr( "Graph" ), _default_constructed ),
	m_samples( _length ),
	m_minValue( _min ),
	m_maxValue( _max ),
	m_step( _step )
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
			for( int i=0; i < length(); i++ )
			{
				m_samples[i] = fmaxf( _min, fminf( m_samples[i], _max ) );
			}
		}

		emit rangeChanged();
	}
}



void graphModel::setLength( int _length )
{
	if( _length != length() )
	{
		m_samples.resize( _length );
		emit lengthChanged();
	}
}



void graphModel::setSampleAt( int _x, float _val )
{
	//snap to the grid
	_val -= ( m_step != 0.0 ) ? fmod( _val, m_step ) * m_step : 0;
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



void graphModel::setWaveToSine()
{
	for( int i = 0; i < length(); i++ )
	{
		m_samples[i] = Oscillator::sinSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};



void graphModel::setWaveToTriangle()
{
	for( int i = 0; i < length(); i++ )
	{
		m_samples[i] = Oscillator::triangleSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};



void graphModel::setWaveToSaw()
{
	for( int i = 0; i < length(); i++ )
	{
		m_samples[i] = Oscillator::sawSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};



void graphModel::setWaveToSquare()
{
	for( int i = 0; i < length(); i++ )
	{
		m_samples[i] = Oscillator::squareSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};



void graphModel::setWaveToNoise()
{
	for( int i = 0; i < length(); i++ )
	{
		m_samples[i] = Oscillator::noiseSample(
				i / static_cast<float>( length() ) );
	}

	emit samplesChanged( 0, length() - 1 );
};

QString graphModel::setWaveToUser()
{
	SampleBuffer * sampleBuffer = new SampleBuffer;
	QString fileName = sampleBuffer->openAndSetWaveformFile();
	if( fileName.isEmpty() == false )
	{
		for( int i = 0; i < length(); i++ )
		{
			m_samples[i] = sampleBuffer->userWaveSample(
					i / static_cast<float>( length() ) );
		}
	}

	sharedObject::unref( sampleBuffer );

	emit samplesChanged( 0, length() - 1 );
	return fileName;
};



void graphModel::smooth()
{
	// store values in temporary array
	QVector<float> temp = m_samples;

	// Smoothing
	m_samples[0] = ( temp[0] + temp[length()-1] ) * 0.5f;
	for ( int i=1; i < length(); i++ )
	{
		m_samples[i] = ( temp[i-1] + temp[i] ) * 0.5f; 	
	}

	emit samplesChanged(0, length()-1);
}



void graphModel::normalize()
{
	float max = 0.0001f;
	for( int i = 0; i < length(); i++ )
	{
		if( fabsf(m_samples[i]) > max && m_samples[i] != 0.0f )
		{
			max = fabs( m_samples[i] );
		}
	}

	for( int i = 0; i < length(); i++ )
	{
		m_samples[i] /= max;
	}

	if( max != 1.0f ) {
		emit samplesChanged( 0, length()-1 );
	}
}	




#include "moc_graph.cxx"
