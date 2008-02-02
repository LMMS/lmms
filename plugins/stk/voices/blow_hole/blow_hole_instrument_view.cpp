/*
 *
 * Copyright (c) 2008 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#include <QtCore/QDir>
#include <QtGui/QLayout>
#include <QtGui/QMessageBox>

#include "blow_hole_instrument_view.h"

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "engine.h"
#include "gui_templates.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


blowHoleInstrumentView::blowHoleInstrumentView( blowHoleInstrument * _instrument, QWidget * _parent ) :
	stkInstrumentView<blowHoleInstrument>( _instrument, _parent )
{
	QVBoxLayout * vl = new QVBoxLayout( m_topView );
	QHBoxLayout * h1 = new QHBoxLayout();
	QHBoxLayout * h2 = new QHBoxLayout();
	QHBoxLayout * h3 = new QHBoxLayout();
	

	m_reedStiffness = new knob( knobSmall_17, m_topView, tr( "Stiffness" ) );
	m_reedStiffness->setLabel( tr( "Reed Stiffness" ) );
	m_reedStiffness->setHintText( tr( "Stiffness:" ) + " ", "" );

	m_noiseGain = new knob( knobSmall_17, m_topView, tr( "Noise" ) );
	m_noiseGain->setLabel( tr( "Noise Gain" ) );
	m_noiseGain->setHintText( tr( "Noise:" ) + " ", "" );

	m_toneholeState = new knob( knobSmall_17, m_topView, tr( "Tone Hole" ) );
	m_toneholeState->setLabel( tr( "Tone Hole" ) );
	m_toneholeState->setHintText( tr( "Tone Hole:" ) + " ", "" );

	m_registerState = new knob( knobSmall_17, m_topView, tr( "Register" ) );
	m_registerState->setLabel( tr( "Register" ) );
	m_registerState->setHintText( tr( "Register:" ) + " ", "" );

	m_breathPressure = new knob( knobSmall_17, m_topView, tr( "Breath Pres" ) );
	m_breathPressure->setLabel( tr( "Breath Pressure" ) );
	m_breathPressure->setHintText( tr( "Pressure:" ) + " ", "" );

	h1->addWidget( m_reedStiffness );
	h1->addWidget( m_noiseGain );
	
	h2->addWidget( m_toneholeState );
	h2->addWidget( m_registerState );
	
	h3->addWidget( m_breathPressure );
	
	vl->addLayout( h1 );
	vl->addLayout( h2 );
	vl->addLayout( h3 );
	
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
}




blowHoleInstrumentView::~blowHoleInstrumentView()
{
}




void blowHoleInstrumentView::modelChanged( void )
{
	stkInstrumentView<blowHoleInstrument>::modelChanged();
	
	blowHoleInstrument * inst = castModel<blowHoleInstrument>();
	m_reedStiffness->setModel( inst->model()->reedStiffness() );
	m_noiseGain->setModel( inst->model()->noiseGain() );
	m_toneholeState->setModel( inst->model()->toneholeState() );
	m_registerState->setModel( inst->model()->registerState() );
	m_breathPressure->setModel( inst->model()->breathPressure() );
}



