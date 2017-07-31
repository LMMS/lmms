/*
 * ClickGDXControlDialog.cpp - control dialog for click remover effect
 *
 * Copyright (c) 2017
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

#include <QLayout>

#include "ClickGDXControlDialog.h"
#include "ClickGDXControls.h"
#include "embed.h"



ClickGDXControlDialog::ClickGDXControlDialog( ClickGDXControls* controls ) :
	EffectControlDialog( controls )
{
	setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
	setFixedSize( 110, 170 );

	Knob * attTimeKnob = new Knob( knobBright_26, this);
	attTimeKnob -> move( 16, 10 );
	attTimeKnob->setModel( &controls->m_attackTimeModel );
	attTimeKnob->setLabel( tr( "A-Time" ) );
	attTimeKnob->setHintText( tr( "Attack Time:" ) , " beat" );

	Knob * desTimeKnob = new Knob( knobBright_26, this);
	desTimeKnob -> move( 57, 10 );
	desTimeKnob->setModel( &controls->m_descentTimeModel );
	desTimeKnob->setLabel( tr( "D-Time" ) );
	desTimeKnob->setHintText( tr( "Descent Time:" ) , " beat" );

	Knob * attTypeKnob = new Knob( knobBright_26, this);
	attTypeKnob -> move( 16, 65 );
	attTypeKnob->setModel( &controls->m_attackTypeModel );
	attTypeKnob->setLabel( tr( "A-Type" ) );
	attTypeKnob->setHintText( tr( "Attack Type:" ) , "" );

	Knob * desTypeKnob = new Knob( knobBright_26, this);
	desTypeKnob -> move( 57, 65 );
	desTypeKnob->setModel( &controls->m_descentTypeModel );
	desTypeKnob->setLabel( tr( "D-Type" ) );
	desTypeKnob->setHintText( tr( "Descent Type:" ) , "" );

	Knob * attTempoKnob = new Knob( knobBright_26, this);
	attTempoKnob -> move( 16, 120 );
	attTempoKnob->setModel( &controls->m_attackTempoModel );
	attTempoKnob->setLabel( tr( "A-Tempo" ) );
	attTempoKnob->setHintText( tr( "Attack Tempo:" ) , "" );

	Knob * desTempoKnob = new Knob( knobBright_26, this);
	desTempoKnob -> move( 57, 120 );
	desTempoKnob->setModel( &controls->m_descentTempoModel );
	desTempoKnob->setLabel( tr( "D-Tempo" ) );
	desTempoKnob->setHintText( tr( "Descent Tempo:" ) , "" );
}
