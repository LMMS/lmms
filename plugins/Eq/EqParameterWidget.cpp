/*
 * eqparameterwidget.cpp - defination of EqParameterWidget class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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


#include "EqParameterWidget.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWidget>

#include "AutomatableModel.h"
#include "EqCurve.h"


namespace lmms::gui
{


EqParameterWidget::EqParameterWidget( QWidget *parent, EqControls * controls ) :
	QWidget( parent ),
	m_displayWidth ( 450 ),
	m_displayHeigth ( 200 ),
	m_controls ( controls )

{
	m_bands = new EqBand[8];
	resize( m_displayWidth, m_displayHeigth );
	float totalHeight = 36; // gain range from -18 to +18
	m_pixelsPerUnitHeight = m_displayHeigth / totalHeight;
	m_pixelsPerOctave = EqHandle::freqToXPixel( 10000, m_displayWidth ) - EqHandle::freqToXPixel( 5000, m_displayWidth );

	//GraphicsScene and GraphicsView stuff
	auto scene = new QGraphicsScene(this);
	scene->setSceneRect( 0, 0, m_displayWidth, m_displayHeigth );
	auto view = new QGraphicsView(this);
	view->setStyleSheet( "border-style: none; background: transparent;" );
	view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	view->setScene( scene );

	//adds the handles
	m_handleList.reserve(bandCount());
	for ( int i = 0; i < bandCount(); i++ )
	{
		m_handle = new EqHandle ( i, m_displayWidth, m_displayHeigth );
		m_handleList.append(m_handle);
		m_handle->setZValue( 1 );
		scene->addItem( m_handle );
	}

	//adds the curve widget
	m_eqcurve = new EqCurve(&m_handleList, m_displayWidth, m_displayHeigth);
	scene->addItem( m_eqcurve );
	for ( int i = 0; i < bandCount(); i++ )
	{
		// if the data of handle position has changed update the models
		QObject::connect(m_handleList.at(i), SIGNAL(positionChanged()), this, SLOT(updateModels()));
	}
}




EqParameterWidget::~EqParameterWidget()
{
	if( m_bands )
	{
		delete[] m_bands;
		m_bands = 0;
	}
}




EqBand *EqParameterWidget::getBandModels( int i )
{
	return &m_bands[i];
}




void EqParameterWidget::updateHandle()
{
	m_eqcurve->setModelChanged( true );
	for( int i = 0 ; i < bandCount(); i++ )
	{
		if (!m_handleList.at(i)->mousePressed()) // prevents a short circuit between handle and data model
		{
			//sets the band on active if a fader or a knob is moved
			bool hover = false; // prevents an action if handle is moved
			for ( int j = 0; j < bandCount(); j++ )
			{
				if (m_handleList.at(j)->isMouseHover()) { hover = true; }
			}
			if ( !hover )
			{
				if ( sender() == m_bands[i].gain ) m_bands[i].active->setValue( true );
				if ( sender() == m_bands[i].freq ) m_bands[i].active->setValue( true );
				if ( sender() == m_bands[i].res ) m_bands[i].active->setValue( true );
			}
			changeHandle( i );
		}
		else { m_handleList.at(i)->setHandleActive(m_bands[i].active->value()); }
	}
	if (m_bands[0].hp12->value()) m_handleList.at(0)->sethp12();
	if (m_bands[0].hp24->value()) m_handleList.at(0)->sethp24();
	if (m_bands[0].hp48->value()) m_handleList.at(0)->sethp48();
	if (m_bands[7].lp12->value()) m_handleList.at(7)->setlp12();
	if (m_bands[7].lp24->value()) m_handleList.at(7)->setlp24();
	if (m_bands[7].lp48->value()) m_handleList.at(7)->setlp48();
}




void EqParameterWidget::changeHandle( int i )
{
	//fill x, y, and bw with data from model
	float x = EqHandle::freqToXPixel( m_bands[i].freq->value(), m_displayWidth );
	float y = m_handleList.at(i)->y();
	//for pass filters there is no gain model
	if( m_bands[i].gain )
	{
		float gain = m_bands[i].gain->value();
		y = EqHandle::gainToYPixel( gain, m_displayHeigth, m_pixelsPerUnitHeight );
	}
	float bw = m_bands[i].res->value();

	// set the handle position, filter type for each handle
	switch ( i )
	{
	case 0 :
		m_handleList.at(i)->setType(EqHandleType::HighPass);
		m_handleList.at(i)->setPos(x, m_displayHeigth / 2);
		break;
	case 1:
		m_handleList.at(i)->setType(EqHandleType::LowShelf);
		m_handleList.at(i)->setPos(x, y);
		break;
	case 2:
		m_handleList.at(i)->setType(EqHandleType::Para);
		m_handleList.at(i)->setPos(x, y);
		break;
	case 3:
		m_handleList.at(i)->setType(EqHandleType::Para);
		m_handleList.at(i)->setPos(x, y);
		break;
	case 4:
		m_handleList.at(i)->setType(EqHandleType::Para);
		m_handleList.at(i)->setPos(x, y);
		break;
	case 5:
		m_handleList.at(i)->setType(EqHandleType::Para);
		m_handleList.at(i)->setPos(x, y);
		break;
	case 6:
		m_handleList.at(i)->setType(EqHandleType::HighShelf);
		m_handleList.at(i)->setPos(x, y);
		break;
	case 7:
		m_handleList.at(i)->setType(EqHandleType::LowPass);
		m_handleList.at(i)->setPos(QPointF(x, m_displayHeigth / 2));
		break;
	}

	// set resonance/bandwidth for each handle
	if (m_handleList.at(i)->getResonance() != bw) { m_handleList.at(i)->setResonance(bw); }

	// and the active status
	m_handleList.at(i)->setHandleActive(m_bands[i].active->value());
	m_handleList.at(i)->update();
	m_eqcurve->update();
}



//this is called if a handle is moved
void EqParameterWidget::updateModels()
{
	for ( int i=0 ; i < bandCount(); i++ )
	{
		m_bands[i].freq->setValue(EqHandle::xPixelToFreq(m_handleList.at(i)->x(), m_displayWidth));

		if( m_bands[i].gain )
		{
			m_bands[i].gain->setValue(
				EqHandle::yPixelToGain(m_handleList.at(i)->y(), m_displayHeigth, m_pixelsPerUnitHeight));
		}

		m_bands[i].res->setValue(m_handleList.at(i)->getResonance());
		//identifies the handle which is moved and set the band active
		if (sender() == m_handleList.at(i)) { m_bands[i].active->setValue(true); }
	}
	m_eqcurve->update();
}




EqBand::EqBand() :
	gain ( 0 ),
	res ( 0 ),
	freq ( 0 ),
	color ( QColor( 255, 255, 255 ) ),
	x( 0 ),
	y( 0 ),
	name ( QString( "" ) ),
	peakL( 0 ),
	peakR( 0 )
{
}


} // namespace lmms::gui
