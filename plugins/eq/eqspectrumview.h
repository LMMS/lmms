#ifndef EQSPECTRUMVIEW_H
#define EQSPECTRUMVIEW_H

#include "qpainter.h"
#include "eqeffect.h"
#include "qwidget.h"

class EqParams
{
public:
	const static int MAX_BANDS = 249;
	float m_bands[MAX_BANDS];
	float m_energy;
};


class EqSpectrumView : public QWidget
{
public:
	EqSpectrumView( EqParams* s, QWidget * _parent = 0) :
		QWidget( _parent ),
		m_sa( s )
	{
		setFixedSize( 240, 116 );
		connect( Engine::mainWindow(), SIGNAL( periodicUpdate() ), this, SLOT( update() ) );
		setAttribute( Qt::WA_OpaquePaintEvent, true );
	}

	virtual ~EqSpectrumView()
	{
	}
EqParams *m_sa;
	virtual void paintEvent( QPaintEvent* event )
	{
		const int MAX_BANDS = 249;
		QPainter p( this );
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
				p.drawPoint( x, fh-h );
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



private:


//EqParams *m_sa;
//	QImage m_backgroundPlain;
//	QImage m_background;

} ;


#endif // EQSPECTRUMVIEW_H
