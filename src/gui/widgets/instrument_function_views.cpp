#ifndef SINGLE_SOURCE_COMPILE

/*
 * instrument_function_views.cpp - view for instrument-functions-tab
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


#include <QtGui/QLabel>

#include "instrument_functions.h"
#include "instrument_function_views.h"
#include "combobox.h"
#include "embed.h"
#include "engine.h"
#include "group_box.h"
#include "gui_templates.h"
#include "knob.h"
#include "pixmap_button.h"
#include "tempo_sync_knob.h"
#include "tooltip.h"



const int CHORDS_GROUPBOX_X = 4;
const int CHORDS_GROUPBOX_Y = 5;
const int CHORDS_GROUPBOX_WIDTH = 238;
const int CHORDS_GROUPBOX_HEIGHT = 65;
const int ARP_GROUPBOX_X = CHORDS_GROUPBOX_X;
const int ARP_GROUPBOX_Y = 10 + CHORDS_GROUPBOX_Y + CHORDS_GROUPBOX_HEIGHT;
const int ARP_GROUPBOX_WIDTH = CHORDS_GROUPBOX_WIDTH;
const int ARP_GROUPBOX_HEIGHT = 240 - ARP_GROUPBOX_Y;



chordCreatorView::chordCreatorView( chordCreator * _cc, QWidget * _parent ) :
	QWidget( _parent ),
	modelView( NULL, this ),
	m_cc( _cc ),
	m_chordsGroupBox( new groupBox( tr( "CHORDS" ), this ) ),
	m_chordsComboBox( new comboBox( m_chordsGroupBox ) ),
	m_chordRangeKnob( new knob( knobBright_26, m_chordsGroupBox ) )
{
	move( CHORDS_GROUPBOX_X, CHORDS_GROUPBOX_Y );
	setFixedSize( 250, CHORDS_GROUPBOX_HEIGHT );
	m_chordsGroupBox->setGeometry( 0, 0, CHORDS_GROUPBOX_WIDTH,
						CHORDS_GROUPBOX_HEIGHT );


	m_chordsComboBox->setGeometry( 10, 25, 140, 22 );


	m_chordRangeKnob->setLabel( tr( "RANGE" ) );
	m_chordRangeKnob->move( 164, 24 );
	m_chordRangeKnob->setHintText( tr( "Chord range:" ) + " ", " " +
							tr( "octave(s)" ) );
	m_chordRangeKnob->setWhatsThis(
		tr( "Use this knob for setting the chord range in octaves. "
			"The selected chord will be played within specified "
			"amount of octaves." ) );

}




chordCreatorView::~chordCreatorView()
{
	delete m_chordsGroupBox;
}




void chordCreatorView::modelChanged( void )
{
	m_cc = castModel<chordCreator>();
	m_chordsGroupBox->setModel( &m_cc->m_chordsEnabledModel );
	m_chordsComboBox->setModel( &m_cc->m_chordsModel );
	m_chordRangeKnob->setModel( &m_cc->m_chordRangeModel );
}







arpeggiatorView::arpeggiatorView( arpeggiator * _arp, QWidget * _parent ) :
	QWidget( _parent ),
	modelView( NULL, this ),
	m_a( _arp ),
	m_arpGroupBox( new groupBox( tr( "ARPEGGIO" ), this ) ),
	m_arpComboBox( new comboBox( m_arpGroupBox) ),
	m_arpRangeKnob( new knob( knobBright_26, m_arpGroupBox ) ),
	m_arpTimeKnob( new tempoSyncKnob( knobBright_26, m_arpGroupBox ) ),
	m_arpGateKnob( new knob( knobBright_26, m_arpGroupBox ) ),
	m_arpModeComboBox( new comboBox( m_arpGroupBox ) )
{
	move( ARP_GROUPBOX_X, ARP_GROUPBOX_Y );
	setFixedSize( 250, ARP_GROUPBOX_HEIGHT );
	m_arpGroupBox->setGeometry( 0, 0, ARP_GROUPBOX_WIDTH,
							ARP_GROUPBOX_HEIGHT );

	m_arpGroupBox->setWhatsThis(
		tr( "An arpeggio is a type of playing (especially plucked) "
			"instruments, which makes the music much livelier. "
			"The strings of such instruments (e.g. harps) are "
			"plucked like chords, the only difference is, that "
			"this is done in a sequential order, so the notes are "
			"not played at the same time. Typical arpeggios are "
			"major or minor triads. But there're a lot of other "
			"possible chords, you can select." ) );


	m_arpComboBox->setGeometry( 10, 25, 140, 22 );


	m_arpRangeKnob->setLabel( tr( "RANGE" ) );
	m_arpRangeKnob->move( 164, 24 );
	m_arpRangeKnob->setHintText( tr( "Arpeggio range:" ) + " ", " " +
							tr( "octave(s)" ) );
	m_arpRangeKnob->setWhatsThis(
		tr( "Use this knob for setting the arpeggio range in octaves. "
			"The selected arpeggio will be played within specified "
			"amount of octaves." ) );


	m_arpTimeKnob->setLabel( tr( "TIME" ) );
	m_arpTimeKnob->move( 164, 70 );
	m_arpTimeKnob->setHintText( tr( "Arpeggio time:" ) + " ", " " +
								tr( "ms" ) );
	m_arpTimeKnob->setWhatsThis(
		tr( "Use this knob for setting the arpeggio time in "
			"milliseconds. The arpeggio time specifies how long "
			"each arpeggio-tone should be played." ) );


	m_arpGateKnob->setLabel( tr( "GATE" ) );
	m_arpGateKnob->move( 204, 70 );
	m_arpGateKnob->setHintText( tr( "Arpeggio gate:" ) + " ", tr( "%" ) );
	m_arpGateKnob->setWhatsThis(
		tr( "Use this knob for setting the arpeggio gate. The "
			"arpeggio gate specifies the percent of a whole "
			"arpeggio-tone that should be played. With this you "
			"can make cool staccato-arpeggios." ) );

	m_arpDirectionLbl = new QLabel( tr( "Direction:" ), m_arpGroupBox );
	m_arpDirectionLbl->setGeometry( 10, 60, 64, 10 );
	m_arpDirectionLbl->setFont( pointSize<7>( m_arpDirectionLbl->font() ) );



	pixmapButton * arp_up_btn = new pixmapButton( m_arpGroupBox, NULL );
	arp_up_btn->move( 10, 74 );
	arp_up_btn->setActiveGraphic( embed::getIconPixmap( "arp_up_on" ) );
	arp_up_btn->setInactiveGraphic( embed::getIconPixmap( "arp_up_off" ) );
	toolTip::add( arp_up_btn, tr( "arpeggio direction = up" ) );


	pixmapButton * arp_down_btn = new pixmapButton( m_arpGroupBox, NULL );
	arp_down_btn->move( 30, 74 );
	arp_down_btn->setActiveGraphic( embed::getIconPixmap( "arp_down_on" ) );
	arp_down_btn->setInactiveGraphic( embed::getIconPixmap(
							"arp_down_off" ) );
	toolTip::add( arp_down_btn, tr( "arpeggio direction = down" ) );


	pixmapButton * arp_up_and_down_btn = new pixmapButton( m_arpGroupBox,
									NULL );
	arp_up_and_down_btn->move( 50, 74 );
	arp_up_and_down_btn->setActiveGraphic( embed::getIconPixmap(
						"arp_up_and_down_on" ) );
	arp_up_and_down_btn->setInactiveGraphic( embed::getIconPixmap(
						"arp_up_and_down_off" ) );
	toolTip::add( arp_up_and_down_btn,
				tr( "arpeggio direction = up and down" ) );


	pixmapButton * arp_random_btn = new pixmapButton( m_arpGroupBox, NULL );
	arp_random_btn->move( 70, 74 );
	arp_random_btn->setActiveGraphic( embed::getIconPixmap(
							"arp_random_on" ) );
	arp_random_btn->setInactiveGraphic( embed::getIconPixmap(
							"arp_random_off" ) );
	toolTip::add( arp_random_btn, tr( "arpeggio direction = random" ) );

	m_arpDirectionBtnGrp = new automatableButtonGroup( this );
	m_arpDirectionBtnGrp->addButton( arp_up_btn );
	m_arpDirectionBtnGrp->addButton( arp_down_btn );
	m_arpDirectionBtnGrp->addButton( arp_up_and_down_btn );
	m_arpDirectionBtnGrp->addButton( arp_random_btn );



	QLabel * mode_lbl = new QLabel( tr( "Mode:" ), m_arpGroupBox );
	mode_lbl->setGeometry( 10, 104, 64, 10 );
	mode_lbl->setFont( pointSize<7>( mode_lbl->font() ) );

	m_arpModeComboBox->setGeometry( 10, 118, 128, 22 );
}




arpeggiatorView::~arpeggiatorView()
{
	delete m_arpDirectionBtnGrp;
	delete m_arpGroupBox;
}




void arpeggiatorView::modelChanged( void )
{
	m_a = castModel<arpeggiator>();
	m_arpGroupBox->setModel( &m_a->m_arpEnabledModel );
	m_arpComboBox->setModel( &m_a->m_arpModel );
	m_arpRangeKnob->setModel( &m_a->m_arpRangeModel );
	m_arpTimeKnob->setModel( &m_a->m_arpTimeModel );
	m_arpGateKnob->setModel( &m_a->m_arpGateModel );
	m_arpDirectionBtnGrp->setModel( &m_a->m_arpDirectionModel );
	m_arpModeComboBox->setModel( &m_a->m_arpModeModel );
}



#include "instrument_function_views.moc"

#endif
