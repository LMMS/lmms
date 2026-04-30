/*
 * BitcrushControlDialog.cpp - A native bitcrusher
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


#include <QLabel>

#include "embed.h"
#include "BitcrushControlDialog.h"
#include "BitcrushControls.h"
#include "FontHelper.h"
#include "LedCheckBox.h"
#include "Knob.h"

namespace lmms::gui
{


BitcrushControlDialog::BitcrushControlDialog( BitcrushControls * controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(),	PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 181, 128 );
	
	// labels
	const auto labelFont = adjustedToPixelSize(font(), DEFAULT_FONT_SIZE);

	auto inLabel = new QLabel(tr("IN"), this);
	inLabel->setFont(labelFont);
	inLabel->move( 24, 15 );

	auto outLabel = new QLabel(tr("OUT"), this);
	outLabel->setFont(labelFont);
	outLabel->move( 139, 15 );
	
	// input knobs
	auto inGain = new Knob(KnobType::Bright26, tr("GAIN"), SMALL_FONT_SIZE, this);
	inGain->move( 16, 32 );
	inGain->setModel( & controls->m_inGain );
	inGain->setHintText( tr( "Input gain:" ) , " dBFS" );

	auto inNoise = new Knob(KnobType::Bright26, tr("NOISE"), SMALL_FONT_SIZE, this);
	inNoise->move( 14, 76 );
	inNoise->setModel( & controls->m_inNoise );
	inNoise->setHintText( tr( "Input noise:" ) , "%" );
	
	
	// output knobs
	auto outGain = new Knob(KnobType::Bright26, tr("GAIN"), SMALL_FONT_SIZE, this);
	outGain->move( 138, 32 );
	outGain->setModel( & controls->m_outGain );
	outGain->setHintText( tr( "Output gain:" ) , " dBFS" );

	auto outClip = new Knob(KnobType::Bright26, tr("CLIP"), SMALL_FONT_SIZE, this);
	outClip->move( 138, 76 );
	outClip->setModel( & controls->m_outClip );
    outClip->setHintText( tr( "Output clip:" ) , " dBFS");

	
	
	// leds
	auto rateEnabled = new LedCheckBox("", this, tr("Rate enabled"), LedCheckBox::LedColor::Green);
	rateEnabled->move( 64, 14 );
	rateEnabled->setModel( & controls->m_rateEnabled );
	rateEnabled->setToolTip(tr("Enable sample-rate crushing"));

	auto depthEnabled = new LedCheckBox("", this, tr("Depth enabled"), LedCheckBox::LedColor::Green);
	depthEnabled->move( 101, 14 );
	depthEnabled->setModel( & controls->m_depthEnabled );
	depthEnabled->setToolTip(tr("Enable bit-depth crushing"));
	
	
	// rate crushing knobs
	auto rate = new Knob(KnobType::Bright26, tr("FREQ"), SMALL_FONT_SIZE, this);
	rate->move( 59, 32 );
	rate->setModel( & controls->m_rate );
	rate->setHintText( tr( "Sample rate:" ) , " Hz" );

	auto stereoDiff = new Knob(KnobType::Bright26, tr("STEREO"), SMALL_FONT_SIZE, this);
	stereoDiff->move( 72, 76 );
	stereoDiff->setModel( & controls->m_stereoDiff );
	stereoDiff->setHintText( tr( "Stereo difference:" ) , "%" );
	
	
	// depth crushing knob
	auto levels = new Knob(KnobType::Bright26, tr("QUANT"), SMALL_FONT_SIZE, this);
	levels->move( 92, 32 );
	levels->setModel( & controls->m_levels );
	levels->setHintText( tr( "Levels:" ) , "" );
}


} // namespace lmms::gui
