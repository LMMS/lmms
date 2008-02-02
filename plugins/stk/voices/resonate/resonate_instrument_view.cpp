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

#include "resonate_instrument_view.h"

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "engine.h"
#include "gui_templates.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


resonateInstrumentView::resonateInstrumentView( resonateInstrument * _instrument, QWidget * _parent ) :
	stkInstrumentView<resonateInstrument>( _instrument, _parent )
{
	QVBoxLayout * vl = new QVBoxLayout( m_topView );
	QHBoxLayout * h1 = new QHBoxLayout;
	QHBoxLayout * h2 = new QHBoxLayout;
	
	m_resonanceFrequency = new knob( knobSmall_17, m_topView, tr( "Resonance" ) );
	m_resonanceFrequency->setLabel( tr( "Resonance Frequency" ) );
	m_resonanceFrequency->setHintText( tr( "Frequency:" ) + " ", "" );

	m_poleRadii = new knob( knobSmall_17, m_topView, tr( "Pole Radii" ) );
	m_poleRadii->setLabel( tr( "Pole Radii" ) );
	m_poleRadii->setHintText( tr( "Radii:" ) + " ", "" );

	m_notchFrequency = new knob( knobSmall_17, m_topView, tr( "Notch" ) );
	m_notchFrequency->setLabel( tr( "Notch Frequency" ) );
	m_notchFrequency->setHintText( tr( "Frequency:" ) + " ", "" );

	m_zeroRadii = new knob( knobSmall_17, m_topView, tr( "Zero Radii" ) );
	m_zeroRadii->setLabel( tr( "Zero Radii" ) );
	m_zeroRadii->setHintText( tr( "Radii:" ) + " ", "" );

	h1->addWidget( m_resonanceFrequency );
	h1->addWidget( m_poleRadii );
	
	h2->addWidget( m_notchFrequency );
	h2->addWidget( m_zeroRadii );
	
	vl->addLayout( h1 );
	vl->addLayout( h2 );
	
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
}




resonateInstrumentView::~resonateInstrumentView()
{
}




void resonateInstrumentView::modelChanged( void )
{
	stkInstrumentView<resonateInstrument>::modelChanged();
	
	resonateInstrument * inst = castModel<resonateInstrument>();
	m_resonanceFrequency->setModel( inst->model()->resonanceFrequency() );
	m_poleRadii->setModel( inst->model()->poleRadii() );
	m_notchFrequency->setModel( inst->model()->notchFrequency() );
	m_zeroRadii->setModel( inst->model()->zeroRadii() );
}



