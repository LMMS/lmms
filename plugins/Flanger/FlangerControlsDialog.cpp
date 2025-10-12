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

#include <QVBoxLayout>

#include "embed.h"
#include "FlangerControls.h"
#include "LedCheckBox.h"
#include "TempoSyncKnob.h"

namespace lmms::gui
{


FlangerControlsDialog::FlangerControlsDialog( FlangerControls *controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	auto mainLayout = new QVBoxLayout(this);
	auto knobLayout = new QHBoxLayout();
	mainLayout->addLayout(knobLayout);

	auto delayKnob = new Knob(KnobType::Bright26, tr("DELAY"), this);
	delayKnob->setVolumeKnob( false );
	delayKnob->setModel( &controls->m_delayTimeModel );
	delayKnob->setHintText( tr( "Delay time:" ) + " ", "s" );

	auto lfoFreqKnob = new TempoSyncKnob(KnobType::Bright26, tr("RATE"), this);
	lfoFreqKnob->setVolumeKnob( false );
	lfoFreqKnob->setModel( &controls->m_lfoFrequencyModel );
	lfoFreqKnob->setHintText( tr( "Period:" ) , " Sec" );

	auto lfoAmtKnob = new Knob(KnobType::Bright26, tr("AMNT"), this);
	lfoAmtKnob->setVolumeKnob( false );
	lfoAmtKnob->setModel( &controls->m_lfoAmountModel );
	lfoAmtKnob->setHintText( tr( "Amount:" ) , "" );

	auto lfoPhaseKnob = new Knob(KnobType::Bright26, tr("PHASE"), this);
	lfoPhaseKnob->setVolumeKnob( false );
	lfoPhaseKnob->setModel( &controls->m_lfoPhaseModel );
	lfoPhaseKnob->setHintText( tr( "Phase:" ) , " degrees" );

	auto feedbackKnob = new Knob(KnobType::Bright26, tr("FDBK"), this);
	feedbackKnob->setVolumeKnob( true) ;
	feedbackKnob->setModel( &controls->m_feedbackModel );
	feedbackKnob->setHintText( tr( "Feedback amount:" ) , "" );

	auto whiteNoiseKnob = new Knob(KnobType::Bright26, tr("NOISE"), this);
	whiteNoiseKnob->setVolumeKnob( true) ;
	whiteNoiseKnob->setModel( &controls->m_whiteNoiseAmountModel );
	whiteNoiseKnob->setHintText( tr( "White noise amount:" ) , "" );

	knobLayout->addWidget(delayKnob);
	knobLayout->addWidget(lfoFreqKnob);
	knobLayout->addWidget(lfoAmtKnob);
	knobLayout->addWidget(lfoPhaseKnob);
	knobLayout->addWidget(feedbackKnob);
	knobLayout->addWidget(whiteNoiseKnob);

	auto invertCb = new LedCheckBox(tr("Invert"), this);
	invertCb->setModel(&controls->m_invertFeedbackModel);
	
	mainLayout->addWidget(invertCb, 0, Qt::AlignLeft);
}


} // namespace lmms::gui
