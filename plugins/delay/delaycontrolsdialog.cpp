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




DelayControlsDialog::DelayControlsDialog(DelayControls *controls) :
    EffectControlDialog( controls )
{
    setAutoFillBackground( true );
    QPalette pal;
    pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
    setPalette( pal );
    setFixedSize( 100,125 );

    TempoSyncKnob* sampleDelayKnob = new TempoSyncKnob( knobBright_26, this );
    sampleDelayKnob->move( 20,30 );
    sampleDelayKnob->setVolumeKnob( false );
    sampleDelayKnob->setModel( &controls->m_delayTimeModel );
    sampleDelayKnob->setLabel( tr( "Delay" ) );
    sampleDelayKnob->setHintText( tr( "Delay Time Samples:" ) + " ", "" );

    knob * feedbackKnob = new knob( knobBright_26, this);
    feedbackKnob->move( 60,30 );
    feedbackKnob->setVolumeKnob(true);
    feedbackKnob->setModel( &controls->m_feedbackModel);
    feedbackKnob->setLabel( tr( "Feedback" ) );
    feedbackKnob->setHintText( tr ( "Feedback Amount:" ) + " ", "");

    TempoSyncKnob * lfoFreqKnob = new TempoSyncKnob( knobBright_26, this);
    lfoFreqKnob->move( 20,80 );
    lfoFreqKnob->setVolumeKnob(false);
    lfoFreqKnob->setModel( &controls->m_lfoTimeModel);
    lfoFreqKnob->setLabel( tr( "Lfo Hz" ) );
    lfoFreqKnob->setHintText( tr ( "Lfo Hz:" ) + " ", "");

    knob * lfoAmtKnob = new knob( knobBright_26, this);
    lfoAmtKnob->move( 60,80 );
    lfoAmtKnob->setVolumeKnob(true);
    lfoAmtKnob->setModel( &controls->m_lfoAmountModel);
    lfoAmtKnob->setLabel( tr( "Lfo Amt" ) );
    lfoAmtKnob->setHintText( tr ( "Lfo Amt:" ) + " ", "");

}
