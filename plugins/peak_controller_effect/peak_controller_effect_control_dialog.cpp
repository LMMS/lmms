/*
 * stereomatrix_control_dialog.cpp - control dialog for stereoMatrix-effect
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail/dot/com>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
#include "tempo_sync_knob.h"
#include "led_checkbox.h"

peakControllerEffectControlDialog::peakControllerEffectControlDialog(
	peakControllerEffectControls * _controls ) :
	effectControlDialog( _controls )
{

	setFixedSize( 120, 90 );

	const int knobY = 10;
	m_baseKnob = new knob( knobBright_26, this );
	m_baseKnob->setLabel( tr( "BASE" ) );
	m_baseKnob->move( 10, knobY );
	m_baseKnob->setModel( &_controls->m_baseModel );
	m_baseKnob->setHintText( tr( "Base amount:" ) + " ", "" );

	m_amountKnob = new knob( knobBright_26, this );
	m_amountKnob->setLabel( tr( "AMT" ) );
	m_amountKnob->move( 45, knobY );
	m_amountKnob->setModel( &_controls->m_amountModel );
	m_amountKnob->setHintText( tr( "Modulation amount:" ) + " ", "" );

	m_decayKnob = new tempoSyncKnob( knobBright_26, this );
	m_decayKnob->setLabel( tr( "DECAY" ) );
	m_decayKnob->move( 80, knobY );
	m_decayKnob->setModel( &_controls->m_decayModel );
	m_decayKnob->setHintText( tr( "Release decay (not implemented):" ) + " ", "" );

	m_muteLed = new ledCheckBox( "Mute", this );
	m_muteLed->move( 10, 60);
	m_muteLed->setModel( &_controls->m_muteModel );
}

