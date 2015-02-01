/*
 * delaycontrolsdialog.cpp - definition of DelayControlsDialog class.
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

#include "delaycontrolsdialog.h"
#include "delaycontrols.h"
#include "embed.h"
#include "TempoSyncKnob.h"




DelayControlsDialog::DelayControlsDialog( DelayControls *controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 200, 75 );

	TempoSyncKnob* sampleDelayKnob = new TempoSyncKnob( knobBright_26, this );
	sampleDelayKnob->move( 20,10 );
	sampleDelayKnob->setVolumeKnob( false );
	sampleDelayKnob->setModel( &controls->m_delayTimeModel );
	sampleDelayKnob->setLabel( tr( "Delay" ) );
	sampleDelayKnob->setHintText( tr( "Delay Time" ) + " ", " s" );

	knob * feedbackKnob = new knob( knobBright_26, this );
	feedbackKnob->move( 63,10 );
	feedbackKnob->setVolumeKnob( true) ;
	feedbackKnob->setModel( &controls->m_feedbackModel);
	feedbackKnob->setLabel( tr( "Regen" ) );
	feedbackKnob->setHintText( tr ( "Feedback Amount" ) + " " , "" );

	TempoSyncKnob * lfoFreqKnob = new TempoSyncKnob( knobBright_26, this );
	lfoFreqKnob->move( 106,10 );
	lfoFreqKnob->setVolumeKnob( false );
	lfoFreqKnob->setModel( &controls->m_lfoTimeModel );
	lfoFreqKnob->setLabel( tr( "Rate" ) );
	lfoFreqKnob->setHintText( tr ( "Lfo") + " ", " s" );

	TempoSyncKnob * lfoAmtKnob = new TempoSyncKnob( knobBright_26, this );
	lfoAmtKnob->move( 150,10 );
	lfoAmtKnob->setVolumeKnob( false );
	lfoAmtKnob->setModel( &controls->m_lfoAmountModel );
	lfoAmtKnob->setLabel( tr( "Lfo" ) );
	lfoAmtKnob->setHintText( tr ( "Lfo Amt" ) + " " , " s" );


}


#include "moc_delaycontrols.cxx"
