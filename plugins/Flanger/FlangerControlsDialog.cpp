/*
 * flangercontrolsdialog.cpp - defination of FlangerControlsDialog class.
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

#include "FlangerControlsDialog.h"
#include "FlangerControls.h"
#include "LedCheckbox.h"
#include "TempoSyncKnob.h"




FlangerControlsDialog::FlangerControlsDialog( FlangerControls *controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), QPixmap( ":/flanger/artwork.png" ) );
	setPalette( pal );
	setFixedSize( 200, 75 );

	Knob* delayKnob = new Knob( knobBright_26, this );
	delayKnob->move( 20,10 );
	delayKnob->setVolumeKnob( false );
	delayKnob->setModel( &controls->m_delayTimeModel );
	delayKnob->setLabel( tr( "Delay" ) );
	delayKnob->setHintText( tr( "Delay Time:" ) + " ", "" );

	TempoSyncKnob * lfoFreqKnob = new TempoSyncKnob( knobBright_26, this );
	lfoFreqKnob->move( 53,10 );
	lfoFreqKnob->setVolumeKnob( false );
	lfoFreqKnob->setModel( &controls->m_lfoFrequencyModel );
	lfoFreqKnob->setLabel( tr( "Lfo Hz" ) );
	lfoFreqKnob->setHintText( tr ( "Lfo:" ) , "s" );

	Knob * lfoAmtKnob = new Knob( knobBright_26, this );
	lfoAmtKnob->move( 86,10 );
	lfoAmtKnob->setVolumeKnob( false );
	lfoAmtKnob->setModel( &controls->m_lfoAmountModel );
	lfoAmtKnob->setLabel( tr( "Amt" ) );
	lfoAmtKnob->setHintText( tr ( "Amt:" ) , "" );

	Knob * feedbackKnob = new Knob( knobBright_26, this );
	feedbackKnob->move( 119,10 );
	feedbackKnob->setVolumeKnob( true) ;
	feedbackKnob->setModel( &controls->m_feedbackModel );
	feedbackKnob->setLabel( tr( "Regen" ) );
	feedbackKnob->setHintText( tr ( "Feedback Amount:" ) , "" );

	Knob * whiteNoiseKnob = new Knob( knobBright_26, this );
	whiteNoiseKnob->move( 150,10 );
	whiteNoiseKnob->setVolumeKnob( true) ;
	whiteNoiseKnob->setModel( &controls->m_whiteNoiseAmountModel );
	whiteNoiseKnob->setLabel( tr( "Noise" ) );
	whiteNoiseKnob->setHintText( tr ( "White Noise Amount:" ) , "" );

	LedCheckBox* invertCb = new LedCheckBox( tr( "" ), this );
	invertCb->move( 15,55 );



}
