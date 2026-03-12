/*
 * DualFilterControlDialog.cpp - control dialog for dual filter effect
 *
 * Copyright (c) 2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "DualFilterControlDialog.h"
#include "DualFilterControls.h"
#include "FontHelper.h"
#include "Knob.h"
#include "LedCheckBox.h"
#include "ComboBox.h"

namespace lmms::gui
{


#define makeknob( name, x, y, model, label, hint, unit ) 	\
	Knob * name = new Knob(KnobType::Bright26, label, SMALL_FONT_SIZE, this); 			\
	(name) -> move( x, y );									\
	(name) ->setModel( &controls-> model );					\
	(name) ->setHintText( hint, unit );



DualFilterControlDialog::DualFilterControlDialog( DualFilterControls* controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 373, 109 );

	makeknob( cut1Knob, 24, 26, m_cut1Model, tr( "FREQ" ), tr( "Cutoff frequency" ), "Hz" )
	makeknob( res1Knob, 74, 26, m_res1Model, tr( "RESO" ), tr( "Resonance" ), "" )
	makeknob( gain1Knob, 124, 26, m_gain1Model, tr( "GAIN" ), tr( "Gain" ), "%" )
	makeknob( mixKnob, 173, 37, m_mixModel, tr( "MIX" ), tr( "Mix" ), "" )
	makeknob( cut2Knob, 222, 26, m_cut2Model, tr( "FREQ" ), tr( "Cutoff frequency" ), "Hz" )
	makeknob( res2Knob, 272, 26, m_res2Model, tr( "RESO" ), tr( "Resonance" ), "" )
	makeknob( gain2Knob, 322, 26, m_gain2Model, tr( "GAIN" ), tr( "Gain" ), "%" )

	gain1Knob-> setVolumeKnob( true );
	gain2Knob-> setVolumeKnob( true );

	auto enabled1Toggle = new LedCheckBox("", this, tr("Filter 1 enabled"), LedCheckBox::LedColor::Green);
	auto enabled2Toggle = new LedCheckBox("", this, tr("Filter 2 enabled"), LedCheckBox::LedColor::Green);

	enabled1Toggle -> move( 12, 11 );
	enabled1Toggle -> setModel( &controls -> m_enabled1Model );
	enabled1Toggle->setToolTip(tr("Enable/disable filter 1"));
	enabled2Toggle -> move( 210, 11 );
	enabled2Toggle -> setModel( &controls -> m_enabled2Model );
	enabled2Toggle->setToolTip(tr("Enable/disable filter 2"));

	auto m_filter1ComboBox = new ComboBox(this);
	m_filter1ComboBox->setGeometry( 19, 70, 137, ComboBox::DEFAULT_HEIGHT );
	m_filter1ComboBox->setModel( &controls->m_filter1Model );

	auto m_filter2ComboBox = new ComboBox(this);
	m_filter2ComboBox->setGeometry( 217, 70, 137, ComboBox::DEFAULT_HEIGHT );
	m_filter2ComboBox->setModel( &controls->m_filter2Model );
}


} // namespace lmms::gui
