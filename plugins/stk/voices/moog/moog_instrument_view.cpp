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

#include "moog_instrument_view.h"

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "engine.h"
#include "gui_templates.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


moogInstrumentView::moogInstrumentView( moogInstrument * _instrument, QWidget * _parent ) :
	stkInstrumentView<moogInstrument>( _instrument, _parent )
{
	QVBoxLayout * vl = new QVBoxLayout( m_topView );
	QHBoxLayout * h1 = new QHBoxLayout;
	QHBoxLayout * h2 = new QHBoxLayout;
	
	m_filterQ = new knob( knobSmall_17, m_topView, tr( "Filter Q" ) );
	m_filterQ->setLabel( tr( "Filter Q" ) );
	m_filterQ->setHintText( tr( "Q:" ) + " ", "" );

	m_filterSweepRate = new knob( knobSmall_17, m_topView, tr( "Sweep Rate" ) );
	m_filterSweepRate->setLabel( tr( "Sweep Rate" ) );
	m_filterSweepRate->setHintText( tr( "Rate:" ) + " ", "" );

	m_vibratoFrequency = new knob( knobSmall_17, m_topView, tr( "Vib Freq" ) );
	m_vibratoFrequency->setLabel( tr( "Vibrato Frequency" ) );
	m_vibratoFrequency->setHintText( tr( "Vib Freq:" ) + " ", "" );

	m_vibratoGain = new knob( knobSmall_17, m_topView, tr( "Vib Gain" ) );
	m_vibratoGain->setLabel( tr( "Vibrato Gain" ) );
	m_vibratoGain->setHintText( tr( "Vib Gain:" ) + " ", "" );

	h1->addWidget( m_filterQ );
	h1->addWidget( m_filterSweepRate );
	
	h2->addWidget( m_vibratoFrequency );
	h2->addWidget( m_vibratoGain );
	
	vl->addLayout( h1 );
	vl->addLayout( h2 );
	
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
}




moogInstrumentView::~moogInstrumentView()
{
}




void moogInstrumentView::modelChanged( void )
{
	stkInstrumentView<moogInstrument>::modelChanged();
	
	moogInstrument * inst = castModel<moogInstrument>();
	m_filterQ->setModel( inst->model()->filterQ() );
	m_filterSweepRate->setModel( inst->model()->filterSweepRate() );
	m_vibratoFrequency->setModel( inst->model()->vibratoFrequency() );
	m_vibratoGain->setModel( inst->model()->vibratoGain() );
}



