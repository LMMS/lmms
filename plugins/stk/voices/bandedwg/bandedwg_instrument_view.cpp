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

#include "bandedwg_instrument_view.h"

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "engine.h"
#include "gui_templates.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


bandedWGInstrumentView::bandedWGInstrumentView( bandedWGInstrument * _instrument, QWidget * _parent ) :
	stkInstrumentView<bandedWGInstrument>( _instrument, _parent )
{
	QVBoxLayout * vl = new QVBoxLayout( m_topView );
	QHBoxLayout * h1 = new QHBoxLayout;
	QHBoxLayout * h2 = new QHBoxLayout;
	QHBoxLayout * h3 = new QHBoxLayout;
	
	m_bowPressure = new knob( knobSmall_17, m_topView, tr( "Pressure" ) );
	m_bowPressure->setLabel( tr( "Bow Pressure" ) );
	m_bowPressure->setHintText( tr( "Pressure:" ) + " ", "" );

	m_bowPosition = new knob( knobSmall_17, m_topView, tr( "Position" ) );
	m_bowPosition->setLabel( tr( "Bow Position" ) );
	m_bowPosition->setHintText( tr( "Position:" ) + " ", "" );

	m_vibratoFrequency = new knob( knobSmall_17, m_topView, tr( "Vib Freq" ) );
	m_vibratoFrequency->setLabel( tr( "Vibrato Frequency" ) );
	m_vibratoFrequency->setHintText( tr( "Vib Freq:" ) + " ", "" );

	m_vibratoGain = new knob( knobSmall_17, m_topView, tr( "Vib Gain" ) );
	m_vibratoGain->setLabel( tr( "Vibrato Gain" ) );
	m_vibratoGain->setHintText( tr( "Vib Gain:" ) + " ", "" );

	m_bowVelocity = new knob( knobSmall_17, m_topView, tr( "Bow Vel" ) );
	m_bowVelocity->setLabel( tr( "Bow Velocity" ) );
	m_bowVelocity->setHintText( tr( "Velocity" ) + " ", "" );
	
	m_setStrike = new knob( knobSmall_17, m_topView, tr( "Strike" ) );
	m_setStrike->setLabel( tr( "Set Strike" ) );
	m_setStrike->setHintText( tr( "Strike" ) + " ", "" );
	
	m_sound = new knob( knobSmall_17, m_topView, tr( "Instrument" ) );
	m_sound->setLabel( tr( "Instrument" ) );
	m_sound->setHintText( tr( "Instrument" ) + " ", "" );
	
	h1->addWidget( m_bowPressure );
	h1->addWidget( m_bowPosition );
	
	h2->addWidget( m_vibratoFrequency );
	h2->addWidget( m_vibratoGain );
	
	h3->addWidget( m_bowVelocity );
	h3->addWidget( m_setStrike );
	h3->addWidget( m_sound );
	
	vl->addLayout( h1 );
	vl->addLayout( h2 );
	vl->addLayout( h3 );
	
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
}




bandedWGInstrumentView::~bandedWGInstrumentView()
{
}




void bandedWGInstrumentView::modelChanged( void )
{
	stkInstrumentView<bandedWGInstrument>::modelChanged();
	
	bandedWGInstrument * inst = castModel<bandedWGInstrument>();
	m_bowPressure->setModel( inst->model()->bowPressure() );
	m_bowPosition->setModel( inst->model()->bowPosition() );
	m_vibratoFrequency->setModel( inst->model()->vibratoFrequency() );
	m_vibratoGain->setModel( inst->model()->vibratoGain() );
	m_bowVelocity->setModel( inst->model()->bowVelocity() );
	m_setStrike->setModel( inst->model()->setStrike() );
	m_sound->setModel( inst->model()->sound() );
}



