/*
 * instrument_sound_shaping_view.cpp - view for instrumentSoundShaping-class
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "instrument_sound_shaping_view.h"
#include "envelope_and_lfo_parameters.h"
#include "envelope_and_lfo_view.h"
#include "combobox.h"
#include "group_box.h"
#include "gui_templates.h"
#include "knob.h"
#include "tab_widget.h"



const int TARGETS_TABWIDGET_X = 4;
const int TARGETS_TABWIDGET_Y = 5;
const int TARGETS_TABWIDGET_WIDTH = 238;
const int TARGETS_TABWIDGET_HEIGTH = 175;

const int FILTER_GROUPBOX_X = TARGETS_TABWIDGET_X;
const int FILTER_GROUPBOX_Y = TARGETS_TABWIDGET_Y+TARGETS_TABWIDGET_HEIGTH+5;
const int FILTER_GROUPBOX_WIDTH = TARGETS_TABWIDGET_WIDTH;
const int FILTER_GROUPBOX_HEIGHT = 245-FILTER_GROUPBOX_Y;



instrumentSoundShapingView::instrumentSoundShapingView( QWidget * _parent ) :
	QWidget( _parent ),
	modelView( NULL ),
	m_ss( NULL )
{
	m_targetsTabWidget = new tabWidget( tr( "TARGET" ), this );
	m_targetsTabWidget->setGeometry( TARGETS_TABWIDGET_X,
						TARGETS_TABWIDGET_Y,
						TARGETS_TABWIDGET_WIDTH,
						TARGETS_TABWIDGET_HEIGTH );
	m_targetsTabWidget->setWhatsThis(
		tr( "These tabs contain envelopes. They're very important for "
			"modifying a sound, for not saying that they're almost "
			"always neccessary for substractive synthesis. For "
			"example if you have a volume-envelope, you can set "
			"when the sound should have which volume-level. "
			"Maybe you want to create some soft strings. Then your "
			"sound has to fade in and out very softly. This can be "
			"done by setting a large attack- and release-time. "
			"It's the same for other envelope-targets like "
			"panning, cutoff-frequency of used filter and so on. "
			"Just monkey around with it! You can really make cool "
			"sounds out of a saw-wave with just some "
			"envelopes...!" ) );

	for( int i = 0; i < instrumentSoundShaping::NumTargets; ++i )
	{
		m_envLFOViews[i] = new envelopeAndLFOView( m_targetsTabWidget );
		m_targetsTabWidget->addTab( m_envLFOViews[i],
						tr( __targetNames[i][0]
						.toAscii().constData() ) );
	}


	m_filterGroupBox = new groupBox( tr( "FILTER" ), this );
	m_filterGroupBox->setGeometry( FILTER_GROUPBOX_X, FILTER_GROUPBOX_Y,
						FILTER_GROUPBOX_WIDTH,
						FILTER_GROUPBOX_HEIGHT );


	m_filterComboBox = new comboBox( m_filterGroupBox,
							tr( "Filter type" ) );
	m_filterComboBox->setGeometry( 14, 22, 120, 22 );
	m_filterComboBox->setFont( pointSize<8>( m_filterComboBox->font() ) );

	m_filterComboBox->setWhatsThis(
		tr( "Here you can select the built-in filter you want to use "
			"for this instrument-track. Filters are very important "
			"for changing the characteristics of a sound." ) );


	m_filterCutKnob = new knob( knobBright_26, m_filterGroupBox,
						tr( "cutoff-frequency" ) );
	m_filterCutKnob->setLabel( tr( "CUTOFF" ) );
	m_filterCutKnob->move( 140, 18 );
	m_filterCutKnob->setHintText( tr( "cutoff-frequency:" ) + " ", " " +
								tr( "Hz" ) );
	m_filterCutKnob->setWhatsThis(
		tr( "Use this knob for setting the cutoff-frequency for the "
			"selected filter. The cutoff-frequency specifies the "
			"frequency for cutting the signal by a filter. For "
			"example a lowpass-filter cuts all frequencies above "
			"the cutoff-frequency. A highpass-filter cuts all "
			"frequencies below cutoff-frequency and so on..." ) );


	m_filterResKnob = new knob( knobBright_26, m_filterGroupBox,
							tr( "Q/Resonance" ) );
	m_filterResKnob->setLabel( tr( "Q/RESO" ) );
	m_filterResKnob->move( 190, 18 );
	m_filterResKnob->setHintText( tr( "Q/Resonance:" ) + " ", "" );
	m_filterResKnob->setWhatsThis(
		tr( "Use this knob for setting Q/Resonance for the selected "
			"filter. Q/Resonance tells the filter, how much it "
			"should amplify frequencies near Cutoff-frequency." ) );
}




instrumentSoundShapingView::~instrumentSoundShapingView()
{
	delete m_targetsTabWidget;
}




void instrumentSoundShapingView::modelChanged( void )
{
	m_ss = castModel<instrumentSoundShaping>();
	m_filterGroupBox->setModel( &m_ss->m_filterEnabledModel );
	m_filterComboBox->setModel( &m_ss->m_filterModel );
	m_filterCutKnob->setModel( &m_ss->m_filterCutModel );
	m_filterResKnob->setModel( &m_ss->m_filterResModel );
	for( int i = 0; i < instrumentSoundShaping::NumTargets; ++i )
	{
		m_envLFOViews[i]->setModel( m_ss->m_envLFOParameters[i] );
	}
}



#include "instrument_sound_shaping_view.moc"

