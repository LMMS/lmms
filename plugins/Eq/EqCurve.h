/*
 * EqCurve.h - defination of EqCurve and EqHandle classes.
 *
* Copyright (c) 2015 Steffen Baranowsky <BaraMGB/at/freenet/dot/de>
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

#ifndef EQCURVE_H
#define EQCURVE_H

#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneWheelEvent>
#include "lmms_math.h"


enum{
	highpass=1,
	lowshelf,
	para,
	highshelf,
	lowpass
};





// implements the Eq_Handle to control a band
class EqHandle : public QGraphicsObject
{
	Q_OBJECT
public:
	EqHandle( int num, int x, int y );
	void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget );
	QPainterPath getCurvePath();
	float getPeakCurve( float x );
	float getHighShelfCurve( float x );
	float getLowShelfCurve( float x );
	float getLowCutCurve( float x );
	float getHighCutCurve( float x );
	float getResonance();
	int getNum();
	int getType();
	void setType( int t );
	void setResonance( float r );
	bool isMouseHover();
	void setMouseHover( bool d );
	bool isActiveHandle();
	void setHandleActive( bool a );
	void setHandleMoved(bool a);
	bool getHandleMoved();
	void sethp12();
	void sethp24();
	void sethp48();
	void setlp12();
	void setlp24();
	void setlp48();
private:
	long double PI;
	float m_pixelsPerUnitWidth;
	float m_pixelsPerUnitHeight;
	float m_scale;
	bool m_hp12;
	bool m_hp24;
	bool m_hp48;
	bool m_lp12;
	bool m_lp24;
	bool m_lp48;
	bool m_mouseHover;
	bool m_active;
	int m_type, m_numb;
	float m_resonance;
	float m_width, m_heigth;
	bool m_mousePressed;
	bool m_handleMoved;
	QRectF boundingRect() const;




	inline float freqToXPixel( float freq )
	{
		float min = log ( 27) / log( 10 );
		float max = log ( 20000 )/ log( 10 );
		float range = max - min;
		return ( log( freq ) / log( 10 ) - min ) / range * m_width;
	}




	inline float xPixelToFreq( float x )
	{
		float min = log ( 27) / log( 10 );
		float max = log ( 20000 ) / log( 10 );
		float range = max - min;
		return pow( 10 , x * ( range / m_width ) + min );
	}




	inline float gainToYPixel( float gain )
	{
		return ( m_heigth ) - ( gain * m_pixelsPerUnitHeight ) - ( ( m_heigth ) * 0.5 );
	}




	inline float yPixelToGain( float y )
	{
		return ( ( 0.5 * m_heigth ) - y ) / m_pixelsPerUnitHeight;
	}




signals:
	void positionChanged();
private slots:
	void handleMoved();


protected:
	void mousePressEvent( QGraphicsSceneMouseEvent *event );
	void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );
	void wheelEvent( QGraphicsSceneWheelEvent *wevent );
	void hoverEnterEvent( QGraphicsSceneHoverEvent *hevent );
	void hoverLeaveEvent( QGraphicsSceneHoverEvent *hevent );
	QVariant itemChange( GraphicsItemChange change, const QVariant &value );
};



class EqCurve : public QGraphicsObject
{
	Q_OBJECT
public:
	EqCurve( QList<EqHandle*> *handle, int x, int y );
	QList<EqHandle*> *m_handle;
	QPainterPath m_curve;
	QRectF boundingRect() const;
	void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget );
	void setModelChanged(bool mc);
private:
	int m_width, m_heigth;
	int m_alpha;
	bool m_modelChanged;

	float m_pixelsPerUnitHeight;
	float m_scale;




	inline float freqToXPixel( float freq )
	{
		float min = log ( 27) / log( 10 );
		float max = log ( 20000 ) / log( 10 );
		float range = max - min;
		return ( log( freq ) / log( 10 ) - min ) / range * m_width;
	}




	inline float xPixelToFreq( float x )
	{
		float min = log ( 27) / log( 10 );
		float max = log ( 20000 ) / log( 10 );
		float range = max - min;
		return pow( 10 , x * ( range / m_width ) + min );
	}




	inline float gainToYPixel( float gain )
	{
		return ( m_heigth ) - ( gain * m_pixelsPerUnitHeight ) - ( ( m_heigth ) * 0.5 );
	}




	inline float yPixelToGain( float y )
	{
		return ( ( 0.5 * m_heigth ) - y ) / m_pixelsPerUnitHeight;
	}

};

#endif // EQCURVE_H
