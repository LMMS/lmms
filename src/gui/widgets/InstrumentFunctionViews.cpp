/*
 * InstrumentFunctionViews.cpp - view for instrument-functions-tab
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
#include <QLayout>

#include "InstrumentFunctions.h"
#include "InstrumentFunctionViews.h"
#include "ComboBox.h"
#include "GroupBox.h"
#include "gui_templates.h"
#include "Knob.h"
#include "TempoSyncKnob.h"


InstrumentFunctionNoteStackingView::InstrumentFunctionNoteStackingView( InstrumentFunctionNoteStacking* cc, QWidget* parent ) :
	QWidget( parent ),
	ModelView( NULL, this ),
	m_cc( cc ),
	m_chordsGroupBox( new GroupBox( tr( "STACKING" ) ) ),
	m_chordsComboBox( new ComboBox() ),
	m_chordRangeKnob( new Knob( knobBright_26 ) )
{
	QHBoxLayout* topLayout = new QHBoxLayout( this );
	topLayout->setMargin( 0 );
	topLayout->addWidget( m_chordsGroupBox );

	QGridLayout* mainLayout = new QGridLayout( m_chordsGroupBox );
	mainLayout->setContentsMargins( 8, 18, 8, 8 );
	mainLayout->setColumnStretch( 0, 1 );
	mainLayout->setHorizontalSpacing( 20 );
	mainLayout->setVerticalSpacing( 1 );

	QLabel* chordLabel = new QLabel( tr( "Chord:" ) );
	chordLabel->setFont( pointSize<8>( chordLabel->font() ) );

	m_chordRangeKnob->setLabel( tr( "RANGE" ) );
	m_chordRangeKnob->setHintText( tr( "Chord range:" ), " " + tr( "octave(s)" ) );

	mainLayout->addWidget( chordLabel, 0, 0 );
	mainLayout->addWidget( m_chordsComboBox, 1, 0 );
	mainLayout->addWidget( m_chordRangeKnob, 0, 1, 2, 1, Qt::AlignHCenter );
}




InstrumentFunctionNoteStackingView::~InstrumentFunctionNoteStackingView()
{
	delete m_chordsGroupBox;
}




void InstrumentFunctionNoteStackingView::modelChanged()
{
	m_cc = castModel<InstrumentFunctionNoteStacking>();
	m_chordsGroupBox->setModel( &m_cc->m_chordsEnabledModel );
	m_chordsComboBox->setModel( &m_cc->m_chordsModel );
	m_chordRangeKnob->setModel( &m_cc->m_chordRangeModel );
}







InstrumentFunctionArpeggioView::InstrumentFunctionArpeggioView( InstrumentFunctionArpeggio* arp, QWidget* parent ) :
	QWidget( parent ),
	ModelView( NULL, this ),
	m_a( arp ),
	m_arpGroupBox( new GroupBox( tr( "ARPEGGIO" ) ) ),
	m_arpComboBox( new ComboBox() ),
	m_arpRangeKnob( new Knob( knobBright_26 ) ),
	m_arpRepeatsKnob( new Knob( knobBright_26 ) ),
	m_arpCycleKnob( new Knob( knobBright_26 ) ),
	m_arpSkipKnob( new Knob( knobBright_26 ) ),
	m_arpMissKnob( new Knob( knobBright_26 ) ),
	m_arpTimeKnob( new TempoSyncKnob( knobBright_26 ) ),
	m_arpGateKnob( new Knob( knobBright_26 ) ),
	m_arpDirectionComboBox( new ComboBox() ),
	m_arpModeComboBox( new ComboBox() )
{
	QHBoxLayout* topLayout = new QHBoxLayout( this );
	topLayout->setMargin( 0 );
	topLayout->addWidget( m_arpGroupBox );

	QGridLayout* mainLayout = new QGridLayout( m_arpGroupBox );
	mainLayout->setContentsMargins( 8, 18, 8, 8 );
	mainLayout->setColumnStretch( 0, 1 );
	mainLayout->setHorizontalSpacing( 20 );
	mainLayout->setVerticalSpacing( 1 );

	m_arpRangeKnob->setLabel( tr( "RANGE" ) );
	m_arpRangeKnob->setHintText( tr( "Arpeggio range:" ), " " + tr( "octave(s)" ) );


	m_arpRepeatsKnob->setLabel( tr( "REP" ) );
	m_arpRepeatsKnob->setHintText( tr( "Note repeats:" ) + " ", " " + tr( "time(s)" ) );


	m_arpCycleKnob->setLabel( tr( "CYCLE" ) );
	m_arpCycleKnob->setHintText( tr( "Cycle notes:" ) + " ", " " + tr( "note(s)" ) );


	m_arpSkipKnob->setLabel( tr( "SKIP" ) );
	m_arpSkipKnob->setHintText( tr( "Skip rate:" ), tr( "%" ) );


	m_arpMissKnob->setLabel( tr( "MISS" ) );
	m_arpMissKnob->setHintText( tr( "Miss rate:" ), tr( "%" ) );


	m_arpTimeKnob->setLabel( tr( "TIME" ) );
	m_arpTimeKnob->setHintText( tr( "Arpeggio time:" ), " " + tr( "ms" ) );


	m_arpGateKnob->setLabel( tr( "GATE" ) );
	m_arpGateKnob->setHintText( tr( "Arpeggio gate:" ), tr( "%" ) );


	QLabel* arpChordLabel = new QLabel( tr( "Chord:" ) );
	arpChordLabel->setFont( pointSize<8>( arpChordLabel->font() ) );

	QLabel* arpDirectionLabel = new QLabel( tr( "Direction:" ) );
	arpDirectionLabel->setFont( pointSize<8>( arpDirectionLabel->font() ) );

	QLabel* arpModeLabel = new QLabel( tr( "Mode:" ) );
	arpModeLabel->setFont( pointSize<8>( arpModeLabel->font() ) );

	mainLayout->addWidget( arpChordLabel, 0, 0 );
	mainLayout->addWidget( m_arpComboBox, 1, 0 );
	mainLayout->addWidget( arpDirectionLabel, 3, 0 );
	mainLayout->addWidget( m_arpDirectionComboBox, 4, 0 );
	mainLayout->addWidget( arpModeLabel, 6, 0 );
	mainLayout->addWidget( m_arpModeComboBox, 7, 0 );

	mainLayout->addWidget( m_arpRangeKnob, 0, 1, 2, 1, Qt::AlignHCenter );
	mainLayout->addWidget( m_arpRepeatsKnob, 0, 2, 2, 1, Qt::AlignHCenter );
	mainLayout->addWidget( m_arpCycleKnob, 0, 3, 2, 1, Qt::AlignHCenter );
	mainLayout->addWidget( m_arpSkipKnob, 3, 2, 2, 1, Qt::AlignHCenter );
	mainLayout->addWidget( m_arpMissKnob, 3, 3, 2, 1, Qt::AlignHCenter );
	mainLayout->addWidget( m_arpGateKnob, 6, 2, 2, 1, Qt::AlignHCenter );
	mainLayout->addWidget( m_arpTimeKnob, 6, 3, 2, 1, Qt::AlignHCenter );

	mainLayout->setRowMinimumHeight( 2, 10 );
	mainLayout->setRowMinimumHeight( 5, 10 );
}




InstrumentFunctionArpeggioView::~InstrumentFunctionArpeggioView()
{
	delete m_arpGroupBox;
}




void InstrumentFunctionArpeggioView::modelChanged()
{
	m_a = castModel<InstrumentFunctionArpeggio>();
	m_arpGroupBox->setModel( &m_a->m_arpEnabledModel );
	m_arpComboBox->setModel( &m_a->m_arpModel );
	m_arpRangeKnob->setModel( &m_a->m_arpRangeModel );
	m_arpRepeatsKnob->setModel( &m_a->m_arpRepeatsModel );
	m_arpCycleKnob->setModel( &m_a->m_arpCycleModel );
	m_arpSkipKnob->setModel( &m_a->m_arpSkipModel );
	m_arpMissKnob->setModel( &m_a->m_arpMissModel );
	m_arpTimeKnob->setModel( &m_a->m_arpTimeModel );
	m_arpGateKnob->setModel( &m_a->m_arpGateModel );
	m_arpDirectionComboBox->setModel( &m_a->m_arpDirectionModel );
	m_arpModeComboBox->setModel( &m_a->m_arpModeModel );
}





