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

#include "metal_instrument_view.h"

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "engine.h"
#include "gui_templates.h"

#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"


metalInstrumentView::metalInstrumentView( metalInstrument * _instrument, QWidget * _parent ) :
	stkInstrumentView<metalInstrument>( _instrument, _parent )
{
	QVBoxLayout * vl = new QVBoxLayout( m_topView );
	QHBoxLayout * h1 = new QHBoxLayout();
	QHBoxLayout * h2 = new QHBoxLayout();
	QHBoxLayout * h3 = new QHBoxLayout();
	

	m_index = new knob( knobSmall_17, m_topView, tr( "Index" ) );
	m_index->setLabel( tr( "Modulator Index" ) );
	m_index->setHintText( tr( "Index:" ) + " ", "" );

	m_crossfade = new knob( knobSmall_17, m_topView, tr( "Crossfade" ) );
	m_crossfade->setLabel( tr( "Modulator Crossfade" ) );
	m_crossfade->setHintText( tr( "Crossfade:" ) + " ", "" );

	m_lfoSpeed = new knob( knobSmall_17, m_topView, tr( "LFO Speed" ) );
	m_lfoSpeed->setLabel( tr( "LFO Speed" ) );
	m_lfoSpeed->setHintText( tr( "LFO Speed:" ) + " ", "" );

	m_lfoDepth = new knob( knobSmall_17, m_topView, tr( "LFO Depth" ) );
	m_lfoDepth->setLabel( tr( "LFO Depth" ) );
	m_lfoDepth->setHintText( tr( "LFO Depth:" ) + " ", "" );

	m_adsrTarget = new knob( knobSmall_17, m_topView, tr( "ADSR Target" ) );
	m_adsrTarget->setLabel( tr( "ADSR Target" ) );
	m_adsrTarget->setHintText( tr( "ADSR Target:" ) + " ", "" );

	h1->addWidget( m_index );
	h1->addWidget( m_crossfade );
	
	h2->addWidget( m_lfoSpeed );
	h2->addWidget( m_lfoDepth );
	
	h3->addWidget( m_adsrTarget );
	
	vl->addLayout( h1 );
	vl->addLayout( h2 );
	vl->addLayout( h3 );
	
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(), PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
}




metalInstrumentView::~metalInstrumentView()
{
}




void metalInstrumentView::modelChanged( void )
{
	stkInstrumentView<metalInstrument>::modelChanged();
	
	metalInstrument * inst = castModel<metalInstrument>();
	m_index->setModel( inst->model()->index() );
	m_crossfade->setModel( inst->model()->crossfade() );
	m_lfoSpeed->setModel( inst->model()->lfoSpeed() );
	m_lfoDepth->setModel( inst->model()->lfoDepth() );
	m_adsrTarget->setModel( inst->model()->adsrTarget() );
}



