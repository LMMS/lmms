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

#include "EqParameterWidget.h"
#include "QPainter"
#include "qwidget.h"
#include "lmms_math.h"
#include "MainWindow.h"
#include "QMouseEvent"

EqParameterWidget::EqParameterWidget( QWidget *parent ) :
	QWidget( parent ),
	m_bands ( 0 ),
	m_selectedBand ( 0 )
{
	m_bands = new EqBand[8];
	resize( 250, 116 );
	//    connect( Engine::mainWindow(), SIGNAL( periodicUpdate() ), this, SLOT( update() ) );
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(100);
	float totalLength = log10( 21000 );
	m_pixelsPerUnitWidth = width( ) /  totalLength ;
	float totalHeight = 80;
	m_pixelsPerUnitHeight = (height() - 4) / ( totalHeight );
	m_scale = 1.5;
	m_pixelsPerOctave = freqToXPixel( 10000 ) - freqToXPixel( 5000 );
}




EqParameterWidget::~EqParameterWidget()
{
	if(m_bands)
	{
		delete m_bands;
	}
}




void EqParameterWidget::paintEvent( QPaintEvent *event )
{
	QPainter painter( this );
	//Draw Frequecy maker lines
	painter.setPen( QPen( QColor( 100, 100, 100, 200 ), 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin ) );
	for( int x = 20 ; x < 100; x += 10)
	{
		painter.drawLine( freqToXPixel( x )  , 0, freqToXPixel( x ) , height() );
	}
	for( int x = 100 ; x < 1000; x += 100)
	{
		painter.drawLine( freqToXPixel( x )  , 0, freqToXPixel( x ) , height() );
	}
	for( int x = 1000 ; x < 11000; x += 1000)
	{
		painter.drawLine( freqToXPixel( x )  , 0, freqToXPixel( x ) , height() );
	}
	//draw 0dB line
	painter.drawLine(0, gainToYPixel( 0 ) , width(), gainToYPixel( 0 ) );

	for( int i = 0 ; i < bandCount() ; i++ )
	{
		m_bands[i].color.setAlpha( m_bands[i].active->value() ? activeAplha() : inactiveAlpha() );
		painter.setPen( QPen( m_bands[i].color, 1, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin ) );
		float  x = freqToXPixel( m_bands[i].freq->value() );
		float y = height() * 0.5;
		float gain = 1;
		if( m_bands[i].gain )
		{
			gain = m_bands[i].gain->value();
		}
		y = gainToYPixel( gain );
		float bw = m_bands[i].freq->value() / m_bands[i].res->value();
		m_bands[i].x = x; m_bands[i].y = y;
		const int radius = 7;
		painter.drawEllipse( x - radius , y - radius, radius * 2 ,radius * 2 );
		QString msg = QString ( "%1" ).arg ( QString::number (i + 1) );
		painter.drawText(x - ( radius * 0.5 ), y + ( radius * 0.85 ), msg );
		painter.setPen( QPen( m_bands[i].color, 1, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin ) );
		if( i == 0 || i == bandCount() - 1 )
		{
			painter.drawLine(x , y, x, y - (m_bands[i].res->value() * 4  ) );
		}
		else
		{
			painter.drawLine(freqToXPixel(m_bands[i].freq->value()-(bw * 0.5)),y,freqToXPixel(m_bands[i].freq->value()+(bw * 0.5)),y);
		}
	}
}




void EqParameterWidget::mousePressEvent( QMouseEvent *event )
{
	m_oldX = event->x(); m_oldY = event->y();
	m_selectedBand = selectNearestHandle( event->x(), event->y() );
	m_mouseAction = none;
	if ( event->button() == Qt::LeftButton ) m_mouseAction = drag;
	if ( event->button() == Qt::RightButton ) m_mouseAction = res;
}




void EqParameterWidget::mouseReleaseEvent( QMouseEvent *event )
{
	m_selectedBand = 0;
	m_mouseAction = none;
}




void EqParameterWidget::mouseMoveEvent( QMouseEvent *event )
{
	int deltaX = event->x() - m_oldX;
	int deltaR = event->y() - m_oldY;
	m_oldX = event->x(); m_oldY = event->y();
	if(m_selectedBand && m_selectedBand->active->value() )
	{
		switch ( m_mouseAction ) {
		case none :
			break;
		case drag:
			if( m_selectedBand->freq ) m_selectedBand->freq->setValue( xPixelToFreq( m_oldX ) );
			if( m_selectedBand->gain )m_selectedBand->gain->setValue( yPixelToGain( m_oldY ) );
			break;
		case res:
			if( m_selectedBand->res )m_selectedBand->res->incValue( deltaX * resPixelMultiplyer() );
			if( m_selectedBand->res )m_selectedBand->res->incValue( (-deltaR) * resPixelMultiplyer() );
			break;
		default:
			break;
		}
	}
}




void EqParameterWidget::mouseDoubleClickEvent( QMouseEvent *event )
{
	EqBand* selected = selectNearestHandle( event->x() , event->y() );
	if( selected )
	{
		selected->active->setValue( selected->active->value() ? 0 : 1 );
	}
}




EqBand*  EqParameterWidget::selectNearestHandle( const int x, const  int y )
{
	EqBand* selectedModel = 0;
	float* distanceToHandles = new float[bandCount()];
	//calc distance to each handle
	for( int i = 0 ; i < bandCount() ; i++ )
	{
		int xOffset = m_bands[i].x - x;
		int yOffset = m_bands[i].y - y;
		distanceToHandles[i] = fabs( sqrt( ( xOffset * xOffset ) + ( yOffset * yOffset ) ) );
	}
	//select band
	int shortestBand = 0;
	for ( int i = 1 ; i < bandCount() ; i++ )
	{
		if ( distanceToHandles [i] < distanceToHandles[shortestBand] ){
			shortestBand = i;
		}
	}
	if(distanceToHandles[shortestBand] <  maxDistanceFromHandle() )
	{
		selectedModel = &m_bands[shortestBand];
	}
	delete distanceToHandles;
	return selectedModel;
}




EqBand::EqBand() :
	gain ( 0 ),
	res ( 0 ),
	freq ( 0 ),
	color ( QColor( 255, 255, 255 ) ),
	name ( QString( "" ) )
{
}
