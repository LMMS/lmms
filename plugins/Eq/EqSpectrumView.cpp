/* eqspectrumview.cpp - implementation of EqSpectrumView class.
*
* Copyright (c) 2014-2017, David French <dave/dot/french3/at/googlemail/dot/com>
*
* This file is part of LMMS - https://lmms.io
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

#include "EqSpectrumView.h"

#include <cmath>
#include <QPainter>
#include <QPen>

#include "AudioEngine.h"
#include "Engine.h"
#include "EqCurve.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "lmms_constants.h"

namespace lmms
{


EqAnalyser::EqAnalyser() :
	m_framesFilledUp ( 0 ),
	m_energy ( 0 ),
	m_sampleRate ( 1 ),
	m_active ( true )
{
	m_inProgress=false;
	m_specBuf = ( fftwf_complex * ) fftwf_malloc( ( FFT_BUFFER_SIZE + 1 ) * sizeof( fftwf_complex ) );
	m_fftPlan = fftwf_plan_dft_r2c_1d( FFT_BUFFER_SIZE*2, m_buffer, m_specBuf, FFTW_MEASURE );

	//initialize Blackman-Harris window, constants taken from
	//https://en.wikipedia.org/wiki/Window_function#A_list_of_window_functions
	const float a0 = 0.35875f;
	const float a1 = 0.48829f;
	const float a2 = 0.14128f;
	const float a3 = 0.01168f;

	for (auto i = std::size_t{0}; i < FFT_BUFFER_SIZE; i++)
	{
		m_fftWindow[i] = (a0 - a1 * cos(2 * F_PI * i / ((float)FFT_BUFFER_SIZE - 1.0))
								+ a2 * cos(4 * F_PI * i / ((float)FFT_BUFFER_SIZE - 1.0))
								- a3 * cos(6 * F_PI * i / ((float)FFT_BUFFER_SIZE - 1.0)));
	}
	clear();
}




EqAnalyser::~EqAnalyser()
{
	fftwf_destroy_plan( m_fftPlan );
	fftwf_free( m_specBuf );
}




void EqAnalyser::analyze( SampleFrame* buf, const fpp_t frames )
{
	//only analyse if the view is visible
	if ( m_active )
	{
		m_inProgress=true;
		const int FFT_BUFFER_SIZE = 2048;
		fpp_t f = 0;
		if( frames > FFT_BUFFER_SIZE )
		{
			m_framesFilledUp = 0;
			f = frames - FFT_BUFFER_SIZE;
		}
		// meger channels
		for( ; f < frames; ++f )
		{
			m_buffer[m_framesFilledUp] =
					( buf[f][0] + buf[f][1] ) * 0.5;
			++m_framesFilledUp;
		}

		if( m_framesFilledUp < FFT_BUFFER_SIZE )
		{
			m_inProgress = false;
			return;
		}

		m_sampleRate = Engine::audioEngine()->outputSampleRate();
		const int LOWEST_FREQ = 0;
		const int HIGHEST_FREQ = m_sampleRate / 2;

		//apply FFT window
		for( int i = 0; i < FFT_BUFFER_SIZE; i++ )
		{
			m_buffer[i] = m_buffer[i] * m_fftWindow[i];
		}

		fftwf_execute( m_fftPlan );
		absspec( m_specBuf, m_absSpecBuf, FFT_BUFFER_SIZE+1 );

		compressbands( m_absSpecBuf, m_bands, FFT_BUFFER_SIZE+1,
					   MAX_BANDS,
					   ( int )( LOWEST_FREQ * ( FFT_BUFFER_SIZE + 1 ) / ( float )( m_sampleRate / 2 ) ),
					   ( int )( HIGHEST_FREQ * ( FFT_BUFFER_SIZE +  1) / ( float )( m_sampleRate / 2 ) ) );
		m_energy = maximum( m_bands, MAX_BANDS ) / maximum( m_buffer, FFT_BUFFER_SIZE );

		m_framesFilledUp = 0;
		m_inProgress = false;
		m_active = false;
	}
}




float EqAnalyser::getEnergy() const
{
	return m_energy;
}




int EqAnalyser::getSampleRate() const
{
	return m_sampleRate;
}




bool EqAnalyser::getActive() const
{
	return m_active;
}




void EqAnalyser::setActive(bool active)
{
	m_active = active;
}




bool EqAnalyser::getInProgress()
{
	return m_inProgress;
}




void EqAnalyser::clear()
{
	m_framesFilledUp = 0;
	m_energy = 0;
	memset( m_buffer, 0, sizeof( m_buffer ) );
	memset( m_bands, 0, sizeof( m_bands ) );
}



namespace gui
{

EqSpectrumView::EqSpectrumView(EqAnalyser *b, QWidget *_parent) :
	QWidget( _parent ),
	m_analyser( b ),
	m_peakSum(0.),
	m_periodicalUpdate( false )
{
	setFixedSize( 450, 200 );
	connect( getGUI()->mainWindow(), SIGNAL( periodicUpdate() ), this, SLOT( periodicalUpdate() ) );
	setAttribute( Qt::WA_TranslucentBackground, true );
	m_skipBands = MAX_BANDS * 0.5;
	float totalLength = log10( 20000 );
	m_pixelsPerUnitWidth = width() / totalLength ;
	m_scale = 1.5;
	m_color = QColor( 255, 255, 255, 255 );
	for ( int i = 0 ; i < MAX_BANDS ; i++ )
	{
		m_bandHeight.append( 0 );
	}
}




void EqSpectrumView::paintEvent(QPaintEvent *event)
{
	const float energy = m_analyser->getEnergy();
	if (energy <= 0. && m_peakSum <= 0) { return; }

	const int fh = height();
	const int LOWER_Y = -36;	// dB
	QPainter painter( this );
	painter.setPen( QPen( m_color, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin ) );
	painter.setRenderHint(QPainter::Antialiasing, true);

	if( m_analyser->getInProgress() || m_periodicalUpdate == false )
	{
		//only paint the cached path
		painter.fillPath( m_path, QBrush( m_color ) );
		return;
	}

	m_periodicalUpdate = false;
	//Now we calculate the path
	m_path = QPainterPath();
	float *bands = m_analyser->m_bands;
	m_path.moveTo( 0, height() );
	m_peakSum = 0;
	const float fallOff = 1.07f;
	for( int x = 0; x < MAX_BANDS; ++x, ++bands )
	{
		float peak = *bands != 0. ? (fh * 2.0 / 3.0 * (20. * log10(*bands / energy) - LOWER_Y) / (-LOWER_Y)) : 0.;

		if( peak < 0 )
		{
			peak = 0;
		}
		else if( peak >= fh )
		{
			continue;
		}

		if ( peak > m_bandHeight[x] )
		{
			m_bandHeight[x] = peak;
		}
		else
		{
			m_bandHeight[x] = m_bandHeight[x] / fallOff;
		}

		if( m_bandHeight[x] < 0 )
		{
			m_bandHeight[x] = 0;
		}

		m_path.lineTo( EqHandle::freqToXPixel( bandToFreq( x ), width() ), fh - m_bandHeight[x] );
		m_peakSum += m_bandHeight[x];
	}

	m_path.lineTo( width(), height() );
	m_path.closeSubpath();
	painter.fillPath( m_path, QBrush( m_color ) );
	painter.drawPath( m_path );
}




QColor EqSpectrumView::getColor() const
{
	return m_color;
}




void EqSpectrumView::setColor( const QColor &value )
{
	m_color = value;
}




float EqSpectrumView::bandToFreq( int index )
{
	return index * m_analyser->getSampleRate() / ( MAX_BANDS * 2 );
}




void EqSpectrumView::periodicalUpdate()
{
	m_periodicalUpdate = true;
	m_analyser->setActive( isVisible() );
	update();
}


} // namespace gui

} // namespace lmms
