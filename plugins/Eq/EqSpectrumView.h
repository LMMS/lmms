/* eqspectrumview.h - defination of EqSpectrumView class.
*
* Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
*
* This file is part of LMMS - http://lmms.io
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
#ifndef EQSPECTRUMVIEW_H
#define EQSPECTRUMVIEW_H

#include "qpainter.h"
//#include "eqeffect.h"
#include "qwidget.h"
#include "fft_helpers.h"
#include "Engine.h"


const int MAX_BANDS = 2048;

class EqAnalyser
{
public:

	fftwf_plan m_fftPlan;
	fftwf_complex * m_specBuf;
	float m_absSpecBuf[FFT_BUFFER_SIZE+1];
	float m_buffer[FFT_BUFFER_SIZE*2];
	int m_framesFilledUp;
	float m_bands[MAX_BANDS];
	float m_energy;
	int m_sr;
	bool m_active;


	EqAnalyser() :
		m_framesFilledUp ( 0 ),
		m_energy ( 0 ),
		m_sr ( 1 ),
		m_active ( true )
	{
		m_inProgress=false;
		m_specBuf = (fftwf_complex *) fftwf_malloc( ( FFT_BUFFER_SIZE + 1 ) * sizeof( fftwf_complex ) );
		m_fftPlan = fftwf_plan_dft_r2c_1d( FFT_BUFFER_SIZE*2, m_buffer, m_specBuf, FFTW_MEASURE );
		clear();
	}

	virtual ~EqAnalyser()
	{
		fftwf_destroy_plan( m_fftPlan );
		fftwf_free( m_specBuf );
	}


	bool getInProgress()
	{
		return m_inProgress;
	}



	void clear()
	{
		m_framesFilledUp = 0;
		m_energy = 0;
		memset( m_buffer, 0, sizeof( m_buffer ) );
		memset( m_bands, 0, sizeof( m_bands ) );
	}



	void analyze( sampleFrame *buf, const fpp_t frames )
	{
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

			m_sr = LmmsEngine::mixer()->processingSampleRate();
			const int LOWEST_FREQ = 0;
			const int HIGHEST_FREQ = m_sr / 2;

			fftwf_execute( m_fftPlan );
			absspec( m_specBuf, m_absSpecBuf, FFT_BUFFER_SIZE+1 );

			compressbands( m_absSpecBuf, m_bands, FFT_BUFFER_SIZE+1,
						   MAX_BANDS,
						   ( int )( LOWEST_FREQ * ( FFT_BUFFER_SIZE + 1 ) / ( float )( m_sr / 2 ) ),
						   ( int )( HIGHEST_FREQ * ( FFT_BUFFER_SIZE +  1) / ( float )( m_sr / 2 ) ) );
			m_energy = maximum( m_bands, MAX_BANDS ) / maximum( m_buffer, FFT_BUFFER_SIZE );
			m_framesFilledUp = 0;
			m_inProgress = false;
			m_active = false;
		}
	}
private:
	bool m_inProgress;
};


class EqSpectrumView : public QWidget
{

public:
	explicit EqSpectrumView( EqAnalyser * b, QWidget * _parent = 0 ):
		QWidget( _parent ),
		m_sa( b )
	{
		setFixedSize( 250, 116 );
		QTimer *timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(update()));
		timer->start(2000);
		setAttribute( Qt::WA_TranslucentBackground, true );
		m_skipBands = MAX_BANDS * 0.5;
		float totalLength = log10( 21000);
		m_pixelsPerUnitWidth = width( ) /  totalLength ;
		m_scale = 1.5;
		color = QColor( 255, 255, 255, 255 );
	}




	virtual ~EqSpectrumView()
	{
	}




	QColor color;
	EqAnalyser *m_sa;
	QPainterPath pp;
	virtual void paintEvent( QPaintEvent* event )
	{
		m_sa->m_active = isVisible();
		const int fh = height();
		const int LOWER_Y = -60;	// dB
		QPainter p( this );
		p.setPen( QPen( color, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin ) );
		const float e =  m_sa->m_energy;
		if( e <= 0 )
		{
			//dont draw anything
			return;
		}
		if(m_sa->getInProgress() ){
			p.fillPath( pp ,QBrush( color ) );
			return;
		}
		pp = QPainterPath();
		float * b = m_sa->m_bands;
		int h;
		pp.moveTo( 0,height() );
		for( int x = 0; x < MAX_BANDS; ++x, ++b )
		{
			h = (int)( fh * 2.0 / 3.0 * ( 20 * ( log10 ( *b / e ) ) - LOWER_Y ) / (-LOWER_Y ) );
			if( h < 0 ) h = 0; else if( h >= fh ) continue;
			pp.lineTo( freqToXPixel(bandToFreq( x ) ), fh-h );
		}
		pp.lineTo( width(), height() );
		pp.closeSubpath();
		p.fillPath( pp, QBrush( color ) );
		p.drawPath( pp );
	}




	inline int bandToXPixel( float band )
	{
		return ( log10( band - m_skipBands ) * m_pixelsPerUnitWidth * m_scale );
	}

	inline float bandToFreq ( int index )
	{
		return index * m_sa->m_sr / (MAX_BANDS * 2 );
	}


	inline int freqToXPixel( float freq )
	{
		return ( log10( freq ) * m_pixelsPerUnitWidth * m_scale ) - ( width() * 0.5 );
	}
private:
	float m_pixelsPerUnitWidth;
	float m_scale;
	int m_skipBands;
} ;


#endif // EQSPECTRUMVIEW_H
