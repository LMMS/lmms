/*
 * arp_and_chords_tab_widget.cpp - widget for use in arp/chord-tab of 
 *                                 channel-window
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QButtonGroup>
#include <QLabel>
#include <QComboBox>

#else

#include <qbuttongroup.h>
#include <qdom.h>
#include <qlabel.h>
#include <qcombobox.h>

#define setChecked setOn

#endif


#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


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
#include "channel_track.h"



arpAndChordsTabWidget::chord arpAndChordsTabWidget::s_chords[] =
{
	// thanks to the FL-team for this chords *lol* ;-) took me at least 3
	// hours to get them all out of FL-arpeggiator...

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
const int ARP_GROUPBOX_HEIGHT = 200 - ARP_GROUPBOX_Y;



arpAndChordsTabWidget::arpAndChordsTabWidget( channelTrack * _channel_track ) :
	QWidget( _channel_track->tabWidgetParent() ),
	settings(),
	m_arpDirection( UP )
{
	m_chordsGroupBox = new groupBox( tr( "CHORDS" ), this );
	m_chordsGroupBox->setGeometry( CHORDS_GROUPBOX_X, CHORDS_GROUPBOX_Y,
						CHORDS_GROUPBOX_WIDTH,
						CHORDS_GROUPBOX_HEIGHT );

	m_chordsComboBox = new QComboBox( m_chordsGroupBox );
	m_chordsComboBox->setFont( pointSize<9>( m_chordsComboBox->font() ) );
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
							tr( "Chord range" ) );
	m_chordRangeKnob->setLabel( tr( "RANGE" ) );
	m_chordRangeKnob->setRange( 1.0, 9.0, 1.0 );
	m_chordRangeKnob->setValue( 1.0, TRUE );
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




	m_arpGroupBox = new groupBox( tr( "ARPEGGIO" ), this );
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
	m_arpComboBox = new QComboBox( m_arpGroupBox );
	m_arpComboBox->setFont( pointSize<9>( m_arpComboBox->font() ) );
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
						tr( "Arpeggio range" ) );
	m_arpRangeKnob->setLabel( tr( "RANGE" ) );
	m_arpRangeKnob->setRange( 1.0, 9.0, 1.0 );
	m_arpRangeKnob->setValue( 1.0, TRUE );
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
							tr( "Arpeggio time" ) );
	m_arpTimeKnob->setLabel( tr( "TIME" ) );
	m_arpTimeKnob->setRange( 10.0, 1000.0, 1.0 );
	m_arpTimeKnob->setValue( 100.0, TRUE );
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
							tr( "Arpeggio gate" ) );
	m_arpGateKnob->setLabel( tr( "GATE" ) );
	m_arpGateKnob->setRange( 1.0, 200.0, 1.0 );
	m_arpGateKnob->setValue( 100.0, TRUE );
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

	m_arpDirectionLbl = new QLabel( tr( "DIRECTION:" ), m_arpGroupBox );
	m_arpDirectionLbl->setGeometry( 10, 70, 64, 8 );
	m_arpDirectionLbl->setFont( pointSize<6>( m_arpDirectionLbl->font() ) );



	m_arpUpBtn = new pixmapButton( m_arpGroupBox );
	m_arpUpBtn->move( 70, 70 );
	m_arpUpBtn->setActiveGraphic( embed::getIconPixmap( "arp_up_on" ) );
	m_arpUpBtn->setInactiveGraphic( embed::getIconPixmap( "arp_up_off" ) );
#ifdef QT4
	m_arpUpBtn->setChecked( TRUE );
#else
	m_arpUpBtn->setOn( TRUE );
#endif
#ifndef QT4
	m_arpUpBtn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( m_arpUpBtn, tr( "arpeggio direction = up" ) );
	connect( m_arpUpBtn, SIGNAL( toggled( bool ) ), this,
						SLOT( arpUpToggled( bool ) ) );

	m_arpDownBtn = new pixmapButton( m_arpGroupBox );
	m_arpDownBtn->move( 90, 70 );
	m_arpDownBtn->setActiveGraphic( embed::getIconPixmap( "arp_down_on" ) );
	m_arpDownBtn->setInactiveGraphic( embed::getIconPixmap(
							"arp_down_off" ) );
#ifndef QT4
	m_arpDownBtn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( m_arpDownBtn, tr( "arpeggio direction = down" ) );
	connect( m_arpDownBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( arpDownToggled( bool ) ) );

	m_arpUpAndDownBtn = new pixmapButton( m_arpGroupBox );
	m_arpUpAndDownBtn->move( 110, 70 );
	m_arpUpAndDownBtn->setActiveGraphic( embed::getIconPixmap(
						"arp_up_and_down_on" ) );
	m_arpUpAndDownBtn->setInactiveGraphic( embed::getIconPixmap(
						"arp_up_and_down_off" ) );
#ifndef QT4
	m_arpUpAndDownBtn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( m_arpUpAndDownBtn,
				tr( "arpeggio direction = up and down" ) );
	connect( m_arpUpAndDownBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( arpUpAndDownToggled( bool ) ) );

	m_arpRandomBtn = new pixmapButton( m_arpGroupBox );
	m_arpRandomBtn->move( 130, 70 );
	m_arpRandomBtn->setActiveGraphic( embed::getIconPixmap(
							"arp_random_on" ) );
	m_arpRandomBtn->setInactiveGraphic( embed::getIconPixmap(
							"arp_random_off" ) );
#ifndef QT4
	m_arpRandomBtn->setBackgroundMode( Qt::PaletteBackground );
#endif
	toolTip::add( m_arpRandomBtn, tr( "arpeggio direction = random" ) );
	connect( m_arpRandomBtn, SIGNAL( toggled( bool ) ), this,
					SLOT( arpRandomToggled( bool ) ) );

	QButtonGroup * m_arpDirections_group = new QButtonGroup( this );
	m_arpDirections_group->addButton( m_arpUpBtn );
	m_arpDirections_group->addButton( m_arpDownBtn );
	m_arpDirections_group->addButton( m_arpUpAndDownBtn );
	m_arpDirections_group->addButton( m_arpRandomBtn );
	m_arpDirections_group->setExclusive( TRUE );
#ifndef QT4
	m_arpDirections_group->hide();
#endif
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
#ifdef QT4
		const int selected_chord = m_chordsComboBox->currentIndex();
#else
		const int selected_chord = m_chordsComboBox->currentItem();
#endif
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
				note note_copy( 0, 0, (tones)( sub_note_key %
							NOTES_PER_OCTAVE ),
						(octaves)( sub_note_key /
							NOTES_PER_OCTAVE ),
							_n->getVolume(),
							_n->getPanning() );
				// duplicate note-play-handle, only note is
				// different
				notePlayHandle * note_play_handle =
					new notePlayHandle(
						_n->getChannelTrack(),
						_n->framesAhead(),
						_n->frames(), &note_copy );
				// add sub-note to base-note, now all stuff is
				// done by notePlayHandle::play_note()
				_n->addSubNote( note_play_handle );
			}
		}
	}


	// now follows code for arpeggio

	if( _n->baseNote() == FALSE || m_arpDirection == OFF ||
		!m_arpGroupBox->isActive() ||
		( _n->released() && _n->releaseFramesDone() >=
					_n->actualReleaseFramesToDo() ) )
	{
		return;
	}


#ifdef QT4
	const int selected_arp = m_arpComboBox->currentIndex();
#else
	const int selected_arp = m_arpComboBox->currentItem();
#endif

	const int cur_chord_size = getChordSize( s_chords[selected_arp] );
	const int total_range = (int)( cur_chord_size *
						m_arpRangeKnob->value() );
	// number of frames that every note should be played
	const Uint32 arp_frames = (Uint32)( m_arpTimeKnob->value() / 1000.0f *
						mixer::inst()->sampleRate() );
	const Uint32 gated_frames = (Uint32)( m_arpGateKnob->value() *
							arp_frames / 100.0f );
	// used for calculating remaining frames for arp-note, we have to add
	// arp_frames-1, otherwise the first arp-note will not be setup
	// correctly... -> arp_frames frames silence at the start of every note!
	int cur_frame = _n->totalFramesPlayed() + arp_frames - 1;
	// used for loop
	Uint32 frames_processed = 0;
	// len for all arp-notes (depending on arp-time)

	while( frames_processed < mixer::inst()->framesPerAudioBuffer() )
	{
		const Uint32 remaining_frames_for_cur_arp = arp_frames -
						( cur_frame % arp_frames );
		// does current arp-note fill whole audio-buffer?
		if( remaining_frames_for_cur_arp >=
					mixer::inst()->framesPerAudioBuffer() )
		{
			// then we don't have to do something!
			break;
		}

		frames_processed += remaining_frames_for_cur_arp;

		// init with zero
		int cur_arp_idx = 0;

		// process according to arpeggio-direction...
		if( m_arpDirection == UP )
		{
			cur_arp_idx = ( cur_frame / arp_frames ) % total_range;
		}
		else if( m_arpDirection == DOWN )
		{
			cur_arp_idx = total_range - ( cur_frame / arp_frames ) %
						total_range - 1;
		}
		else if( m_arpDirection == UP_AND_DOWN && total_range > 1 )
		{
			// imagine, we had to play the arp once up and then
			// once down -> makes 2 * total_range possible notes...
			// because we don't play the lower and upper notes
			// twice, we have to subtract 2
			cur_arp_idx = ( cur_frame / arp_frames ) %
							( total_range * 2 - 2 );
			// if greater than total_range, we have to play down...
			// looks like the code for arp_dir==DOWN... :)
			if( cur_arp_idx >= total_range )
			{
				cur_arp_idx = total_range - cur_arp_idx %
							( total_range - 1 ) - 1;
			}
		}
		else if( m_arpDirection == RANDOM )
		{
			// just pick a random chord-index
			cur_arp_idx = (int)( total_range * ( (float) rand() /
							(float) RAND_MAX ) );
		}

		// now calculate final key for our arp-note
		const int sub_note_key = base_note_key + (cur_arp_idx /
							cur_chord_size ) *
							NOTES_PER_OCTAVE +
	s_chords[selected_arp].interval[cur_arp_idx % cur_chord_size];

		// range-checking
		if( sub_note_key >= NOTES_PER_OCTAVE * OCTAVES ||
							sub_note_key < 0 )
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
				_n->getPanning() );

		// duplicate note-play-handle, only ptr to note is different
		// and is_arp_note=TRUE
		notePlayHandle * note_play_handle = new notePlayHandle(
						_n->getChannelTrack(),
						_n->framesAhead() +
							frames_processed,
						gated_frames,
						&new_note,
						TRUE );

		// add sub-note to base-note, now all stuff is done by
		// notePlayHandle::play_note()
		_n->addSubNote( note_play_handle );

		// update counters
		frames_processed += arp_frames;
		cur_frame += arp_frames;
	}
}




void arpAndChordsTabWidget::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement act_de = _doc.createElement( nodeName() );
	act_de.setAttribute( "chorddisabled", QString::number(
					!m_chordsGroupBox->isActive() ) );
#ifdef QT4
	act_de.setAttribute( "chord", QString::number(
					m_chordsComboBox->currentIndex() ) );
#else
	act_de.setAttribute( "chord", QString::number(
					m_chordsComboBox->currentItem() ) );
#endif
	act_de.setAttribute( "chordrange", QString::number(
					m_chordRangeKnob->value() ) );

	act_de.setAttribute( "arpdisabled", QString::number(
						!m_arpGroupBox->isActive() ) );
#ifdef QT4
	act_de.setAttribute( "arp", QString::number(
					m_arpComboBox->currentIndex() ) );
#else
	act_de.setAttribute( "arp", QString::number(
					m_arpComboBox->currentItem() ) );
#endif
	act_de.setAttribute( "arprange", QString::number(
					m_arpRangeKnob->value() ) );
	act_de.setAttribute( "arptime", QString::number(
					m_arpTimeKnob->value() ) );
	act_de.setAttribute( "arpgate", QString::number(
					m_arpGateKnob->value() ) );
	act_de.setAttribute( "arpdir", QString::number(
					m_arpDirection ) );
	act_de.setAttribute( "arpsyncmode", QString::number(
				( int ) m_arpTimeKnob->getSyncMode() ) );


	_parent.appendChild( act_de );
}




void arpAndChordsTabWidget::loadSettings( const QDomElement & _this )
{
	m_chordsGroupBox->setState( !_this.attribute
						( "chorddisabled" ).toInt() );
#ifdef QT4
	m_chordsComboBox->setCurrentIndex( _this.attribute( "chord" ).toInt() );
#else
	m_chordsComboBox->setCurrentItem( _this.attribute( "chord" ).toInt() );
#endif
	m_chordRangeKnob->setValue( _this.attribute( "chordrange" ).toFloat() );
#ifdef QT4
	m_arpComboBox->setCurrentIndex( _this.attribute( "arp" ).toInt() );
#else
	m_arpComboBox->setCurrentItem( _this.attribute( "arp" ).toInt() );
#endif
	m_arpRangeKnob->setValue( _this.attribute( "arprange" ).toFloat() );
	m_arpTimeKnob->setValue( _this.attribute( "arptime" ).toFloat() );
	m_arpGateKnob->setValue( _this.attribute( "arpgate" ).toFloat() );
	m_arpDirection = static_cast<arpDirections>(
					_this.attribute( "arpdir" ).toInt() );
	m_arpTimeKnob->setSyncMode( 
				( tempoSyncMode ) _this.attribute( "arpsyncmode" ).toInt() );

	m_arpGroupBox->setState( m_arpDirection != OFF &&
				!_this.attribute( "arpdisabled" ).toInt() );
	switch( m_arpDirection )
	{
		case DOWN:
			m_arpDownBtn->setChecked( TRUE );
			break;
		case UP_AND_DOWN:
			m_arpUpAndDownBtn->setChecked( TRUE );
			break;
		case RANDOM:
			m_arpRandomBtn->setChecked( TRUE );
			break;
		case UP:
		default:
			m_arpUpBtn->setChecked( TRUE );
			m_arpDirection = UP;
			break;
	}
}




void arpAndChordsTabWidget::arpUpToggled( bool _on )
{
	if( _on )
	{
		m_arpDirection = UP;
	}
	songEditor::inst()->setModified();
}




void arpAndChordsTabWidget::arpDownToggled( bool _on )
{
	if( _on )
	{
		m_arpDirection = DOWN;
	}
	songEditor::inst()->setModified();
}




void arpAndChordsTabWidget::arpUpAndDownToggled( bool _on )
{
	if( _on )
	{
		m_arpDirection = UP_AND_DOWN;
	}
	songEditor::inst()->setModified();
}




void arpAndChordsTabWidget::arpRandomToggled( bool _on )
{
	if( _on )
	{
		m_arpDirection = RANDOM;
	}
	songEditor::inst()->setModified();
}




#undef setChecked

#include "arp_and_chords_tab_widget.moc"

