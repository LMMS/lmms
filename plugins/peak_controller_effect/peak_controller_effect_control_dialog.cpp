/*
 * peak_controller_effect_control_dialog.cpp - control dialog for
 *                                             PeakControllerEffect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
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



#include <QtGui/QLayout>
#include <QtGui/QLabel>

#include "peak_controller_effect_control_dialog.h"
#include "peak_controller_effect_controls.h"
#include "knob.h"
#include "led_checkbox.h"
#include "embed.h"


PeakControllerEffectControlDialog::PeakControllerEffectControlDialog(
				PeakControllerEffectControls * _controls ) :
	EffectControlDialog( _controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );

	QVBoxLayout * tl = new QVBoxLayout( this );
	tl->setContentsMargins( 5, 30, 8, 8 );

	QHBoxLayout * l = new QHBoxLayout;
	l->setSpacing( 4 );
	m_baseKnob = new knob( knobBright_26, this );
	m_baseKnob->setLabel( tr( "BASE" ) );
	m_baseKnob->setModel( &_controls->m_baseModel );
	m_baseKnob->setHintText( tr( "Base amount:" ) + " ", "" );

	m_amountKnob = new knob( knobBright_26, this );
	m_amountKnob->setLabel( tr( "AMNT" ) );
	m_amountKnob->setModel( &_controls->m_amountModel );
	m_amountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );

	m_amountMultKnob = new knob( knobBright_26, this );
	m_amountMultKnob->setLabel( tr( "MULT" ) );
	m_amountMultKnob->setModel( &_controls->m_amountMultModel );
	m_amountMultKnob->setHintText( tr( "Amount Multiplicator:" ) + " ", "" );

	m_attackKnob = new knob( knobBright_26, this );
	m_attackKnob->setLabel( tr( "ATCK" ) );
	m_attackKnob->setModel( &_controls->m_attackModel );
	m_attackKnob->setHintText( tr( "Attack:" ) + " ", "" );

	m_decayKnob = new knob( knobBright_26, this );
	m_decayKnob->setLabel( tr( "DCAY" ) );
	m_decayKnob->setModel( &_controls->m_decayModel );
	m_decayKnob->setHintText( tr( "Release:" ) + " ", "" );

	l->addWidget( m_baseKnob );
	l->addWidget( m_amountKnob );
	l->addWidget( m_amountMultKnob );
	l->addWidget( m_attackKnob );
	l->addWidget( m_decayKnob );
	l->addStretch(); // expand, so other widgets have minimum width
	tl->addLayout( l );

	QVBoxLayout * l2 = new QVBoxLayout; // = 2nd vbox

	m_muteLed = new ledCheckBox( "Mute Effect", this );
	m_muteLed->setModel( &_controls->m_muteModel );

	m_absLed = new ledCheckBox( "Absolute Value", this );
	m_absLed->setModel( &_controls->m_absModel );

	l2->addWidget( m_muteLed );
	l2->addWidget( m_absLed );
	l2->addStretch(); // expand, so other widgets have minimum height
	tl->addLayout( l2 );

	setLayout( tl );
}

#include "moc_peak_controller_effect_control_dialog.cxx"
