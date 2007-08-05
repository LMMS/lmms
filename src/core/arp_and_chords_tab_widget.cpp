#ifndef SINGLE_SOURCE_COMPILE

/*
 * arp_and_chords_tab_widget.cpp - widget for use in arp/chord-tab of 
 *                                 instrument-track-window
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtGui/QLabel>

#else

#include <qdom.h>
#include <qlabel.h>
#include <qwhatsthis.h>

#endif


#include "arp_and_chords_tab_widget.h"
#include "combobox.h"
#include "embed.h"
#include "engine.h"
#include "group_box.h"
#include "gui_templates.h"
#include "instrument_track.h"
#include "knob.h"
#include "led_checkbox.h"
#include "note_play_handle.h"
#include "song_editor.h"
#include "pixmap_button.h"
#include "preset_preview_play_handle.h"
#include "tempo_sync_knob.h"
#include "tooltip.h"



arpAndChordsTabWidget::chord arpAndChordsTabWidget::s_chords[] =
{
	{ arpAndChordsTabWidget::tr( "octave" ), { 0, -1 } },
	{ arpAndChordsTabWidget::tr( "Major" ), { 0, 4, 7, -1 } },
	{ arpAndChordsTabWidget::tr( "Majb5" ), { 0, 4, 6, -1 } },
	{ arpAndChordsTabWidget::tr( "minor" ), { 0, 3, 7, -1 } },
	{ arpAndChordsTabWidget::tr( "minb5" ), { 0, 3, 6, -1 } },
	{ arpAndChordsTabWidget::tr( "sus2" ), { 0, 2, 7, -1 } },
	{ arpAndChordsTabWidget::tr( "sus4" ), { 0, 5, 7, -1 } },
	{ arpAndChordsTabWidget::tr( "aug" ), { 0, 4, 8, -1 } },
	{ arpAndChordsTabWidget::tr( "augsus4" ), { 0, 5, 8, -1 } },
	{ arpAndChordsTabWidget::tr( "tri" ), { 0, 3, 6, 9, -1 } },
	
	{ arpAndChordsTabWidget::tr( "6" ), { 0, 4, 7, 9, -1 } },
	{ arpAndChordsTabWidget::tr( "6sus4" ), { 0, 5, 7, 9, -1 } },
	{ arpAndChordsTabWidget::tr( "6add9" ), { 0, 4, 7, 12, -1 } },
	{ arpAndChordsTabWidget::tr( "m6" ), { 0, 3, 7, 9, -1 } },
	{ arpAndChordsTabWidget::tr( "m6add9" ), { 0, 3, 7, 9, 14, -1 } },

	{ arpAndChordsTabWidget::tr( "7" ), { 0, 4, 7, 10, -1 } },
	{ arpAndChordsTabWidget::tr( "7sus4" ), { 0, 5, 7, 10, -1 } },
	{ arpAndChordsTabWidget::tr( "7#5" ), { 0, 4, 8, 10, -1 } },
	{ arpAndChordsTabWidget::tr( "7b5" ), { 0, 4, 6, 10, -1 } },
	{ arpAndChordsTabWidget::tr( "7#9" ), { 0, 4, 7, 10, 13, 18, -1 } },
	{ arpAndChordsTabWidget::tr( "7b9" ), { 0, 4, 7, 10, 13, 16, -1 } },
	{ arpAndChordsTabWidget::tr( "7#5#9" ), { 0, 4, 8, 12, 14, 19, -1 } },
	{ arpAndChordsTabWidget::tr( "7#5b9" ), { 0, 4, 8, 12, 14, 17, -1 } },
	{ arpAndChordsTabWidget::tr( "7b5b9" ), { 0, 4, 6, 10, 12, 15, -1 } },
	{ arpAndChordsTabWidget::tr( "7add11" ), { 0, 4, 7, 10, 17, -1 } },
	{ arpAndChordsTabWidget::tr( "7add13" ), { 0, 4, 7, 10, 21, -1 } },
	{ arpAndChordsTabWidget::tr( "7#11" ), { 0, 4, 7, 10, 18, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj7" ), { 0, 4, 7, 11, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj7b5" ), { 0, 4, 6, 11, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj7#5" ), { 0, 4, 8, 11, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj7#11" ), { 0, 4, 7, 11, 18, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj7add13" ), { 0, 4, 7, 11, 21, -1 } },
	{ arpAndChordsTabWidget::tr( "m7" ), { 0, 3, 7, 10, -1 } },
	{ arpAndChordsTabWidget::tr( "m7b5" ), { 0, 3, 6, 10, -1 } },
	{ arpAndChordsTabWidget::tr( "m7b9" ), { 0, 3, 7, 10, 13, -1 } },
	{ arpAndChordsTabWidget::tr( "m7add11" ), { 0, 3, 7, 10, 17, -1 } },
	{ arpAndChordsTabWidget::tr( "m7add13" ), { 0, 3, 7, 10, 21, -1 } },
	{ arpAndChordsTabWidget::tr( "m-Maj7" ), { 0, 3, 7, 11, -1 } },
	{ arpAndChordsTabWidget::tr( "m-Maj7add11" ), { 0, 3, 7, 11, 17, -1 } },
	{ arpAndChordsTabWidget::tr( "m-Maj7add13" ), { 0, 3, 7, 11, 21, -1 } },

	{ arpAndChordsTabWidget::tr( "9" ), { 0, 4, 7, 10, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "9sus4" ), { 0, 5, 7, 10, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "add9" ), { 0, 4, 7, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "9#5" ), { 0, 4, 8, 10, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "9b5" ), { 0, 4, 6, 10, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "9#11" ), { 0, 4, 7, 10, 14, 18, -1 } },
	{ arpAndChordsTabWidget::tr( "9b13" ), { 0, 4, 7, 10, 14, 20, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj9" ), { 0, 4, 7, 11, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj9sus4" ), { 0, 5, 7, 11, 15, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj9#5" ), { 0, 4, 8, 11, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj9#11" ), { 0, 4, 7, 11, 14, 18, -1 } },
	{ arpAndChordsTabWidget::tr( "m9" ), { 0, 3, 7, 10, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "madd9" ), { 0, 3, 7, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "m9b5" ), { 0, 3, 6, 10, 14, -1 } },
	{ arpAndChordsTabWidget::tr( "m9-Maj7" ), { 0, 3, 7, 11, 14, -1 } },

	{ arpAndChordsTabWidget::tr( "11" ), { 0, 4, 7, 10, 14, 17, -1 } },
	{ arpAndChordsTabWidget::tr( "11b9" ), { 0, 4, 7, 10, 13, 17, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj11" ), { 0, 4, 7, 11, 14, 17, -1 } },
	{ arpAndChordsTabWidget::tr( "m11" ), { 0, 3, 7, 10, 14, 17, -1 } },
	{ arpAndChordsTabWidget::tr( "m-Maj11" ), { 0, 3, 7, 11, 14, 17, -1 } },

	{ arpAndChordsTabWidget::tr( "13" ), { 0, 4, 7, 10, 14, 21, -1 } },
	{ arpAndChordsTabWidget::tr( "13#9" ), { 0, 4, 7, 10, 15, 21, -1 } },
	{ arpAndChordsTabWidget::tr( "13b9" ), { 0, 4, 7, 10, 13, 21, -1 } },
	{ arpAndChordsTabWidget::tr( "13b5b9" ), { 0, 4, 6, 10, 13, 21, -1 } },
	{ arpAndChordsTabWidget::tr( "Maj13" ), { 0, 4, 7, 11, 14, 21, -1 } },
	{ arpAndChordsTabWidget::tr( "m13" ), { 0, 3, 7, 10, 14, 21, -1 } },
	{ arpAndChordsTabWidget::tr( "m-Maj13" ), { 0, 3, 7, 11, 14, 21, -1 } },

	{ arpAndChordsTabWidget::tr( "Major" ), { 0, 2, 4, 5, 7, 9, 11, -1 } },
	{ arpAndChordsTabWidget::tr( "Harmonic minor" ), { 0, 2, 3, 5, 7, 8,
								11, -1 } },
	{ arpAndChordsTabWidget::tr( "Melodic minor" ), { 0, 2, 3, 5, 7, 9,
								11, -1 } },
	{ arpAndChordsTabWidget::tr( "Whole tone" ), { 0, 2, 4, 6, 8, 10,
								-1 } },
	{ arpAndChordsTabWidget::tr( "Diminished" ), { 0, 2, 3, 5, 6, 8, 9,
								11, -1 } },
	{ arpAndChordsTabWidget::tr( "Major pentatonic" ), { 0, 2, 4, 7, 10,
									-1 } },
	{ arpAndChordsTabWidget::tr( "Minor pentatonic" ), { 0, 3, 5, 7, 10,
									-1 } },
	{ arpAndChordsTabWidget::tr( "Jap in sen" ), { 0, 1, 5, 7, 10, -1 } },
	{ arpAndChordsTabWidget::tr( "Major bebop" ), { 0, 2, 4, 5, 7, 8, 9,
								11, -1 } },
	{ arpAndChordsTabWidget::tr( "Dominant bebop" ), { 0, 2, 4, 5, 7, 9,
								10, 11, -1 } },
	{ arpAndChordsTabWidget::tr( "Blues" ), { 0, 3, 5, 6, 7, 10, -1 } },
	{ arpAndChordsTabWidget::tr( "Arabic" ), { 0, 1, 4, 5, 7, 8, 11, -1 } },
	{ arpAndChordsTabWidget::tr( "Enigmatic" ), { 0, 1, 4, 6, 8, 10, 11,
									-1 } },
	{ arpAndChordsTabWidget::tr( "Neopolitan" ), { 0, 1, 3, 5, 7, 9, 11,
								-1 } },
	{ arpAndChordsTabWidget::tr( "Neopolitan minor" ), { 0, 1, 3, 5, 7, 9,
								11, -1 } },
	{ arpAndChordsTabWidget::tr( "Hungarian minor" ), { 0, 2, 3, 6, 7, 9,
								11, -1 } },
	{ arpAndChordsTabWidget::tr( "Dorian" ), { 0, 2, 3, 5, 7, 9, 10, -1 } },
	{ arpAndChordsTabWidget::tr( "Phrygolydian" ), { 0, 1, 3, 5, 7, 8, 10,
									-1 } },
	{ arpAndChordsTabWidget::tr( "Lydian" ), { 0, 2, 4, 6, 7, 9, 11, -1 } },
	{ arpAndChordsTabWidget::tr( "Mixolydian" ), { 0, 2, 4, 5, 7, 9, 10,
									-1 } },
	{ arpAndChordsTabWidget::tr( "Aeolian" ), { 0, 2, 3, 5, 7, 8, 10,
									-1 } },
	{ arpAndChordsTabWidget::tr( "Locrian" ), { 0, 1, 3, 5, 6, 8, 10,
									-1 } },

	{ "", { -1, -1 } }

} ;


const int CHORDS_GROUPBOX_X = 4;
const int CHORDS_GROUPBOX_Y = 5;
const int CHORDS_GROUPBOX_WIDTH = 238;
const int CHORDS_GROUPBOX_HEIGHT = 65;
const int ARP_GROUPBOX_X = CHORDS_GROUPBOX_X;
const int ARP_GROUPBOX_Y = 10 + CHORDS_GROUPBOX_Y + CHORDS_GROUPBOX_HEIGHT;
const int ARP_GROUPBOX_WIDTH = CHORDS_GROUPBOX_WIDTH;
const int ARP_GROUPBOX_HEIGHT = 240 - ARP_GROUPBOX_Y;



arpAndChordsTabWidget::arpAndChordsTabWidget(
					instrumentTrack * _instrument_track ) :
	QWidget( _instrument_track->tabWidgetParent() )
{
	m_chordsGroupBox = new groupBox( tr( "CHORDS" ), this,
							_instrument_track );
	m_chordsGroupBox->setGeometry( CHORDS_GROUPBOX_X, CHORDS_GROUPBOX_Y,
						CHORDS_GROUPBOX_WIDTH,
						CHORDS_GROUPBOX_HEIGHT );

	m_chordsComboBox = new comboBox( m_chordsGroupBox, tr( "Chord type" ),
							_instrument_track );
	m_chordsComboBox->setFont( pointSize<8>( m_chordsComboBox->font() ) );
	m_chordsComboBox->setGeometry( 10, 25, 140, 22 );

	for( int i = 0; s_chords[i].interval[0] != -1; ++i )
	{
		m_chordsComboBox->addItem( tr( s_chords[i].name
#ifdef QT4
							.toAscii().constData()
#endif
						) );
	}

	m_chordRangeKnob = new knob( knobBright_26, m_chordsGroupBox,
							tr( "Chord range" ),
							_instrument_track );
	m_chordRangeKnob->setLabel( tr( "RANGE" ) );
	m_chordRangeKnob->setRange( 1.0f, 9.0f, 1.0f );
	m_chordRangeKnob->setInitValue( 1.0f );
	m_chordRangeKnob->move( 164, 24 );
	m_chordRangeKnob->setHintText( tr( "Chord range:" ) + " ", " " +
							tr( "octave(s)" ) );
#ifdef QT4
	m_chordRangeKnob->setWhatsThis(
#else
	QWhatsThis::add( m_chordRangeKnob,
#endif
		tr( "Use this knob for setting the chord range in octaves. "
			"The selected chord will be played within specified "
			"amount of octaves." ) );




	m_arpGroupBox = new groupBox( tr( "ARPEGGIO" ), this,
							_instrument_track );
	m_arpGroupBox->setGeometry( ARP_GROUPBOX_X, ARP_GROUPBOX_Y,
					ARP_GROUPBOX_WIDTH,
					ARP_GROUPBOX_HEIGHT );

#ifdef QT4
	m_arpGroupBox->setWhatsThis(
#else
	QWhatsThis::add( m_arpGroupBox,
#endif
		tr( "An arpeggio is a type of playing (especially plucked) "
			"instruments, which makes the music much livelier. "
			"The strings of such instruments (e.g. harps) are "
			"plucked like chords, the only difference is, that "
			"this is done in a sequential order, so the notes are "
			"not played at the same time. Typical arpeggios are "
			"major or minor triads. But there're a lot of other "
			"possible chords, you can select." ) );
	m_arpComboBox = new comboBox( m_arpGroupBox, tr( "Arpeggio type" ),
							_instrument_track );
	m_arpComboBox->setFont( pointSize<8>( m_arpComboBox->font() ) );
	m_arpComboBox->setGeometry( 10, 25, 140, 22 );

	for( int i = 0; s_chords[i].interval[0] != -1; ++i )
	{
		m_arpComboBox->addItem( tr( s_chords[i].name
#ifdef QT4
							.toAscii().constData()
#endif
						) );
	}


	m_arpRangeKnob = new knob( knobBright_26, m_arpGroupBox,
							tr( "Arpeggio range" ),
							_instrument_track );
	m_arpRangeKnob->setLabel( tr( "RANGE" ) );
	m_arpRangeKnob->setRange( 1.0f, 9.0f, 1.0f );
	m_arpRangeKnob->setInitValue( 1.0f );
	m_arpRangeKnob->move( 164, 24 );
	m_arpRangeKnob->setHintText( tr( "Arpeggio range:" ) + " ", " " +
							tr( "octave(s)" ) );
#ifdef QT4
	m_arpRangeKnob->setWhatsThis(
#else
	QWhatsThis::add( m_arpRangeKnob,
#endif
		tr( "Use this knob for setting the arpeggio range in octaves. "
			"The selected arpeggio will be played within specified "
			"amount of octaves." ) );

	m_arpTimeKnob = new tempoSyncKnob( knobBright_26, m_arpGroupBox,
							tr( "Arpeggio time" ),
							_instrument_track );
	m_arpTimeKnob->setLabel( tr( "TIME" ) );
	m_arpTimeKnob->setRange( 25.0f, 2000.0f, 1.0f );
	m_arpTimeKnob->setInitValue( 100.0f );
	m_arpTimeKnob->move( 164, 70 );
	m_arpTimeKnob->setHintText( tr( "Arpeggio time:" ) + " ", " " +
								tr( "ms" ) );
#ifdef QT4
	m_arpTimeKnob->setWhatsThis(
#else
	QWhatsThis::add( m_arpTimeKnob,
#endif
		tr( "Use this knob for setting the arpeggio time in "
			"milliseconds. The arpeggio time specifies how long "
			"each arpeggio-tone should be played." ) );

	m_arpGateKnob = new knob( knobBright_26, m_arpGroupBox,
							tr( "Arpeggio gate" ),
							_instrument_track );
	m_arpGateKnob->setLabel( tr( "GATE" ) );
	m_arpGateKnob->setRange( 1.0f, 200.0f, 1.0f );
	m_arpGateKnob->setInitValue( 100.0f );
	m_arpGateKnob->move( 204, 70 );
	m_arpGateKnob->setHintText( tr( "Arpeggio gate:" ) + " ", tr( "%" ) );
#ifdef QT4
	m_arpGateKnob->setWhatsThis(
#else
	QWhatsThis::add( m_arpGateKnob,
#endif
		tr( "Use this knob for setting the arpeggio gate. The "
			"arpeggio gate specifies the percent of a whole "
			"arpeggio-tone that should be played. With this you "
			"can make cool staccato-arpeggios." ) );

	m_arpDirectionLbl = new QLabel( tr( "Direction:" ), m_arpGroupBox );
	m_arpDirectionLbl->setGeometry( 10, 60, 64, 10 );
	m_arpDirectionLbl->setFont( pointSize<7>( m_arpDirectionLbl->font() ) );



	pixmapButton * arp_up_btn = new pixmapButton( m_arpGroupBox, NULL,
									NULL );
	arp_up_btn->move( 10, 74 );
	arp_up_btn->setActiveGraphic( embed::getIconPixmap( "arp_up_on" ) );
	arp_up_btn->setInactiveGraphic( embed::getIconPixmap( "arp_up_off" ) );
#ifdef QT3
	arp_up_btn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( arp_up_btn, tr( "arpeggio direction = up" ) );


	pixmapButton * arp_down_btn = new pixmapButton( m_arpGroupBox, NULL,
									NULL );
	arp_down_btn->move( 30, 74 );
	arp_down_btn->setActiveGraphic( embed::getIconPixmap( "arp_down_on" ) );
	arp_down_btn->setInactiveGraphic( embed::getIconPixmap(
							"arp_down_off" ) );
#ifdef QT3
	arp_down_btn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( arp_down_btn, tr( "arpeggio direction = down" ) );


	pixmapButton * arp_up_and_down_btn = new pixmapButton( m_arpGroupBox,
								NULL, NULL );
	arp_up_and_down_btn->move( 50, 74 );
	arp_up_and_down_btn->setActiveGraphic( embed::getIconPixmap(
						"arp_up_and_down_on" ) );
	arp_up_and_down_btn->setInactiveGraphic( embed::getIconPixmap(
						"arp_up_and_down_off" ) );
#ifdef QT3
	arp_up_and_down_btn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( arp_up_and_down_btn,
				tr( "arpeggio direction = up and down" ) );


	pixmapButton * arp_random_btn = new pixmapButton( m_arpGroupBox, NULL,
									NULL );
	arp_random_btn->move( 70, 74 );
	arp_random_btn->setActiveGraphic( embed::getIconPixmap(
							"arp_random_on" ) );
	arp_random_btn->setInactiveGraphic( embed::getIconPixmap(
							"arp_random_off" ) );
#ifdef QT3
	arp_random_btn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( arp_random_btn, tr( "arpeggio direction = random" ) );

	m_arpDirectionBtnGrp = new automatableButtonGroup( this,
						tr( "Arpeggio direction" ),
						_instrument_track );
	m_arpDirectionBtnGrp->addButton( arp_up_btn );
	m_arpDirectionBtnGrp->addButton( arp_down_btn );
	m_arpDirectionBtnGrp->addButton( arp_up_and_down_btn );
	m_arpDirectionBtnGrp->addButton( arp_random_btn );

	m_arpDirectionBtnGrp->setInitValue( UP );


	QLabel * mode_lbl = new QLabel( tr( "Mode:" ), m_arpGroupBox );
	mode_lbl->setGeometry( 10, 104, 64, 10 );
	mode_lbl->setFont( pointSize<7>( mode_lbl->font() ) );

	m_arpModeComboBox = new comboBox( m_arpGroupBox, tr( "Arpeggio mode" ),
							_instrument_track );
	m_arpModeComboBox->setFont( pointSize<8>( m_arpModeComboBox->font() ) );
	m_arpModeComboBox->setGeometry( 10, 118, 128, 22 );

	m_arpModeComboBox->addItem( tr( "Free" ),
					embed::getIconPixmap( "arp_free" ) );
	m_arpModeComboBox->addItem( tr( "Sort" ),
					embed::getIconPixmap( "arp_sort" ) );
	m_arpModeComboBox->addItem( tr( "Sync" ),
					embed::getIconPixmap( "arp_sync" ) );
	//m_arpModeComboBox->setValue( 0 );
}




arpAndChordsTabWidget::~arpAndChordsTabWidget()
{
}




void arpAndChordsTabWidget::processNote( notePlayHandle * _n )
{
	const int base_note_key = _n->key();
	// we add chord-subnotes to note if either note is a base-note and
	// arpeggio is not used or note is part of an arpeggio
	// at the same time we only add sub-notes if nothing of the note was
	// played yet, because otherwise we would add chord-subnotes every
	// time an audio-buffer is rendered...
	if( ( ( _n->baseNote() && m_arpGroupBox->isActive() == FALSE ) ||
							_n->arpNote() ) &&
					_n->totalFramesPlayed() == 0 &&
					m_chordsGroupBox->isActive() == TRUE )
	{
		// then insert sub-notes for chord
		const int selected_chord = m_chordsComboBox->value();

		for( int octave_cnt = 0;
			octave_cnt < m_chordRangeKnob->value(); ++octave_cnt )
		{
			const int sub_note_key_base = base_note_key +
						octave_cnt * NOTES_PER_OCTAVE;
			// if octave_cnt == 1 we're in the first octave and
			// the base-note is already done, so we don't have to
			// create it in the following loop, then we loop until
			// there's a -1 in the interval-array
			for( int i = ( octave_cnt == 0 ) ? 1 : 0;
				s_chords[selected_chord].interval[i] != -1;
									++i )
			{
				// add interval to sub-note-key
				const int sub_note_key = sub_note_key_base +
							(int) s_chords[
						selected_chord].interval[i];
				// maybe we're out of range -> let's get outta
				// here!
				if( sub_note_key > NOTES_PER_OCTAVE*OCTAVES )
				{
					break;
				}
				// create copy of base-note
				note note_copy( _n->length(), 0,
						(tones)( sub_note_key %
							NOTES_PER_OCTAVE ),
						(octaves)( sub_note_key /
							NOTES_PER_OCTAVE ),
							_n->getVolume(),
							_n->getPanning(),
							_n->detuning() );
				// create sub-note-play-handle, only note is
				// different
				new notePlayHandle( _n->getInstrumentTrack(),
							_n->offset(),
							_n->frames(), note_copy,
							_n );
			}
		}
	}


	// now follows code for arpeggio

	if( _n->baseNote() == FALSE ||
			!m_arpGroupBox->isActive() ||
			( _n->released() && _n->releaseFramesDone() >=
					_n->actualReleaseFramesToDo() ) )
	{
		return;
	}


	const int selected_arp = m_arpComboBox->value();

	constNotePlayHandleVector cnphv = notePlayHandle::nphsOfInstrumentTrack(
						_n->getInstrumentTrack() );
	if( m_arpModeComboBox->value() != FREE && cnphv.size() == 0 )
	{
		// maybe we're playing only a preset-preview-note?
		cnphv = presetPreviewPlayHandle::nphsOfInstrumentTrack(
						_n->getInstrumentTrack() );
		if( cnphv.size() == 0 )
		{
			// still nothing found here, so lets return
			//return;
			cnphv.push_back( _n );
		}
	}

	const int cur_chord_size = getChordSize( s_chords[selected_arp] );
	const int range = (int)( cur_chord_size * m_arpRangeKnob->value() );
	const int total_range = range * cnphv.size();

	// number of frames that every note should be played
	const f_cnt_t arp_frames = (f_cnt_t)( m_arpTimeKnob->value() / 1000.0f *
					engine::getMixer()->sampleRate() );
	const f_cnt_t gated_frames = (f_cnt_t)( m_arpGateKnob->value() *
							arp_frames / 100.0f );

	// used for calculating remaining frames for arp-note, we have to add
	// arp_frames-1, otherwise the first arp-note will not be setup
	// correctly... -> arp_frames frames silence at the start of every note!
	int cur_frame = ( ( m_arpModeComboBox->value() != FREE ) ?
				cnphv.first()->totalFramesPlayed() :
				_n->totalFramesPlayed() ) + arp_frames - 1;
	// used for loop
	f_cnt_t frames_processed = 0;

	while( frames_processed < engine::getMixer()->framesPerPeriod() )
	{
		const f_cnt_t remaining_frames_for_cur_arp = arp_frames -
						( cur_frame % arp_frames );
		// does current arp-note fill whole audio-buffer?
		if( remaining_frames_for_cur_arp >
				engine::getMixer()->framesPerPeriod() )
		{
			// then we don't have to do something!
			break;
		}

		frames_processed += remaining_frames_for_cur_arp;

		// init with zero
		int cur_arp_idx = 0;

		// in sorted mode: is it our turn or do we have to be quiet for
		// now?
		if( m_arpModeComboBox->value() == SORT &&
				( ( cur_frame / arp_frames ) % total_range ) /
						range != (f_cnt_t) _n->index() )
		{
			// update counters
			frames_processed += arp_frames;
			cur_frame += arp_frames;
			continue;
		}

		const int dir = m_arpDirectionBtnGrp->value();
		// process according to arpeggio-direction...
		if( dir == UP )
		{
			cur_arp_idx = ( cur_frame / arp_frames ) % range;
		}
		else if( dir == DOWN )
		{
			cur_arp_idx = range - ( cur_frame / arp_frames ) %
								range - 1;
		}
		else if( dir == UP_AND_DOWN && range > 1 )
		{
			// imagine, we had to play the arp once up and then
			// once down -> makes 2 * range possible notes...
			// because we don't play the lower and upper notes
			// twice, we have to subtract 2
			cur_arp_idx = ( cur_frame / arp_frames ) %
							( range * 2 - 2 );
			// if greater than range, we have to play down...
			// looks like the code for arp_dir==DOWN... :)
			if( cur_arp_idx >= range )
			{
				cur_arp_idx = range - cur_arp_idx %
							( range - 1 ) - 1;
			}
		}
		else if( dir == RANDOM )
		{
			// just pick a random chord-index
			cur_arp_idx = (int)( range * ( (float) rand() /
							(float) RAND_MAX ) );
		}

		// now calculate final key for our arp-note
		const int sub_note_key = base_note_key + (cur_arp_idx /
							cur_chord_size ) *
							NOTES_PER_OCTAVE +
	s_chords[selected_arp].interval[cur_arp_idx % cur_chord_size];

		// range-checking
		if( sub_note_key >= NOTES_PER_OCTAVE * OCTAVES ||
			sub_note_key < 0 ||
					engine::getMixer()->criticalXRuns() )
		{
			continue;
		}

		float vol_level = 1.0f;
		if( _n->released() )
		{
			vol_level = _n->volumeLevel( cur_frame + gated_frames );
		}

		// create new arp-note
		note new_note( midiTime( 0 ), midiTime( 0 ),
				static_cast<tones>( sub_note_key %
							NOTES_PER_OCTAVE ),
				static_cast<octaves>( sub_note_key /
							NOTES_PER_OCTAVE ),
				static_cast<volume>( _n->getVolume() *
								vol_level ),
				_n->getPanning(), _n->detuning() );

		// create sub-note-play-handle, only ptr to note is different
		// and is_arp_note=TRUE
		new notePlayHandle( _n->getInstrumentTrack(),
				( ( m_arpModeComboBox->value() != FREE ) ?
						cnphv.first()->offset() :
						_n->offset() ) +
							frames_processed,
						gated_frames,
						new_note,
						_n, TRUE );

		// update counters
		frames_processed += arp_frames;
		cur_frame += arp_frames;
	}

	// make sure, note is handled as arp-base-note, even if we didn't add a
	// sub-note so far
	if( m_arpModeComboBox->value() != FREE )
	{
		_n->setArpNote( TRUE );
	}
}




void arpAndChordsTabWidget::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	m_chordsGroupBox->saveSettings( _doc, _this, "chord-enabled" );
	m_chordsComboBox->saveSettings( _doc, _this, "chord" );
	m_chordRangeKnob->saveSettings( _doc, _this, "chordrange" );

	m_arpGroupBox->saveSettings( _doc, _this, "arp-enabled" );
	m_arpComboBox->saveSettings( _doc, _this, "arp" );
	m_arpRangeKnob->saveSettings( _doc, _this, "arprange" );
	m_arpTimeKnob->saveSettings( _doc, _this, "arptime" );
	m_arpGateKnob->saveSettings( _doc, _this, "arpgate" );
	m_arpDirectionBtnGrp->saveSettings( _doc, _this, "arpdir" );

	m_arpModeComboBox->saveSettings( _doc, _this, "arpmode" );
}




void arpAndChordsTabWidget::loadSettings( const QDomElement & _this )
{
	m_chordsGroupBox->loadSettings( _this, "chord-enabled" );
	m_chordsComboBox->loadSettings( _this, "chord" );
	m_chordRangeKnob->loadSettings( _this, "chordrange" );

	m_arpGroupBox->loadSettings( _this, "arp-enabled" );
	m_arpComboBox->loadSettings( _this, "arp" );
	m_arpRangeKnob->loadSettings( _this, "arprange" );
	m_arpTimeKnob->loadSettings( _this, "arptime" );
	m_arpGateKnob->loadSettings( _this, "arpgate" );
	m_arpDirectionBtnGrp->loadSettings( _this, "arpdir" );

	// Keep compatibility with version 0.2.1 file format
	if( _this.hasAttribute( "arpsyncmode" ) )
	{
	 	m_arpTimeKnob->setSyncMode( 
 		( tempoSyncKnob::tempoSyncMode ) _this.attribute(
 						 "arpsyncmode" ).toInt() );
	}

	m_arpModeComboBox->loadSettings( _this, "arpmode" );
}






#include "arp_and_chords_tab_widget.moc"


#endif
