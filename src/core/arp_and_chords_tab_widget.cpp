#ifndef SINGLE_SOURCE_COMPILE

/*
 * arp_and_chords_tab_widget.cpp - widget for use in arp/chord-tab of 
 *                                 instrument-track-window
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QLabel>

#else

#include <qdom.h>
#include <qlabel.h>
#include <qwhatsthis.h>

#endif


/*#ifdef HAVE_STDLIB_H*/
#include <stdlib.h>
/*#endif*/


#include "arp_and_chords_tab_widget.h"
#include "embed.h"
#include "note_play_handle.h"
#include "song_editor.h"
#include "group_box.h"
#include "pixmap_button.h"
#include "knob.h"
#include "tooltip.h"
#include "gui_templates.h"
#include "tempo_sync_knob.h"
#include "instrument_track.h"
#include "led_checkbox.h"
#include "preset_preview_play_handle.h"
#include "combobox.h"



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



arpAndChordsTabWidget::arpAndChordsTabWidget( instrumentTrack * _instrument_track ) :
	QWidget( _instrument_track->tabWidgetParent() ),
	journallingObject( _instrument_track->eng() )
{
	m_chordsGroupBox = new groupBox( tr( "CHORDS" ), this, eng() );
	m_chordsGroupBox->setGeometry( CHORDS_GROUPBOX_X, CHORDS_GROUPBOX_Y,
						CHORDS_GROUPBOX_WIDTH,
						CHORDS_GROUPBOX_HEIGHT );

	m_chordsComboBox = new comboBox( m_chordsGroupBox, eng() );
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
						tr( "Chord range" ), eng() );
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




	m_arpGroupBox = new groupBox( tr( "ARPEGGIO" ), this, eng() );
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
	m_arpComboBox = new comboBox( m_arpGroupBox, eng() );
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
						tr( "Arpeggio range" ), eng() );
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
						tr( "Arpeggio time" ), eng() );
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
						tr( "Arpeggio gate" ), eng() );
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



	pixmapButton * arp_up_btn = new pixmapButton( m_arpGroupBox, eng() );
	arp_up_btn->move( 10, 74 );
	arp_up_btn->setActiveGraphic( embed::getIconPixmap( "arp_up_on" ) );
	arp_up_btn->setInactiveGraphic( embed::getIconPixmap( "arp_up_off" ) );
#ifdef QT3
	arp_up_btn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( arp_up_btn, tr( "arpeggio direction = up" ) );


	pixmapButton * arp_down_btn = new pixmapButton( m_arpGroupBox, eng() );
	arp_down_btn->move( 30, 74 );
	arp_down_btn->setActiveGraphic( embed::getIconPixmap( "arp_down_on" ) );
	arp_down_btn->setInactiveGraphic( embed::getIconPixmap(
							"arp_down_off" ) );
#ifdef QT3
	arp_down_btn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( arp_down_btn, tr( "arpeggio direction = down" ) );


	pixmapButton * arp_up_and_down_btn = new pixmapButton( m_arpGroupBox,
									eng() );
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


	pixmapButton * arp_random_btn = new pixmapButton( m_arpGroupBox,
									eng() );
	arp_random_btn->move( 70, 74 );
	arp_random_btn->setActiveGraphic( embed::getIconPixmap(
							"arp_random_on" ) );
	arp_random_btn->setInactiveGraphic( embed::getIconPixmap(
							"arp_random_off" ) );
#ifdef QT3
	arp_random_btn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( arp_random_btn, tr( "arpeggio direction = random" ) );

	m_arpDirectionBtnGrp = new automatableButtonGroup( this, eng() );
	m_arpDirectionBtnGrp->addButton( arp_up_btn );
	m_arpDirectionBtnGrp->addButton( arp_down_btn );
	m_arpDirectionBtnGrp->addButton( arp_up_and_down_btn );
	m_arpDirectionBtnGrp->addButton( arp_random_btn );

	m_arpDirectionBtnGrp->setInitValue( UP - 1 );


	QLabel * mode_lbl = new QLabel( tr( "Mode:" ), m_arpGroupBox );
	mode_lbl->setGeometry( 10, 104, 64, 10 );
	mode_lbl->setFont( pointSize<7>( mode_lbl->font() ) );

	m_arpModeComboBox = new comboBox( m_arpGroupBox, eng() );
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
				note note_copy( NULL, 0, 0,
						(tones)( sub_note_key %
							NOTES_PER_OCTAVE ),
						(octaves)( sub_note_key /
							NOTES_PER_OCTAVE ),
							_n->getVolume(),
							_n->getPanning() );
				// duplicate note-play-handle, only note is
				// different
				notePlayHandle * note_play_handle =
					new notePlayHandle(
						_n->getInstrumentTrack(),
						_n->framesAhead(),
						_n->frames(), note_copy );
				// add sub-note to base-note, now all stuff is
				// done by notePlayHandle::play_note()
				_n->addSubNote( note_play_handle );
			}
		}
	}


	// now follows code for arpeggio

	if( _n->baseNote() == FALSE ||
		( m_arpDirectionBtnGrp->value() + 1) == OFF ||
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
					eng()->getMixer()->sampleRate() );
	const f_cnt_t gated_frames = (f_cnt_t)( m_arpGateKnob->value() *
							arp_frames / 100.0f );

	// used for calculating remaining frames for arp-note, we have to add
	// arp_frames-1, otherwise the first arp-note will not be setup
	// correctly... -> arp_frames frames silence at the start of every note!
	int cur_frame = ( ( m_arpModeComboBox->value() != FREE ) ?
				cnphv.first()->totalFramesPlayed() :
				_n->totalFramesPlayed() )
							+ arp_frames - 1;
	// used for loop
	fpab_t frames_processed = 0;

	while( frames_processed < eng()->getMixer()->framesPerAudioBuffer() )
	{
		const f_cnt_t remaining_frames_for_cur_arp = arp_frames -
						( cur_frame % arp_frames );
		// does current arp-note fill whole audio-buffer?
		if( remaining_frames_for_cur_arp >
				eng()->getMixer()->framesPerAudioBuffer() )
		{
			// then we don't have to do something!
			break;
		}

		frames_processed += remaining_frames_for_cur_arp;

		// init with zero
		int cur_arp_idx = 0;

		if( m_arpModeComboBox->value() == SORT &&
				( ( cur_frame / arp_frames ) % total_range ) /
					range != (Uint32) _n->index() )
		{
			// update counters
			frames_processed += arp_frames;
			cur_frame += arp_frames;
			continue;
		}

		arpDirections dir = static_cast<arpDirections>(
					m_arpDirectionBtnGrp->value() + 1 );
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
			sub_note_key < 0 || eng()->getMixer()->criticalXRuns() )
		{
			continue;
		}

		float vol_level = 1.0f;
		if( _n->released() )
		{
			vol_level = _n->volumeLevel( cur_frame + gated_frames );
		}

		// create new arp-note
		note new_note( NULL, midiTime( 0 ), midiTime( 0 ),
				static_cast<tones>( sub_note_key %
							NOTES_PER_OCTAVE ),
				static_cast<octaves>( sub_note_key /
							NOTES_PER_OCTAVE ),
				static_cast<volume>( _n->getVolume() *
								vol_level ),
				_n->getPanning() );

		// duplicate note-play-handle, only ptr to note is different
		// and is_arp_note=TRUE
		notePlayHandle * note_play_handle = new notePlayHandle(
						_n->getInstrumentTrack(),
					( ( m_arpModeComboBox->value() !=
					    				FREE ) ?
						cnphv.first()->framesAhead() :
						_n->framesAhead() ) +
							frames_processed,
						gated_frames,
						new_note,
						TRUE );

		// add sub-note to base-note - now all stuff is done by
		// notePlayHandle::playNote()
		_n->addSubNote( note_play_handle );

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
	_this.setAttribute( "chorddisabled", !m_chordsGroupBox->isActive() );
	_this.setAttribute( "chord", m_chordsComboBox->value() );
	_this.setAttribute( "chordrange", m_chordRangeKnob->value() );

	_this.setAttribute( "arpdisabled", !m_arpGroupBox->isActive() );
	_this.setAttribute( "arp", m_arpComboBox->value() );
	_this.setAttribute( "arprange", m_arpRangeKnob->value() );
	_this.setAttribute( "arptime", m_arpTimeKnob->value() );
	_this.setAttribute( "arpgate", m_arpGateKnob->value() );
	_this.setAttribute( "arpdir", m_arpDirectionBtnGrp->value() + 1 );
	_this.setAttribute( "arpsyncmode",
					( int ) m_arpTimeKnob->getSyncMode() );

	_this.setAttribute( "arpmode", m_arpModeComboBox->value() );
}




void arpAndChordsTabWidget::loadSettings( const QDomElement & _this )
{
	m_chordsGroupBox->setState( !_this.attribute
						( "chorddisabled" ).toInt() );
	m_chordsComboBox->setValue( _this.attribute( "chord" ).toInt() );
	m_chordRangeKnob->setValue( _this.attribute( "chordrange" ).toFloat() );
	m_arpComboBox->setValue( _this.attribute( "arp" ).toInt() );
	m_arpRangeKnob->setValue( _this.attribute( "arprange" ).toFloat() );
	m_arpTimeKnob->setValue( _this.attribute( "arptime" ).toFloat() );
	m_arpGateKnob->setValue( _this.attribute( "arpgate" ).toFloat() );
	m_arpDirectionBtnGrp->setInitValue(
				_this.attribute( "arpdir" ).toInt() - 1 );
	m_arpTimeKnob->setSyncMode( 
		( tempoSyncKnob::tempoSyncMode ) _this.attribute(
						 "arpsyncmode" ).toInt() );

	m_arpModeComboBox->setValue( _this.attribute( "arpmode" ).toInt() );

	m_arpGroupBox->setState( _this.attribute( "arpdir" ).toInt() != OFF &&
				!_this.attribute( "arpdisabled" ).toInt() );
}






#include "arp_and_chords_tab_widget.moc"


#endif
