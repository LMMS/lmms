/*
 * InstrumentSoundShapingView.cpp - view for InstrumentSoundShaping class
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QLabel>

#include "InstrumentSoundShapingView.h"
#include "EnvelopeAndLfoParameters.h"
#include "EnvelopeAndLfoView.h"
#include "ComboBox.h"
#include "GroupBox.h"
#include "gui_templates.h"
#include "Knob.h"
#include "TabWidget.h"



const int TARGETS_TABWIDGET_X = 4;
const int TARGETS_TABWIDGET_Y = 5;
const int TARGETS_TABWIDGET_WIDTH = 242;
const int TARGETS_TABWIDGET_HEIGTH = 175;

const int FILTER_GROUPBOX_X = TARGETS_TABWIDGET_X;
const int FILTER_GROUPBOX_Y = TARGETS_TABWIDGET_Y+TARGETS_TABWIDGET_HEIGTH+5;
const int FILTER_GROUPBOX_WIDTH = TARGETS_TABWIDGET_WIDTH;
const int FILTER_GROUPBOX_HEIGHT = 245-FILTER_GROUPBOX_Y;



InstrumentSoundShapingView::InstrumentSoundShapingView( QWidget * _parent ) :
	QWidget( _parent ),
	ModelView( NULL, this ),
	m_ss( NULL )
{
	m_targetsTabWidget = new TabWidget( tr( "TARGET" ), this );
	m_targetsTabWidget->setGeometry( TARGETS_TABWIDGET_X,
						TARGETS_TABWIDGET_Y,
						TARGETS_TABWIDGET_WIDTH,
						TARGETS_TABWIDGET_HEIGTH );

	for( int i = 0; i < InstrumentSoundShaping::NumTargets; ++i )
	{
		m_envLfoViews[i] = new EnvelopeAndLfoView( m_targetsTabWidget );
		m_targetsTabWidget->addTab( m_envLfoViews[i],
						tr( InstrumentSoundShaping::targetNames[i][0].toUtf8().constData() ), 
                                                NULL );
	}


	m_filterGroupBox = new GroupBox( tr( "FILTER" ), this );
	m_filterGroupBox->setGeometry( FILTER_GROUPBOX_X, FILTER_GROUPBOX_Y,
						FILTER_GROUPBOX_WIDTH,
						FILTER_GROUPBOX_HEIGHT );


	m_filterComboBox = new ComboBox( m_filterGroupBox );
	m_filterComboBox->setGeometry( 14, 22, 120, 22 );
	m_filterComboBox->setFont( pointSize<8>( m_filterComboBox->font() ) );


	m_filterCutKnob = new Knob( knobBright_26, m_filterGroupBox );
	m_filterCutKnob->setLabel( tr( "FREQ" ) );
	m_filterCutKnob->move( 140, 18 );
	m_filterCutKnob->setHintText( tr( "Cutoff frequency:" ), " " + tr( "Hz" ) );


	m_filterResKnob = new Knob( knobBright_26, m_filterGroupBox );
	m_filterResKnob->setLabel( tr( "Q/RESO" ) );
	m_filterResKnob->move( 196, 18 );
	m_filterResKnob->setHintText( tr( "Q/Resonance:" ), "" );


	m_singleStreamInfoLabel = new QLabel( tr( "Envelopes, LFOs and filters are not supported by the current instrument." ), this );
	m_singleStreamInfoLabel->setWordWrap( true );
	m_singleStreamInfoLabel->setFont( pointSize<8>( m_singleStreamInfoLabel->font() ) );

	m_singleStreamInfoLabel->setGeometry( TARGETS_TABWIDGET_X,
						TARGETS_TABWIDGET_Y,
						TARGETS_TABWIDGET_WIDTH,
						TARGETS_TABWIDGET_HEIGTH );
}




InstrumentSoundShapingView::~InstrumentSoundShapingView()
{
	delete m_targetsTabWidget;
}



void InstrumentSoundShapingView::setFunctionsHidden( bool hidden )
{
	m_targetsTabWidget->setHidden( hidden );
	m_filterGroupBox->setHidden( hidden );
	m_singleStreamInfoLabel->setHidden( !hidden );
}



void InstrumentSoundShapingView::modelChanged()
{
	m_ss = castModel<InstrumentSoundShaping>();
	m_filterGroupBox->setModel( &m_ss->m_filterEnabledModel );
	m_filterComboBox->setModel( &m_ss->m_filterModel );
	m_filterCutKnob->setModel( &m_ss->m_filterCutModel );
	m_filterResKnob->setModel( &m_ss->m_filterResModel );
	for( int i = 0; i < InstrumentSoundShaping::NumTargets; ++i )
	{
		m_envLfoViews[i]->setModel( m_ss->m_envLfoParameters[i] );
	}
}




	
