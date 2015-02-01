/*
 * DualFilterControlDialog.cpp - control dialog for dual filter effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "DualFilterControlDialog.h"
#include "DualFilterControls.h"
#include "embed.h"
#include "led_checkbox.h"
#include "combobox.h"
#include "tooltip.h"
#include "gui_templates.h"

#define makeknob( name, x, y, model, label, hint, unit ) 	\
	knob * name = new knob( knobBright_26, this); 			\
	name -> move( x, y );									\
	name ->setModel( &controls-> model );					\
	name ->setLabel( tr( label ) );							\
	name ->setHintText( tr( hint ) + " ", unit );



DualFilterControlDialog::DualFilterControlDialog( DualFilterControls* controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 150, 220 );

	makeknob( cut1Knob, 33, 30, m_cut1Model, "FREQ", "Cutoff frequency", "Hz" )
	makeknob( res1Knob, 75, 30, m_res1Model, "RESO", "Resonance", "" )
	makeknob( gain1Knob, 117, 30, m_gain1Model, "GAIN", "Gain", "%" )
	makeknob( mixKnob, 62, 100, m_mixModel, "MIX", "Mix", "" )
	makeknob( cut2Knob, 33, 145, m_cut2Model, "FREQ", "Cutoff frequency", "Hz" )
	makeknob( res2Knob, 75, 145, m_res2Model, "RESO", "Resonance", "" )
	makeknob( gain2Knob, 117, 145, m_gain2Model, "GAIN", "Gain", "%" )

	gain1Knob-> setVolumeKnob( true );
	gain2Knob-> setVolumeKnob( true );

	ledCheckBox * enabled1Toggle = new ledCheckBox( "", this,
				tr( "Filter 1 enabled" ), ledCheckBox::Green );
	ledCheckBox * enabled2Toggle = new ledCheckBox( "", this,
				tr( "Filter 2 enabled" ), ledCheckBox::Green );

	enabled1Toggle -> move( 5, 30 );
	enabled1Toggle -> setModel( &controls -> m_enabled1Model );
	toolTip::add( enabled1Toggle, tr( "Click to enable/disable Filter 1" ) );
	enabled2Toggle -> move( 5, 145 );
	enabled2Toggle -> setModel( &controls -> m_enabled2Model );
	toolTip::add( enabled2Toggle, tr( "Click to enable/disable Filter 2" ) );

	comboBox * m_filter1ComboBox = new comboBox( this );
	m_filter1ComboBox->setGeometry( 5, 70, 140, 22 );
	m_filter1ComboBox->setFont( pointSize<8>( m_filter1ComboBox->font() ) );
	m_filter1ComboBox->setModel( &controls->m_filter1Model );

	comboBox * m_filter2ComboBox = new comboBox( this );
	m_filter2ComboBox->setGeometry( 5, 185, 140, 22 );
	m_filter2ComboBox->setFont( pointSize<8>( m_filter2ComboBox->font() ) );
	m_filter2ComboBox->setModel( &controls->m_filter2Model );
}

#include "moc_DualFilterControlDialog.cxx"
