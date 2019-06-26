/*
 * EqCurve.h - defination of EqCurve and EqHandle classes.
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

#ifndef EQCURVE_H
#define EQCURVE_H

#include <QGraphicsItem>
#include <QPainter>
#include <QGraphicsSceneWheelEvent>
#include "lmms_math.h"
#include "AutomatableModelView.h"


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

	static float freqToXPixel( float freq, int w );
	static float xPixelToFreq( float x , int w );
	static float gainToYPixel( float gain, int h, float pixelPerUnitHeight );
	static float yPixelToGain( float y, int h, float pixelPerUnitHeight );

	QRectF boundingRect() const;
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
	bool mousePressed() const;
	void sethp12();
	void sethp24();
	void sethp48();
	void setlp12();
	void setlp24();
	void setlp48();

signals:
	void positionChanged();

protected:
	void mousePressEvent( QGraphicsSceneMouseEvent *event );
	void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );
	void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget );
	void wheelEvent( QGraphicsSceneWheelEvent *wevent );
	void hoverEnterEvent( QGraphicsSceneHoverEvent *hevent );
	void hoverLeaveEvent( QGraphicsSceneHoverEvent *hevent );
	QVariant itemChange( GraphicsItemChange change, const QVariant &value );

private:
	double calculateGain( const double freq, const double a1, const double a2, const double b0, const double b1, const double b2 );
	void loadPixmap();
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
	int m_type, m_numb;
	float m_width, m_heigth;
	float m_resonance;
	bool m_mousePressed;
	bool m_active;
	QPixmap m_circlePixmap;
};




class EqCurve : public QGraphicsObject
{
	Q_OBJECT
public:
	EqCurve( QList<EqHandle*> *handle, int x, int y );
	QRectF boundingRect() const;
	void setModelChanged(bool mc);

protected:
	void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget );

private:
	QList<EqHandle*> *m_handle;
	QPainterPath m_curve;
	QPixmap m_curvePixmapCache;
	int m_width, m_heigth;
	int m_alpha;
	bool m_modelChanged;
	float m_pixelsPerUnitHeight;
	float m_scale;
};

#endif // EQCURVE_H
