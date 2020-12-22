/*
 * delaycontrolsdialog.cpp - definition of DelayControlsDialog class.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
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

#include "DelayControlsDialog.h"
#include "DelayControls.h"
#include "embed.h"
#include "TempoSyncKnob.h"
#include "../Eq/EqFader.h"
#include <QMouseEvent>
#include <QPainter>




DelayControlsDialog::DelayControlsDialog( DelayControls *controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 300, 208 );

	TempoSyncKnob* sampleDelayKnob = new TempoSyncKnob( knobBright_26, this );
	sampleDelayKnob->move( 10,14 );
	sampleDelayKnob->setVolumeKnob( false );
	sampleDelayKnob->setModel( &controls->m_delayTimeModel );
	sampleDelayKnob->setLabel( tr( "DELAY" ) );
	sampleDelayKnob->setHintText( tr( "Delay time" ) + " ", " s" );

	Knob * feedbackKnob = new Knob( knobBright_26, this );
	feedbackKnob->move( 11, 58 );
	feedbackKnob->setVolumeKnob( true) ;
	feedbackKnob->setModel( &controls->m_feedbackModel);
	feedbackKnob->setLabel( tr( "FDBK" ) );
	feedbackKnob->setHintText( tr ( "Feedback amount" ) + " " , "" );

	TempoSyncKnob * lfoFreqKnob = new TempoSyncKnob( knobBright_26, this );
	lfoFreqKnob->move( 11, 119 );
	lfoFreqKnob->setVolumeKnob( false );
	lfoFreqKnob->setModel( &controls->m_lfoTimeModel );
	lfoFreqKnob->setLabel( tr( "RATE" ) );
	lfoFreqKnob->setHintText( tr ( "LFO frequency") + " ", " s" );

	TempoSyncKnob * lfoAmtKnob = new TempoSyncKnob( knobBright_26, this );
	lfoAmtKnob->move( 11, 159 );
	lfoAmtKnob->setVolumeKnob( false );
	lfoAmtKnob->setModel( &controls->m_lfoAmountModel );
	lfoAmtKnob->setLabel( tr( "AMNT" ) );
	lfoAmtKnob->setHintText( tr ( "LFO amount" ) + " " , " s" );

	EqFader * outFader = new EqFader( &controls->m_outGainModel,tr( "Out gain" ),
									  this, &controls->m_outPeakL, &controls->m_outPeakR );
	outFader->setMaximumHeight( 196 );
	outFader->move( 263, 45 );
	outFader->setDisplayConversion( false );
	outFader->setHintText( tr( "Gain" ), "dBFS" );

	XyPad * pad = new XyPad( this, &controls->m_feedbackModel, &controls->m_delayTimeModel );
	pad->resize( 200, 200 );
	pad->move( 50, 5 );
}




XyPad::XyPad(QWidget *parent, FloatModel *xModel, FloatModel *yModel) :
	QWidget( parent ),
	m_xModel( xModel ),
	m_yModel( yModel ),
	m_acceptInput( false )
{
	connect( m_xModel, SIGNAL( dataChanged() ) , this, SLOT( update() ) );
	connect( m_yModel, SIGNAL( dataChanged() ) , this, SLOT( update() ) );
}




void XyPad::paintEvent(QPaintEvent *event)
{
	QPainter painter( this );
		//Draw Frequecy maker lines
		painter.setPen( QPen( QColor( 200, 200, 200, 200 ), 8, Qt::SolidLine, Qt::RoundCap, Qt::BevelJoin ) );
		painter.setRenderHint( QPainter::Antialiasing, true );

		float xRange = m_xModel->maxValue() - m_xModel->minValue();
		float xInc = xRange / width();
		int xPos = ( m_xModel->value() - m_xModel->minValue() ) / xInc;

		float yRange = m_yModel->maxValue() - m_yModel->minValue();
		float yInc = yRange / height();
		int yPos = ( m_yModel->value() - m_yModel->minValue() ) / yInc;

		painter.drawPoint( xPos, yPos );
}




void XyPad::mousePressEvent(QMouseEvent *event)
{
	m_acceptInput = true;
}




void XyPad::mouseReleaseEvent(QMouseEvent *event)
{
	m_acceptInput = false;
}




void XyPad::mouseMoveEvent(QMouseEvent *event)
{
	if( m_acceptInput && (event->x() >= 0) && ( event->x() < width() )
			&& ( event->y() >= 0) && ( event->y() < height() ) )
	{
		//set xmodel
		float xRange = m_xModel->maxValue() - m_xModel->minValue();
		float xInc = xRange / width();
		m_xModel->setValue( m_xModel->minValue() + ( event->x() * xInc ) );

		//set ymodel
		float yRange = m_yModel->maxValue() - m_yModel->minValue();
		float yInc = yRange / height();
		m_yModel->setValue( m_yModel->minValue() + ( event->y() * yInc ) );
	}
}
