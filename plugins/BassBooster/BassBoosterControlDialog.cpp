/*
 * BassBoosterControlDialog.cpp - control dialog for bassbooster effect
 *
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


#include <QHBoxLayout>
#include <QVBoxLayout>

#include "BassBoosterControlDialog.h"
#include "BassBoosterControls.h"
#include "embed.h"
#include "Knob.h"


namespace lmms::gui
{


BassBoosterControlDialog::BassBoosterControlDialog( BassBoosterControls* controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 120, 60 );

	auto tl = new QVBoxLayout(this);
	tl->addSpacing( 4 );

	auto l = new QHBoxLayout;

	auto freqKnob = new Knob(knobBright_26, this);
	freqKnob->setModel( &controls->m_freqModel );
	freqKnob->setLabel( tr( "FREQ" ) );
	freqKnob->setHintText( tr( "Frequency:" ) , "Hz" );

	auto gainKnob = new Knob(knobBright_26, this);
	gainKnob->setModel( &controls->m_gainModel );
	gainKnob->setLabel( tr( "GAIN" ) );
	gainKnob->setHintText( tr( "Gain:" ) , "" );

	auto ratioKnob = new Knob(knobBright_26, this);
	ratioKnob->setModel( &controls->m_ratioModel );
	ratioKnob->setLabel( tr( "RATIO" ) );
	ratioKnob->setHintText( tr( "Ratio:" ) , "" );

	l->addWidget( freqKnob );
	l->addWidget( gainKnob );
	l->addWidget( ratioKnob );

	tl->addLayout( l );
	setLayout( tl );
}


} // namespace lmms::gui
