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

#include "flute_instrument_view.h"

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "engine.h"
#include "gui_templates.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


fluteInstrumentView::fluteInstrumentView( fluteInstrument * _instrument, QWidget * _parent ) :
	stkInstrumentView<fluteInstrument>( _instrument, _parent )
{
	QVBoxLayout * vl = new QVBoxLayout( m_topView );
	QHBoxLayout * h1 = new QHBoxLayout();
	QHBoxLayout * h2 = new QHBoxLayout();
	QHBoxLayout * h3 = new QHBoxLayout();
	

	m_jetDelay = new knob( knobSmall_17, m_topView, tr( "Jet Delay" ) );
	m_jetDelay->setLabel( tr( "Jet Delay" ) );
	m_jetDelay->setHintText( tr( "Delay:" ) + " ", "" );

	m_noiseGain = new knob( knobSmall_17, m_topView, tr( "Noise" ) );
	m_noiseGain->setLabel( tr( "Noise Gain" ) );
	m_noiseGain->setHintText( tr( "Noise:" ) + " ", "" );

	m_vibratoFrequency = new knob( knobSmall_17, m_topView, tr( "Vib Freq" ) );
	m_vibratoFrequency->setLabel( tr( "Vibrato Frequency" ) );
	m_vibratoFrequency->setHintText( tr( "Tone Hole:" ) + " ", "" );

	m_vibratoGain = new knob( knobSmall_17, m_topView, tr( "vib Gain" ) );
	m_vibratoGain->setLabel( tr( "Vibrato Gain" ) );
	m_vibratoGain->setHintText( tr( "Vib Gain:" ) + " ", "" );

	m_breathPressure = new knob( knobSmall_17, m_topView, tr( "Breath Pres" ) );
	m_breathPressure->setLabel( tr( "Breath Pressure" ) );
	m_breathPressure->setHintText( tr( "Pressure:" ) + " ", "" );

	h1->addWidget( m_jetDelay );
	h1->addWidget( m_noiseGain );
	
	h2->addWidget( m_vibratoFrequency );
	h2->addWidget( m_vibratoGain );
	
	h3->addWidget( m_breathPressure );
	
	vl->addLayout( h1 );
	vl->addLayout( h2 );
	vl->addLayout( h3 );
	
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
}




fluteInstrumentView::~fluteInstrumentView()
{
}




void fluteInstrumentView::modelChanged( void )
{
	stkInstrumentView<fluteInstrument>::modelChanged();
	
	fluteInstrument * inst = castModel<fluteInstrument>();
	m_jetDelay->setModel( inst->model()->jetDelay() );
	m_noiseGain->setModel( inst->model()->noiseGain() );
	m_vibratoFrequency->setModel( inst->model()->vibratoFrequency() );
	m_vibratoGain->setModel( inst->model()->vibratoGain() );
	m_breathPressure->setModel( inst->model()->breathPressure() );
}



