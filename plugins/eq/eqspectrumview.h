#ifndef EQSPECTRUMVIEW_H
#define EQSPECTRUMVIEW_H

#include "qpainter.h"
//#include "eqeffect.h"
#include "qwidget.h"
#include "fft_helpers.h"


const int MAX_BANDS = 512;

class FftBands
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

	FftBands() :
		m_framesFilledUp( 0 ),
		m_energy( 0 )
	{
		memset( m_buffer, 0, sizeof( m_buffer ) );

		m_specBuf = (fftwf_complex *) fftwf_malloc( ( FFT_BUFFER_SIZE + 1 ) * sizeof( fftwf_complex ) );
		m_fftPlan = fftwf_plan_dft_r2c_1d( FFT_BUFFER_SIZE*2, m_buffer, m_specBuf, FFTW_MEASURE );
	}
};


class EqSpectrumView : public QWidget
{

public:
	explicit EqSpectrumView( FftBands * b, QWidget * _parent = 0) :
		QWidget( _parent ),
		m_sa( b )
	{
		setFixedSize( 250, 116 );
		connect( Engine::mainWindow(), SIGNAL( periodicUpdate() ), this, SLOT( update() ) );
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
	FftBands *m_sa;
	int m_lastY;
	virtual void paintEvent( QPaintEvent* event )
	{
		int m_lastY = height();
		QPainter p( this );
		p.setPen( QPen( color, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin ) );
		const float e =  m_sa->m_energy;
		if( e <= 0 )
		{
			//dont draw anything
			return;
		}
		float * b = m_sa->m_bands;
		const int LOWER_Y = -60;	// dB
		int h;
		const int fh = height();
		bool linX = true;
		if( linX )
		{
			for( int x = 0; x < MAX_BANDS; ++x, ++b )
			{
				h = (int)( fh * 2.0 / 3.0 * (20*(log10( *b / e ) ) - LOWER_Y ) / (-LOWER_Y ) );
				if( h < 0 ) h = 0; else if( h >= fh ) continue;
				p.drawLine(freqToXPixel(bandToFreq(x -1 ) ),m_lastY, freqToXPixel(bandToFreq(x  ) ), fh-h );
				m_lastY = fh-h;
			}
		}
		else
		{
			for( int x = 0; x < 31; ++x, ++b )
			{
				h = (int)( fh * 2.0 / 3.0 * (20*(log10( *b / e ) ) - LOWER_Y ) / (-LOWER_Y ) );
				if( h < 0 ) h = 0; else if( h >= fh ) continue; else h = ( h / 3 ) * 3;
				p.drawPoint(x * 8, fh-h );
			}
		}
	}




	inline int bandToXPixel( float band )
	{
		return ( log10( band - m_skipBands ) * m_pixelsPerUnitWidth * m_scale );
	}

	inline float bandToFreq ( int index )
	{
		return index * m_sa->m_sr / (MAX_BANDS * 2);
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
