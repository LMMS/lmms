/*
 * PeakControllerEffectControlDialog.cpp - control dialog for
 *                                             PeakControllerEffect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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




#include <QHBoxLayout>
#include <QVBoxLayout>

#include "PeakControllerEffectControlDialog.h"
#include "PeakControllerEffectControls.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "embed.h"

namespace lmms::gui
{

PeakControllerEffectControlDialog::PeakControllerEffectControlDialog(
				PeakControllerEffectControls * _controls ) :
	EffectControlDialog( _controls )
{
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 240, 80 );

	m_baseKnob = new Knob( KnobType::Bright26, this );
	m_baseKnob->setLabel( tr( "BASE" ) );
	m_baseKnob->setModel( &_controls->m_baseModel );
	m_baseKnob->setHintText( tr( "Base:" ) , "" );

	m_amountKnob = new Knob( KnobType::Bright26, this );
	m_amountKnob->setLabel( tr( "AMNT" ) );
	m_amountKnob->setModel( &_controls->m_amountModel );
	m_amountKnob->setHintText( tr( "Modulation amount:" ) , "" );

	m_amountMultKnob = new Knob( KnobType::Bright26, this );
	m_amountMultKnob->setLabel( tr( "MULT" ) );
	m_amountMultKnob->setModel( &_controls->m_amountMultModel );
	m_amountMultKnob->setHintText( tr( "Amount multiplicator:" ) , "" );

	m_attackKnob = new Knob( KnobType::Bright26, this );
	m_attackKnob->setLabel( tr( "ATCK" ) );
	m_attackKnob->setModel( &_controls->m_attackModel );
	m_attackKnob->setHintText( tr( "Attack:" ) , "" );

	m_decayKnob = new Knob( KnobType::Bright26, this );
	m_decayKnob->setLabel( tr( "DCAY" ) );
	m_decayKnob->setModel( &_controls->m_decayModel );
	m_decayKnob->setHintText( tr( "Release:" ) , "" );
	
	m_tresholdKnob = new Knob( KnobType::Bright26, this );
	m_tresholdKnob->setLabel( tr( "TRSH" ) );
	m_tresholdKnob->setModel( &_controls->m_tresholdModel );
	m_tresholdKnob->setHintText( tr( "Treshold:" ) , "" );

	m_muteLed = new LedCheckBox( tr( "Mute output" ), this );
	m_muteLed->setModel( &_controls->m_muteModel );

	m_absLed = new LedCheckBox( tr( "Absolute value" ), this );
	m_absLed->setModel( &_controls->m_absModel );

	auto mainLayout = new QVBoxLayout();
	auto knobLayout = new QHBoxLayout();
	auto ledLayout = new QHBoxLayout();

	knobLayout->addWidget( m_baseKnob );
	knobLayout->addWidget( m_amountKnob );
	knobLayout->addWidget( m_amountMultKnob );
	knobLayout->addWidget( m_attackKnob );
	knobLayout->addWidget( m_decayKnob );
	knobLayout->addWidget( m_tresholdKnob );

	ledLayout->addWidget( m_muteLed );
	ledLayout->addWidget( m_absLed );

	mainLayout->setContentsMargins( 3, 10, 0, 0 );
	mainLayout->addLayout( knobLayout );
	mainLayout->addLayout( ledLayout );

	setLayout( mainLayout );
}


} // namespace lmms::gui
