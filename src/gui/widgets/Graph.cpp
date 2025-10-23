/*
 * Graph.cpp - a QT widget for displaying and manipulating waveforms
 *
 * Copyright (c) 2006-2007 Andreas Brandmaier <andy/at/brandmaier/dot/de>
 *               2008 Paul Giblock            <drfaygo/at/gmail/dot/com>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <QPainter>

#include "Clipboard.h"
#include "FileTypes.h"
#include "Graph.h"
#include "SampleLoader.h"
#include "Oscillator.h"

namespace lmms
{

namespace gui
{

Graph::Graph( QWidget * _parent, Style _style, int _width,
		int _height ) :
	QWidget( _parent ),
	/* TODO: size, background? */
	ModelView( new graphModel( -1.0, 1.0, 128, nullptr, true ), this ),
	m_graphStyle( _style )
{
	m_mouseDown = false;
	m_graphColor = QColor( 0xFF, 0xAA, 0x00 );

	resize( _width, _height );
	setAcceptDrops( true );
	setCursor( Qt::CrossCursor );

	auto gModel = castModel<graphModel>();

	QObject::connect( gModel, SIGNAL(samplesChanged(int,int)),
			this, SLOT(updateGraph(int,int)));

	QObject::connect( gModel, SIGNAL(lengthChanged()),
			this, SLOT(updateGraph()));
}

void Graph::setForeground( const QPixmap &_pixmap )
{
	m_foreground = _pixmap;
}

void Graph::setGraphColor( QColor _graphcol )
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



void Graph::mouseMoveEvent ( QMouseEvent * _me )
{
	// get position
	int x = _me->x();
	int y = _me->y();

/*	static bool skip = false;

	if( skip )
	{
		skip = false;
		return;
	}
*/
	// avoid mouse leaps
	int diff = x - m_lastCursorX;

/*	if( diff >= 1 )
	{
		x = qMin( width() - 3, m_lastCursorX + 1 );
	}
	else if( diff <= -1 )
	{
		x = qMax( 2, m_lastCursorX - 1 );
	}*/

	x = qMax( 2, qMin( x, width()-3 ) );
	y = qMax( 2, qMin( y, height()-3 ) );


	drawLineAt( x, y, m_lastCursorX );

	// update mouse
	if( diff != 0 )
	{
		m_lastCursorX = x;

		QPoint pt = mapToGlobal( QPoint( x, y ) );

		QCursor::setPos( pt.x(), pt.y() );
	}

//	skip = true;
}



void Graph::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		if ( !( _me->modifiers() & Qt::ShiftModifier ) )
		{
			// get position
			int x = _me->x();
			int y = _me->y();

			changeSampleAt( x, y );

			// toggle mouse state
			m_mouseDown = true;
			setCursor( Qt::BlankCursor );
			m_lastCursorX = x;
		}
		else
		{
			//when shift-clicking, draw a line from last position to current
			//position
			int x = _me->x();
			int y = _me->y();

			drawLineAt( x, y, m_lastCursorX );

			m_mouseDown = true;
			setCursor( Qt::BlankCursor );
			m_lastCursorX = x;
		}

	}
}

void Graph::drawLineAt( int _x, int _y, int _lastx )
{
	float minVal = model()->minValue();
	float maxVal = model()->maxValue();
	if ( width() <= 4 )
	{
		return;
	}

	float xscale = static_cast<float>( model()->length() ) /
					( width()-4 );

	//consider border
	_x -= 2;
	_y -= 2;
	_lastx -= 2;

	_lastx = qMax( 0, qMin( _lastx, width()-5 ) );

	float range = minVal - maxVal;
	float val = ( _y*range/( height()-5 ) ) + maxVal;
	
	int sample_begin, sample_end;
	float lastval;
	float val_begin, val_end;
	
	if (_lastx > _x)
	{
		sample_begin = (int)((_x) * xscale);
		sample_end = (int)ceil((_lastx+1) * xscale);
		lastval = model() -> m_samples[ (int)( sample_end - 1 ) ];
		val_begin = val;
		val_end = lastval;

	}
	else
	{
		sample_begin = (int)(_lastx * xscale);
		sample_end =  (int)ceil((_x+1) * xscale);
		lastval = model() -> m_samples[ (int)( sample_begin ) ];
		val_begin = lastval;
		val_end = val;
		
	}
	
	// calculate line drawing variables
	int linelen = sample_end - sample_begin;
	if (linelen == 1)
	{
		val_begin = val;
	}
	//int xstep = _x > _lastx ? -1 : 1;
	float ystep = ( val_end - val_begin ) / linelen;

	// draw a line
	for ( int i = 0 ; i < linelen; i++ )
	{
		model()->drawSampleAt( sample_begin + i , val_begin + ((i ) * ystep));
	}

	// We've changed [sample_end, sample_begin)
	// However, samplesChanged expects two end points
	model()->samplesChanged(sample_begin, sample_end - 1);
}

void Graph::changeSampleAt( int _x, int _y )
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
	float val = ( _y*range/( height()-5 ) ) + maxVal;

	model()->setSampleAt( (int)( _x*xscale ), val );
}



void Graph::mouseReleaseEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		// toggle mouse state
		m_mouseDown = false;
		setCursor( Qt::CrossCursor );
		update();
		emit drawn();
	}
}



void Graph::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.setPen( QPen( m_graphColor, 1 ) );
	QColor gcol = QColor( m_graphColor.red(), m_graphColor.green(), m_graphColor.blue(), 100 );

	QVector<float> * samps = &(model()->m_samples);
	int length = model()->length();
	const float maxVal = model()->maxValue();
	const float minVal = model()->minValue();

	float xscale = (float)( width()-4 ) / length;
	float yscale = (float)( height()-4 ) / ( minVal - maxVal );

	// Max index, more useful below
	length--;


	switch( m_graphStyle )
	{
		case Style::Linear:
			p.setRenderHints( QPainter::Antialiasing, true );

			for( int i=0; i < length; i++ )
			{
				// Needs to be rewritten
				p.drawLine(
					2+static_cast<int>(i*xscale),
					2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale ),
					2+static_cast<int>((i+1)*xscale),
					2+static_cast<int>( ( (*samps)[i+1] - maxVal ) * yscale )
					);
			}

			// Draw last segment wrapped around
			p.drawLine(2+static_cast<int>(length*xscale),
				2+static_cast<int>( ( (*samps)[length] - maxVal ) * yscale ),
				width()-3,
				2+static_cast<int>( ( (*samps)[0] - maxVal ) * yscale ) );

			p.setRenderHints( QPainter::Antialiasing, false );
			break;


		case Style::Nearest:
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
				width()-3,
				2+static_cast<int>( ( (*samps)[length] - maxVal ) * yscale ) );
			break;

		case Style::LinearNonCyclic:
			p.setRenderHints( QPainter::Antialiasing, true );

			for( int i=0; i < length; i++ )
			{
				// Needs to be rewritten
				p.drawLine(
					2+static_cast<int>(i*xscale),
					2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale ),
					2+static_cast<int>((i+1)*xscale),
					2+static_cast<int>( ( (*samps)[i+1] - maxVal ) * yscale )
					);
			}

			// Do not draw last segment wrapped around - hence, "non-cyclic"

			p.setRenderHints( QPainter::Antialiasing, false );
			break;

		case Style::Bar:
			for( int i=0; i <= length; i++ )
			{
				p.fillRect( 2+static_cast<int>( i*xscale ),
							2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale ),
							qMax( static_cast<int>(xscale) - 1, 1 ),
							qMax( static_cast<int>( ( minVal - maxVal ) * yscale ) - static_cast<int>( ( (*samps)[i] - maxVal ) * yscale ), 1 ),
							gcol );

				p.setPen( QPen( m_graphColor, 1.0 ) );

				p.drawLine( 2+static_cast<int>(i*xscale),
					2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale ),
					qMax( static_cast<int>(i*xscale) + static_cast<int>(xscale), 2+static_cast<int>(i*xscale) ),
					2+static_cast<int>( ( (*samps)[i] - maxVal ) * yscale )
					);
			}

			break;

		default:
			break;
	}


	// draw Pointer
	if( m_mouseDown )
	{
		QPoint cursor = mapFromGlobal( QCursor::pos() );
		p.setPen( QColor( 0x70, 0x7C, 0x91 ) );
		p.drawLine( 2, cursor.y(), width()-2, cursor.y() );
		p.drawLine( cursor.x(), 2, cursor.x(), height()-2 );
	}
	p.drawPixmap( 0, 0, m_foreground );
}



void Graph::dropEvent( QDropEvent * _de )
{
		// TODO: call setWaveToUser
		// loadSampleFromFile( value );
}

void Graph::dragEnterEvent(QDragEnterEvent* _dee)
{
	DragAndDrop::acceptFile(_dee, {FileType::Sample});
}



void Graph::modelChanged()
{
	auto gModel = castModel<graphModel>();

	QObject::connect( gModel, SIGNAL(samplesChanged(int,int)),
			this, SLOT(updateGraph(int,int)));

	QObject::connect( gModel, SIGNAL(lengthChanged()),
			this, SLOT(updateGraph()));
}


void Graph::updateGraph( int _startPos, int _endPos )
{
	// Can optimize by only drawing changed position
	update();
}


void Graph::updateGraph()
{
    updateGraph( 0, model()->length() - 1 );
}


} // namespace gui

graphModel::graphModel(float ymin, float ymax, int length, Model* parent, bool defaultConstructed, float step) :
	Model(parent, tr("Graph"), defaultConstructed),
	m_samples(length),
	m_length(length),
	m_minValue(ymin),
	m_maxValue(ymax),
	m_step(std::clamp(step, 0.f, std::abs(ymax - ymin)))
{
}

void graphModel::setRange(float ymin, float ymax)
{
	if (ymin == m_minValue && ymax == m_maxValue) { return; }
	// Step sizes less than zero or larger than the entire range of
	// values are nonsense, clamp those
	m_step = std::clamp(m_step, 0.f, std::abs(ymax - ymin));
	m_minValue = ymin;
	m_maxValue = ymax;
	// Trim existing values
	for (float& sample : m_samples) {
		sample = std::clamp(sample, ymin, ymax);
	}
	emit rangeChanged();
}

void graphModel::setLength( int _length )
{
	if( _length != m_length )
	{
		m_length = _length;
		if( m_samples.size() < m_length )
		{
			m_samples.resize( m_length );
		}
		emit lengthChanged();
	}
}



void graphModel::setSampleAt( int x, float val )
{
	drawSampleAt( x, val );
	emit samplesChanged( x, x );
}



void graphModel::setSamples( const float * _samples )
{
	std::copy( _samples, _samples + length(), m_samples.begin());

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
	QString fileName = gui::SampleLoader::openWaveformFile();
	if( fileName.isEmpty() == false )
	{
		auto sampleBuffer = gui::SampleLoader::createBufferFromFile(fileName);
		for( int i = 0; i < length(); i++ )
		{
			m_samples[i] = Oscillator::userWaveSample(sampleBuffer.get(), i / static_cast<float>(length()));
		}
	}

	emit samplesChanged( 0, length() - 1 );
	return fileName;
};



void graphModel::smooth()
{
	// store values in temporary array
	QVector<float> temp = m_samples;

	// Smoothing
	m_samples[0] = ( temp[length()-1] + ( temp[0] * 2 ) + temp[1] ) * 0.25f;
	for ( int i=1; i < ( length()-1 ); i++ )
	{
		m_samples[i] = ( temp[i-1] + ( temp[i] * 2 ) + temp[i+1] ) * 0.25f;
	}
	m_samples[length()-1] = ( temp[length()-2] + ( temp[length()-1] * 2 ) + temp[0] ) * 0.25f;

	emit samplesChanged(0, length()-1);
}

void graphModel::smoothNonCyclic()
{
	// store values in temporary array
	QVector<float> temp = m_samples;

	// Smoothing
	//m_samples[0] = ( ( temp[0] * 3 ) + temp[1] ) * 0.25f;
	for ( int i=1; i < ( length()-1 ); i++ )
	{
		m_samples[i] = ( temp[i-1] + ( temp[i] * 2 ) + temp[i+1] ) * 0.25f;
	}
	//m_samples[length()-1] = ( temp[length()-2] + ( temp[length()-1] * 3 ) ) * 0.25f;

	emit samplesChanged(0, length()-1);
}

void graphModel::convolve(const float *convolution,
	const int convolutionLength, const int centerOffset)
{
	// store values in temporary array
	QVector<float> temp = m_samples;
	const int graphLength = length();
	// make a cyclic convolution
	for ( int i = 0; i <  graphLength; i++ )
	{
		float sum = 0.0f;
		for ( int j = 0; j < convolutionLength; j++ )
		{
			sum += convolution[j] * temp[( i + j ) % graphLength];
		}
		m_samples[( i + centerOffset ) % graphLength] = sum;
	}
	emit samplesChanged(0, graphLength - 1);
}

void graphModel::normalize()
{
	float max = 0.0001f;
	float avg = 0.0f;

	// first correct dc offset by normalizing to average
	for( int i = 0; i < length(); i++ )
		avg += m_samples[i];
	avg /= length();
	for( int i = 0; i < length(); i++ )
		m_samples[i] -= avg;

	// then maximize
	for( int i = 0; i < length(); i++ )
		max = qMax( max, qAbs( m_samples[i] ) );

	for( int i = 0; i < length(); i++ )
		m_samples[i] = qBound( m_minValue, m_samples[i] / max, m_maxValue );

	// signal changes if any
	if( max != 1.0f || avg != 0.0f )
		emit samplesChanged( 0, length()-1 );
}



void graphModel::invert()
{
	const float range = m_maxValue - m_minValue;

	for( int i = 0; i < length(); i++ )
		m_samples[i] = m_minValue + ( range - ( m_samples[i] - m_minValue ) );

	emit samplesChanged( 0, length()-1 );
}



void graphModel::shiftPhase( int _deg )
{
	// calculate offset in samples
	const int offset = ( _deg * length() ) / 360; //multiply first because integers
	
	// store values in temporary array
	QVector<float> temp = m_samples;
	
	// shift phase
	for( int i = 0; i < length(); i++ )
	{
		int o = ( i + offset ) % length();
		while( o < 0 ) o += length();
		m_samples[i] = temp[o];
	}
	
	emit samplesChanged( 0, length()-1 );
}

void graphModel::clear()
{
	const int graph_length = length();
	for( int i = 0; i < graph_length; i++ )
		m_samples[i] = 0;
	emit samplesChanged( 0, graph_length - 1 );
}


// Clear any part of the graph that isn't displayed
void graphModel::clearInvisible()
{
	const int graph_length = length();
	const int full_graph_length = m_samples.size();
	for( int i = graph_length; i < full_graph_length; i++ )
		m_samples[i] = 0;
	emit samplesChanged( graph_length, full_graph_length - 1 );
}

void graphModel::drawSampleAt(int x, float val)
{
	// snap to the grid
	if (m_step > 0) { val = std::floor(val / m_step) * m_step; }
	// boundary crop
	x = std::clamp(x, 0, length() - 1);
	val = std::clamp(val, minValue(), maxValue());
	// change sample shape
	m_samples[x] = val;
}


} // namespace lmms
