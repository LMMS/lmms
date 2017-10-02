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

#include <QGridLayout>
#include <QGroupBox>
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
	setFixedSize( 150, 400 );

	QGroupBox * attGB=new QGroupBox(tr ( "Attack" ), this);
	QGroupBox * desGB=new QGroupBox(tr ( "Descent" ), this);
	QGroupBox * panGB=new QGroupBox(tr ( "Pan" ), this);
	attGB->setGeometry(10,10,60,185);
	desGB->setGeometry(80,10,60,185);
	panGB->setGeometry(10,205,60,185);

	Knob * attTimeKnob = new Knob( knobBright_26, attGB);
	attTimeKnob -> move( 17, 35 );
	attTimeKnob->setModel( &controls->m_attackTimeModel );
	attTimeKnob->setLabel( tr( "Time" ) );
	attTimeKnob->setHintText( tr( "Attack Time:" ) , " beat" );

	Knob * desTimeKnob = new Knob( knobBright_26, desGB);
	desTimeKnob -> move( 17, 35 );
	desTimeKnob->setModel( &controls->m_descentTimeModel );
	desTimeKnob->setLabel( tr( "Time" ) );
	desTimeKnob->setHintText( tr( "Descent Time:" ) , " beat" );

	Knob * attTypeKnob = new Knob( knobBright_26, attGB);
	attTypeKnob -> move( 17, 85 );
	attTypeKnob->setModel( &controls->m_attackTypeModel );
	attTypeKnob->setLabel( tr( "Type" ) );
	attTypeKnob->setHintText( tr( "Attack Type:" ) , "" );

	Knob * desTypeKnob = new Knob( knobBright_26, desGB);
	desTypeKnob -> move( 17, 85 );
	desTypeKnob->setModel( &controls->m_descentTypeModel );
	desTypeKnob->setLabel( tr( "Type" ) );
	desTypeKnob->setHintText( tr( "Descent Type:" ) , "" );

	Knob * attTempoKnob = new Knob( knobBright_26, attGB);
	attTempoKnob -> move( 17, 135 );
	attTempoKnob->setModel( &controls->m_attackTempoModel );
	attTempoKnob->setLabel( tr( "Tempo" ) );
	attTempoKnob->setHintText( tr( "Attack Tempo:" ) , "" );

	Knob * desTempoKnob = new Knob( knobBright_26, desGB);
	desTempoKnob -> move( 17, 135 );
	desTempoKnob->setModel( &controls->m_descentTempoModel );
	desTempoKnob->setLabel( tr( "Tempo" ) );
	desTempoKnob->setHintText( tr( "Descent Tempo:" ) , "" );

	Knob * panTimeKnob = new Knob( knobBright_26, panGB);
	panTimeKnob -> move( 17, 35 );
	panTimeKnob->setModel( &controls->m_panTimeModel );
	panTimeKnob->setLabel( tr( "Time" ) );
	panTimeKnob->setHintText( tr( "Pan Time:" ) , "" );

	Knob * panTypeKnob = new Knob( knobBright_26, panGB);
	panTypeKnob -> move( 17, 85 );
	panTypeKnob->setModel( &controls->m_panTypeModel );
	panTypeKnob->setLabel( tr( "Type" ) );
	panTypeKnob->setHintText( tr( "Pan Type:" ) , "" );

	Knob * panTempoKnob = new Knob( knobBright_26, panGB);
	panTempoKnob -> move( 17, 135 );
	panTempoKnob->setModel( &controls->m_panTempoModel );
	panTempoKnob->setLabel( tr( "Tempo" ) );
	panTempoKnob->setHintText( tr( "Pan Tempo:" ) , "" );
}
