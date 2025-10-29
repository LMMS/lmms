/*
 * EqCurve.cpp - declaration of EqCurve and EqHandle classes.
 *
 * Copyright (c) 2015 Steffen Baranowsky <BaraMGB/at/freenet/dot/de>
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
#include "EqCurve.h"

#include <cmath>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include "AudioEngine.h"
#include "embed.h"
#include "Engine.h"
#include "FontHelper.h"
#include "lmms_math.h"


namespace lmms::gui
{


EqHandle::EqHandle( int num, int x, int y ):
	m_numb( num ),
	m_width( x ),
	m_heigth( y ),
	m_mousePressed( false ),
	m_active( false )
{
	setFlag( ItemIsMovable );
	setFlag( ItemSendsGeometryChanges );
	setAcceptHoverEvents( true );
	float totalHeight = 36;
	m_pixelsPerUnitHeight = ( m_heigth ) / ( totalHeight );
	setMouseHover( false );
}




QRectF EqHandle::boundingRect() const
{
	return QRectF( - m_circlePixmap.width() / 2, - m_circlePixmap.height() / 2, m_circlePixmap.width(), m_circlePixmap.height() );
}




float EqHandle::freqToXPixel( float freq , int w )
{
	if (approximatelyEqual(freq, 0.0f)) { return 0.0f; }
	float min = std::log10(20);
	float max = std::log10(20000);
	float range = max - min;
	return (std::log10(freq) - min) / range * w;
}




float EqHandle::xPixelToFreq( float x , int w )
{
	float min = std::log10(20);
	float max = std::log10(20000);
	float range = max - min;
	return fastPow10f(x * (range / w) + min);
}




float EqHandle::gainToYPixel(float gain , int h, float pixelPerUnitHeight )
{
	return h * 0.5 - gain * pixelPerUnitHeight;
}




float EqHandle::yPixelToGain(float y , int h, float pixelPerUnitHeight )
{
	return ( ( h * 0.5 ) - y ) / pixelPerUnitHeight;
}




void EqHandle::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
	painter->setRenderHint( QPainter::Antialiasing, true );
	if ( m_mousePressed )
	{
		emit positionChanged();
	}

	// graphics for the handles
	loadPixmap();
	painter->drawPixmap( - ( m_circlePixmap.width() / 2 ) - 1 , - ( m_circlePixmap.height() / 2 ), m_circlePixmap );

	// on mouse hover draw an info box and change the pixmap of the handle
	if ( isMouseHover() )
	{
		// keeps the info box in view
		float rectX = -40;
		float rectY = -40;
		if ( EqHandle::y() < 40 )
		{
			rectY = rectY + 40 - EqHandle::y();
		}
		if ( EqHandle::x() < 40 )
		{
			rectX = rectX + 40 - EqHandle::x();
		}
		if ( EqHandle::x() > m_width - 40 )
		{
			rectX = rectX - ( 40 - ( m_width - EqHandle::x() ) );
		}
		QPixmap hover = PLUGIN_NAME::getIconPixmap( "handlehover" );
		painter->drawPixmap( - ( hover.width() / 2) - 1, - ( hover.height() / 2 ), hover );
		QRectF textRect = QRectF ( rectX, rectY, 80, 30 );
		QRectF textRect2 = QRectF ( rectX+1, rectY+1, 80, 30 );
		QString freq = QString::number( xPixelToFreq( EqHandle::x(), m_width ) );
		QString res;
		if ( getType() != EqHandleType::Para )
		{
			res = tr( "Reso: ") + QString::number( getResonance() );
		}
		else
		{
			res = tr( "BW: " ) +  QString::number( getResonance() );
		}

		painter->setFont(adjustedToPixelSize(painter->font(), SMALL_FONT_SIZE));
		painter->setPen( Qt::black );
		painter->drawRect( textRect );
		painter->fillRect( textRect, QBrush( QColor( 6, 106, 43, 180 ) ) );

		painter->setPen ( QColor( 0, 0, 0 ) );
		painter->drawText( textRect2, Qt::AlignCenter,
						   QString( tr( "Freq: " ) + freq + "\n" + res ) );
		painter->setPen( QColor( 255, 255, 255 ) );
		painter->drawText( textRect, Qt::AlignCenter,
						   QString( tr( "Freq: " ) + freq + "\n" + res ) );
	}
}




QPainterPath EqHandle::getCurvePath()
{
	QPainterPath path;
	float y = m_heigth * 0.5;
	for ( float x = 0 ; x < m_width; x++ )
	{
		if ( m_type == EqHandleType::HighPass ) y = getLowCutCurve( x );
		if ( m_type == EqHandleType::LowShelf ) y = getLowShelfCurve( x );
		if ( m_type == EqHandleType::Para ) y = getPeakCurve( x );
		if ( m_type == EqHandleType::HighShelf ) y = getHighShelfCurve( x );
		if ( m_type == EqHandleType::LowPass ) y = getHighCutCurve( x );
		if ( x == 0 ) path.moveTo( x, y ); // sets the begin of Path
		path.lineTo( x, y );
	}
	return path;
}

void EqHandle::loadPixmap()
{
	auto fileName = "handle" + std::to_string(m_numb + 1);
	if (!isActiveHandle()) { fileName += "inactive"; }
	m_circlePixmap = PLUGIN_NAME::getIconPixmap(fileName);
}

bool EqHandle::mousePressed() const
{
	return m_mousePressed;
}




float EqHandle::getPeakCurve( float x )
{
	using namespace std::numbers;
	double freqZ = xPixelToFreq( EqHandle::x(), m_width );
	double w0 = 2 * pi * freqZ / Engine::audioEngine()->outputSampleRate();
	double c = std::cos(w0);
	double s = std::sin(w0);
	double Q = getResonance();
	double A = fastPow10f(yPixelToGain(EqHandle::y(), m_heigth, m_pixelsPerUnitHeight) / 40);
	double alpha = s * std::sinh(ln2 / 2 * Q * w0 / std::sin(w0));

	//calc coefficents
	double b0 = 1 + alpha * A;
	double b1 = -2 * c;
	double b2 = 1 - alpha * A;
	double a0 = 1 + alpha / A;
	double a1 = -2 * c;
	double a2 = 1 - alpha / A;

	//normalise
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;
	a0 = 1;

	double freq = xPixelToFreq( x, m_width );
	double gain = calculateGain( freq, a1, a2, b0, b1, b2 );
	float y = gainToYPixel( gain, m_heigth, m_pixelsPerUnitHeight );

	return y;
}




float EqHandle::getHighShelfCurve( float x )
{
	double freqZ = xPixelToFreq( EqHandle::x(), m_width );
	double w0 = 2 * std::numbers::pi * freqZ / Engine::audioEngine()->outputSampleRate();
	double c = std::cos(w0);
	double s = std::sin(w0);
	double A = fastPow10f(yPixelToGain(EqHandle::y(), m_heigth, m_pixelsPerUnitHeight) * 0.025);
	double beta = std::sqrt(A) / m_resonance;

	//calc coefficents
	double b0 = A * ((A + 1) + (A - 1) * c + beta * s);
	double b1 = -2 * A * ((A - 1) + (A + 1) * c);
	double b2 = A * ((A + 1) + (A - 1) * c - beta * s);
	double a0 = (A + 1) - (A - 1) * c + beta * s;
	double a1 = 2 * ((A - 1) - (A + 1) * c);
	double a2 = (A + 1) - (A - 1) * c - beta * s;

	//normalise
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;
	a0 = 1;

	double freq = xPixelToFreq( x, m_width );
	double gain = calculateGain( freq, a1, a2, b0, b1, b2 );
	float y = gainToYPixel( gain, m_heigth, m_pixelsPerUnitHeight );

	return y;
}




float EqHandle::getLowShelfCurve( float x )
{
	double freqZ = xPixelToFreq( EqHandle::x(), m_width );
	double w0 = 2 * std::numbers::pi * freqZ / Engine::audioEngine()->outputSampleRate();
	double c = std::cos(w0);
	double s = std::sin(w0);
	double A = fastPow10f(yPixelToGain(EqHandle::y(), m_heigth, m_pixelsPerUnitHeight) / 40);
	double beta = std::sqrt(A) / m_resonance;

	//calc coefficents
	double b0 = A * ((A + 1) - (A - 1) * c + beta * s);
	double b1 = 2 * A * ((A - 1) - (A + 1) * c);
	double b2 = A * ((A + 1) - (A - 1) * c - beta * s);
	double a0 = (A + 1) + (A - 1) * c + beta * s;
	double a1 = -2 * ((A - 1) + (A + 1) * c);
	double a2 = (A + 1) + (A - 1) * c - beta * s;

	//normalise
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;
	a0 = 1;

	double freq = xPixelToFreq( x, m_width );
	double gain = calculateGain( freq, a1, a2, b0, b1, b2 );
	float y = gainToYPixel( gain, m_heigth, m_pixelsPerUnitHeight );

	return y;
}




float EqHandle::getLowCutCurve( float x )
{
	double freqZ = xPixelToFreq( EqHandle::x(), m_width );
	double w0 = 2 * std::numbers::pi * freqZ / Engine::audioEngine()->outputSampleRate();
	double c = std::cos(w0);
	double s = std::sin(w0);
	double resonance = getResonance();
	double alpha = s / (2 * resonance);

	double b0 = (1 + c) * 0.5;
	double b1 = (-(1 + c));
	double b2 = (1 + c) * 0.5;
	double a0 = 1 + alpha;
	double a1 = (-2 * c);
	double a2 = 1 - alpha;

	//normalise
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;
	a0 = 1;

	double freq = xPixelToFreq( x, m_width );
	double gain = calculateGain( freq, a1, a2, b0, b1, b2 );
	if ( m_hp24 )
	{
		gain = gain * 2;
	}
	if ( m_hp48 )
	{
		gain = gain * 3;
	}
	float y = gainToYPixel( gain, m_heigth, m_pixelsPerUnitHeight );

	return y;
}




float EqHandle::getHighCutCurve( float x )
{
	double freqZ = xPixelToFreq( EqHandle::x(), m_width );
	double w0 = 2 * std::numbers::pi * freqZ / Engine::audioEngine()->outputSampleRate();
	double c = std::cos(w0);
	double s = std::sin(w0);
	double resonance = getResonance();
	double alpha = s / (2 * resonance);

	double b0 = (1 - c) * 0.5;
	double b1 = 1 - c;
	double b2 = (1 - c) * 0.5;
	double a0 = 1 + alpha;
	double a1 = -2 * c;
	double a2 = 1 - alpha;

	//normalise
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;
	a0 = 1;

	double freq = xPixelToFreq( x, m_width );
	double gain = calculateGain( freq, a1, a2, b0, b1, b2 );
	if ( m_lp24 )
	{
		gain = gain * 2;
	}
	if ( m_lp48 )
	{
		gain = gain * 3;
	}
	float y = gainToYPixel( gain, m_heigth, m_pixelsPerUnitHeight );

	return y;
}




float EqHandle::getResonance()
{
	return m_resonance;
}




int EqHandle::getNum()
{
	return m_numb;
}




void EqHandle::setType( EqHandleType t )
{
	EqHandle::m_type = t;
}




void EqHandle::setResonance( float r )
{
	EqHandle::m_resonance = r;
}




bool EqHandle::isMouseHover()
{
	return m_mouseHover;
}




void EqHandle::setMouseHover( bool d )
{
	m_mouseHover = d;
}




EqHandleType EqHandle::getType()
{
	return m_type;
}




bool EqHandle::isActiveHandle()
{
	return m_active;
}




void EqHandle::setHandleActive( bool a )
{
	EqHandle::m_active = a;
}




void EqHandle::sethp12()
{
	m_hp12 = true;
	m_hp24 = false;
	m_hp48 = false;
}




void EqHandle::sethp24()
{
	m_hp12 = false;
	m_hp24 = true;
	m_hp48 = false;
}




void EqHandle::sethp48()
{
	m_hp12 = false;
	m_hp24 = false;
	m_hp48 = true;
}




void EqHandle::setlp12()
{
	m_lp12 = true;
	m_lp24 = false;
	m_lp48 = false;
}




void EqHandle::setlp24()
{
	m_lp12 = false;
	m_lp24 = true;
	m_lp48 = false;
}




void EqHandle::setlp48()
{
	m_lp12 = false;
	m_lp24 = false;
	m_lp48 = true;
}




double EqHandle::calculateGain(const double freq, const double a1, const double a2, const double b0, const double b1, const double b2 )
{
	const double w = std::sin(std::numbers::pi * freq / Engine::audioEngine()->outputSampleRate());
	const double PHI = w * w * 4;

	auto bb = b0 + b1 + b2;
	auto aa = 1 + a1 + a2;
	double gain = 10 * std::log10(bb * bb + (b0 * b2 * PHI - (b1 * (b0 + b2) + 4 * b0 * b2)) * PHI)
		- 10 * std::log10(aa * aa + (1 * a2 * PHI - (a1 * (1 + a2) + 4 * 1 * a2)) * PHI);
	return gain;
}




void EqHandle::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
	if( event->button() == Qt::LeftButton )
	{
		m_mousePressed = true;
		QGraphicsItem::mousePressEvent( event );
	}
}




void EqHandle::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
	if( event->button() == Qt::LeftButton )
	{
		m_mousePressed = false;
		QGraphicsItem::mouseReleaseEvent( event );
	}
}




void EqHandle::wheelEvent( QGraphicsSceneWheelEvent *wevent )
{
	float highestBandwich = m_type != EqHandleType::Para ? 10 : 4;
	int numDegrees = wevent->delta() / 120;
	float numSteps = 0;
	if( wevent->modifiers() == Qt::ControlModifier )
	{
		numSteps = numDegrees * 0.01;
	}
	else
	{
		 numSteps = numDegrees * 0.15;
	}

	if( wevent->orientation() == Qt::Vertical )
	{
		m_resonance = std::clamp(m_resonance + numSteps, 0.1f, highestBandwich);

		emit positionChanged();
	}
	wevent->accept();
}




void EqHandle::hoverEnterEvent( QGraphicsSceneHoverEvent *hevent )
{
	setMouseHover( true );
}




void EqHandle::hoverLeaveEvent( QGraphicsSceneHoverEvent *hevent )
{
	setMouseHover( false );
}




QVariant EqHandle::itemChange( QGraphicsItem::GraphicsItemChange change, const QVariant &value )
{
	if( change == ItemPositionChange )
	{
		// pass filter don't move in y direction
		if ( m_type == EqHandleType::HighPass || m_type == EqHandleType::LowPass )
		{
			float newX = value.toPointF().x();
			if( newX < 0 )
			{
				newX = 0;
			}
			if( newX > m_width )
			{
				newX = m_width;
			}
			return QPointF(newX, m_heigth/2);
		}
	}

	QPointF newPos = value.toPointF();
	QRectF rect = QRectF( 0, 0, m_width, m_heigth );
	if( !rect.contains( newPos ) )
	{
		// Keep the item inside the scene rect.
		newPos.setX( qMin( rect.right(), qMax( newPos.x(), rect.left() ) ) );
		newPos.setY( qMin( rect.bottom(), qMax( newPos.y(), rect.top() ) ) );
		return newPos;
	}
	return QGraphicsItem::itemChange( change, value );
}




// ----------------------------------------------------------------------
//
// Class EqCurve
//
// Every Handle calculates its own curve.
// But EqCurve generates an average curve.
//
// ----------------------------------------------------------------------

EqCurve::EqCurve( QList<EqHandle*> *handle, int x, int y ) :
	m_handle( handle ),
	m_width( x ),
	m_heigth( y ),
	m_alpha( 0 ),
	m_modelChanged( false )
{
}




QRectF EqCurve::boundingRect() const
{
	return QRect( 0, 0, m_width, m_heigth );
}




void EqCurve::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
	painter->setRenderHint( QPainter::Antialiasing, true );
	if( m_modelChanged )
	{
		setModelChanged( false );
		//counts the active bands
		int activeHandles=0;
		for ( int thatHandle = 0; thatHandle<m_handle->count(); thatHandle++ )
		{
			if ( m_handle->at(thatHandle)->isActiveHandle() == true )
			{
				activeHandles++;
			}
		}
		//Computes the main curve
		//if a band is active the curve will be computed by averaging the curves of each band
		QMap<float,float> mainCurve;
		for ( int thatHandle = 0; thatHandle<m_handle->count(); thatHandle++ )
		{
			if ( m_handle->at(thatHandle)->isActiveHandle() == true )
			{
				for ( int x = 0; x < m_width ; x=x+1 )
				{
					if ( m_handle->at( thatHandle )->getType() == EqHandleType::HighPass )
					{
						mainCurve[x]= ( mainCurve[x] + ( m_handle->at( thatHandle )->getLowCutCurve( x ) * ( activeHandles ) ) - ( ( activeHandles * ( m_heigth/2 ) ) - m_heigth ) );
					}
					if ( m_handle->at(thatHandle)->getType() == EqHandleType::LowShelf )
					{
						mainCurve[x]= ( mainCurve[x] + ( m_handle->at( thatHandle )->getLowShelfCurve( x ) * ( activeHandles ) ) - ( ( activeHandles * ( m_heigth/2 ) ) - m_heigth ) );
					}
					if ( m_handle->at( thatHandle )->getType() == EqHandleType::Para )
					{
						mainCurve[x]= ( mainCurve[x] + ( m_handle->at( thatHandle )->getPeakCurve( x ) * ( activeHandles ) ) - ( ( activeHandles * ( m_heigth/2 ) ) - m_heigth ) );
					}
					if ( m_handle->at( thatHandle )->getType() == EqHandleType::HighShelf )
					{
						mainCurve[x]= ( mainCurve[x] + ( m_handle->at( thatHandle )->getHighShelfCurve( x ) * ( activeHandles ) ) - ( ( activeHandles * ( m_heigth/2 ) ) - m_heigth ) );
					}
					if ( m_handle->at(thatHandle)->getType() == EqHandleType::LowPass )
					{
						mainCurve[x]= ( mainCurve[x] + ( m_handle->at( thatHandle )->getHighCutCurve( x ) * ( activeHandles ) ) - ( ( activeHandles * ( m_heigth/2 ) ) - m_heigth ) );
					}
				}
			}
		}
		//compute a QPainterPath
		m_curve = QPainterPath();
		//only draw the EQ curve if there are any activeHandles
		if (activeHandles != 0)
		{
			for (int x = 0; x < m_width; x++)
			{
				mainCurve[x] = ((mainCurve[x] / activeHandles)) - (m_heigth / 2);
				if (x == 0)
				{
					m_curve.moveTo(x, mainCurve[x]);
				}
				m_curve.lineTo(x, mainCurve[x]);
			}
		}
		//we cache the curve painting in a pixmap for saving cpu
		QPixmap cacheMap( boundingRect().size().toSize() );
		cacheMap.fill( QColor( 0, 0, 0, 0 ) );
		QPainter cachePainter( &cacheMap );
		cachePainter.setRenderHint( QPainter::Antialiasing, true );
		QPen pen;
		pen.setWidth( 2 );
		pen.setColor( Qt::white );
		cachePainter.setPen( pen );
		cachePainter.drawPath( m_curve );
		cachePainter.end();

		m_curvePixmapCache.fill( QColor( 0, 0, 0, 0 ) );
		m_curvePixmapCache.swap( cacheMap );
	}
	//we paint our cached curve pixmap
	painter->drawPixmap( 0, 0, m_width, m_heigth, m_curvePixmapCache );
	// if mouse hover a handle, m_alpha counts up slow for blend in the filled EQ curve
	// todo: a smarter way of this "if-monster"
	QColor curveColor;
	if( m_handle->at( 0 )->isMouseHover()
		 || m_handle->at( 1 )->isMouseHover()
		 || m_handle->at( 2 )->isMouseHover()
		 || m_handle->at( 3 )->isMouseHover()
		 || m_handle->at( 4 )->isMouseHover()
		 || m_handle->at( 5 )->isMouseHover()
		 || m_handle->at( 6 )->isMouseHover()
		 || m_handle->at( 7 )->isMouseHover()
		 )
	{
		if ( m_alpha < 40 )
		{
			m_alpha = m_alpha + 10;
		}
	}
	else
	{
		if ( m_alpha > 0 )
		{
			m_alpha = m_alpha - 10;
		}
	}
	//draw on mouse hover the curve of hovered filter in different colors
	for ( int i = 0; i < m_handle->count(); i++ )
	{
		if ( m_handle->at(i)->isMouseHover() )
		{
			curveColor = QColor( qRgba( 6, 106, 43, 242 ));
			QPen pen ( curveColor);
			pen.setWidth( 2 );
			painter->setPen( pen );
			painter->drawPath( m_handle->at( i )->getCurvePath() );
		}
	}
	//draw on mouse hover the EQ curve filled. with m_alpha it blends in and out smooth
	QPainterPath cPath;
	cPath.addPath( m_curve );
	cPath.lineTo( cPath.currentPosition().x(), m_heigth );
	cPath.lineTo( cPath.elementAt( 0 ).x  , m_heigth );
	painter->fillPath( cPath, QBrush ( QColor( 255,255,255, m_alpha ) ) );
}




void EqCurve::setModelChanged( bool mc )
{
	m_modelChanged = mc;
}


} // namespace lmms::gui
