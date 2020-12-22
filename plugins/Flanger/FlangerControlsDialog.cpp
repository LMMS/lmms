/*
 * flangercontrolsdialog.cpp - defination of FlangerControlsDialog class.
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

#include "FlangerControlsDialog.h"
#include "FlangerControls.h"
#include "embed.h"
#include "LedCheckbox.h"
#include "TempoSyncKnob.h"




FlangerControlsDialog::FlangerControlsDialog( FlangerControls *controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 195, 75 );

	Knob* delayKnob = new Knob( knobBright_26, this );
	delayKnob->move( 10,10 );
	delayKnob->setVolumeKnob( false );
	delayKnob->setModel( &controls->m_delayTimeModel );
	delayKnob->setLabel( tr( "DELAY" ) );
	delayKnob->setHintText( tr( "Delay time:" ) + " ", "s" );

	TempoSyncKnob * lfoFreqKnob = new TempoSyncKnob( knobBright_26, this );
	lfoFreqKnob->move( 48,10 );
	lfoFreqKnob->setVolumeKnob( false );
	lfoFreqKnob->setModel( &controls->m_lfoFrequencyModel );
	lfoFreqKnob->setLabel( tr( "RATE" ) );
	lfoFreqKnob->setHintText( tr( "Period:" ) , " Sec" );

	Knob * lfoAmtKnob = new Knob( knobBright_26, this );
	lfoAmtKnob->move( 85,10 );
	lfoAmtKnob->setVolumeKnob( false );
	lfoAmtKnob->setModel( &controls->m_lfoAmountModel );
	lfoAmtKnob->setLabel( tr( "AMNT" ) );
	lfoAmtKnob->setHintText( tr( "Amount:" ) , "" );

	Knob * feedbackKnob = new Knob( knobBright_26, this );
	feedbackKnob->move( 122,10 );
	feedbackKnob->setVolumeKnob( true) ;
	feedbackKnob->setModel( &controls->m_feedbackModel );
	feedbackKnob->setLabel( tr( "FDBK" ) );
	feedbackKnob->setHintText( tr( "Feedback amount:" ) , "" );

	Knob * whiteNoiseKnob = new Knob( knobBright_26, this );
	whiteNoiseKnob->move( 156,10 );
	whiteNoiseKnob->setVolumeKnob( true) ;
	whiteNoiseKnob->setModel( &controls->m_whiteNoiseAmountModel );
	whiteNoiseKnob->setLabel( tr( "NOISE" ) );
	whiteNoiseKnob->setHintText( tr( "White noise amount:" ) , "" );

	LedCheckBox * invertCb = new LedCheckBox( tr( "Invert" ), this );
	invertCb->move( 10,53 );



}
