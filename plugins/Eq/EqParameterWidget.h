/*
 * eqparameterwidget.cpp - defination of EqParameterWidget class.
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


#ifndef EQPARAMETERWIDGET_H
#define EQPARAMETERWIDGET_H
#include <QWidget>
#include "EffectControls.h"


class EqBand
{
public :
	EqBand();
	FloatModel* gain;
	FloatModel* res;
	FloatModel* freq;
	BoolModel* active;
	QColor color;
	int x;
	int y;
	QString name;
	float* peakL;
	float* peakR;


};



class EqParameterWidget : public QWidget
{

public:
	explicit EqParameterWidget( QWidget *parent = 0 );
	~EqParameterWidget();
	const int bandCount()
	{
		return 8;
	}



	const int maxDistanceFromHandle()
	{
		return 20;
	}




	EqBand* getBandModels( int i )
	{
		return &m_bands[i];
	}




	const int activeAplha()
	{
		return 200;
	}




	const int inactiveAlpha()
	{
		return 100;
	}




	const float resPixelMultiplyer()
	{
		return 100;
	}


signals:

public slots:

protected:
	virtual void paintEvent ( QPaintEvent * event );
	virtual void mousePressEvent(QMouseEvent * event );
	virtual void mouseReleaseEvent(QMouseEvent * event);
	virtual void mouseMoveEvent(QMouseEvent * event);
	virtual void mouseDoubleClickEvent(QMouseEvent * event);

private:
	EqBand *m_bands;
	float m_pixelsPerUnitWidth;
	float m_pixelsPerUnitHeight;
	float m_pixelsPerOctave;
	float m_scale;
	EqBand* m_selectedBand;

	EqBand*  selectNearestHandle( const int x, const int y );

	enum MouseAction { none, drag, res } m_mouseAction;
	int m_oldX, m_oldY;
	int *m_xGridBands;


	inline int freqToXPixel( float freq )
	{
		return ( log10( freq ) * m_pixelsPerUnitWidth * m_scale ) - ( width() * 0.5 );
	}




	inline float xPixelToFreq( int x )
	{
		return   pow( 10, ( x + ( width() * 0.5 ) ) / ( m_pixelsPerUnitWidth * m_scale ) );
	}




	inline int gainToYPixel( float gain )
	{
		return ( height() - 3) - ( gain * m_pixelsPerUnitHeight ) - ( (height() -3 ) * 0.5);
	}




	inline float yPixelToGain( int y )
	{
		return ( ( 0.5 * height() ) - y) / m_pixelsPerUnitHeight;
	}

};


#endif // EQPARAMETERWIDGET_H
