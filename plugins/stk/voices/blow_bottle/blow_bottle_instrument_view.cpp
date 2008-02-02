/*
 * blow_bottle_instrument_view.cpp - gui interface to blown bottle noises
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

#include "blow_bottle_instrument_view.h"

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "engine.h"
#include "gui_templates.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


blowBottleInstrumentView::blowBottleInstrumentView( blowBottleInstrument * _instrument, QWidget * _parent ) :
	stkInstrumentView<blowBottleInstrument>( _instrument, _parent )
{
	QVBoxLayout * vl = new QVBoxLayout( m_topView );
	
	m_noiseGain = new knob( knobSmall_17, m_topView, tr( "Noise" ) );
	m_noiseGain->setLabel( tr( "Noise Gain" ) );
	m_noiseGain->setHintText( tr( "Noise:" ) + " ", "" );

	m_vibratoFrequency = new knob( knobSmall_17, m_topView, tr( "Vib Freq" ) );
	m_vibratoFrequency->setLabel( tr( "Vibrato Frequency" ) );
	m_vibratoFrequency->setHintText( tr( "Vib Freq:" ) + " ", "" );

	m_vibratoGain = new knob( knobSmall_17, m_topView, tr( "Vib Gain" ) );
	m_vibratoGain->setLabel( tr( "Vibrato Gain" ) );
	m_vibratoGain->setHintText( tr( "Vib Gain:" ) + " ", "" );

	vl->addWidget( m_noiseGain );
	vl->addWidget( m_vibratoFrequency );
	vl->addWidget( m_vibratoGain );
	
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
}




blowBottleInstrumentView::~blowBottleInstrumentView()
{
}




void blowBottleInstrumentView::modelChanged( void )
{
	stkInstrumentView<blowBottleInstrument>::modelChanged();
	
	blowBottleInstrument * inst = castModel<blowBottleInstrument>();
	m_noiseGain->setModel( inst->model()->noiseGain() );
	m_vibratoFrequency->setModel( inst->model()->vibratoFrequency() );
	m_vibratoGain->setModel( inst->model()->vibratoGain() );
}



