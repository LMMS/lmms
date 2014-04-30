/*
 * PianoRoll.cpp - implementation of piano roll which is used for actual
 *                  writing of melodies
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008 Andrew Kelley <superjoe30/at/gmail/dot/com>
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

#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMdiArea>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QStyleOption>
#include <QtGui/QWheelEvent>
#include <QString>
#include <QSignalMapper>

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#include <math.h>
#include <algorithm>

#include "config_mgr.h"
#include "PianoRoll.h"
#include "bb_track_container.h"
#include "Clipboard.h"
#include "combobox.h"
#include "debug.h"
#include "DetuningHelper.h"
#include "embed.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "MidiEvent.h"
#include "DataFile.h"
#include "pattern.h"
#include "Piano.h"
#include "pixmap_button.h"
#include "song.h"
#include "SongEditor.h"
#include "templates.h"
#include "text_float.h"
#include "timeline.h"
#include "tool_button.h"
#include "tooltip.h"


typedef AutomationPattern::timeMap timeMap;


extern Keys whiteKeys[];	// defined in piano_widget.cpp


// some constants...
const int INITIAL_PIANOROLL_HEIGHT = 480;

const int SCROLLBAR_SIZE = 16;
const int PIANO_X = 0;

const int WHITE_KEY_WIDTH = 64;
const int BLACK_KEY_WIDTH = 41;
const int WHITE_KEY_SMALL_HEIGHT = 18;
const int WHITE_KEY_BIG_HEIGHT = 24;
const int BLACK_KEY_HEIGHT = 16;
const int C_KEY_LABEL_X = WHITE_KEY_WIDTH - 19;
const int KEY_LINE_HEIGHT = 12;
const int OCTAVE_HEIGHT = KEY_LINE_HEIGHT * KeysPerOctave;	// = 12 * 12;

const int NOTE_EDIT_RESIZE_BAR = 6;
const int NOTE_EDIT_MIN_HEIGHT = 50;
const int KEY_AREA_MIN_HEIGHT = 100;
const int PR_BOTTOM_MARGIN = SCROLLBAR_SIZE;
const int PR_TOP_MARGIN = 48;
const int PR_RIGHT_MARGIN = SCROLLBAR_SIZE;


// width of area used for resizing (the grip at the end of a note)
const int RESIZE_AREA_WIDTH = 4;

// width of line for setting volume/panning of note
const int NE_LINE_WIDTH = 3;

// key where to start
const int INITIAL_START_KEY = Key_C + Octave_4 * KeysPerOctave;

// number of each note to provide in quantization and note lengths
const int NUM_EVEN_LENGTHS = 6;
const int NUM_TRIPLET_LENGTHS = 5;



QPixmap * PianoRoll::s_whiteKeySmallPm = NULL;
QPixmap * PianoRoll::s_whiteKeySmallPressedPm = NULL;
QPixmap * PianoRoll::s_whiteKeyBigPm = NULL;
QPixmap * PianoRoll::s_whiteKeyBigPressedPm = NULL;
QPixmap * PianoRoll::s_blackKeyPm = NULL;
QPixmap * PianoRoll::s_blackKeyPressedPm = NULL;
QPixmap * PianoRoll::s_toolDraw = NULL;
QPixmap * PianoRoll::s_toolErase = NULL;
QPixmap * PianoRoll::s_toolSelect = NULL;
QPixmap * PianoRoll::s_toolMove = NULL;
QPixmap * PianoRoll::s_toolOpen = NULL;

// used for drawing of piano
PianoRoll::PianoRollKeyTypes PianoRoll::prKeyOrder[] =
{
	PR_WHITE_KEY_SMALL, PR_BLACK_KEY, PR_WHITE_KEY_BIG, PR_BLACK_KEY,
	PR_WHITE_KEY_SMALL, PR_WHITE_KEY_SMALL, PR_BLACK_KEY, PR_WHITE_KEY_BIG,
	PR_BLACK_KEY, PR_WHITE_KEY_BIG, PR_BLACK_KEY, PR_WHITE_KEY_SMALL
} ;


const int DEFAULT_PR_PPT = KEY_LINE_HEIGHT * DefaultStepsPerTact;


PianoRoll::PianoRoll() :
	m_nemStr( QVector<QString>() ),
	m_noteEditMenu( NULL ),
	m_semiToneMarkerMenu( NULL ),
	m_zoomingModel(),
	m_quantizeModel(),
	m_noteLenModel(),
	m_pattern( NULL ),
	m_currentPosition(),
	m_recording( false ),
	m_currentNote( NULL ),
	m_action( ActionNone ),
	m_noteEditMode( NoteEditVolume ),
	m_moveBoundaryLeft( 0 ),
	m_moveBoundaryTop( 0 ),
	m_moveBoundaryRight( 0 ),
	m_moveBoundaryBottom( 0 ),
	m_mouseDownKey( 0 ),
	m_mouseDownTick( 0 ),
	m_lastMouseX( 0 ),
	m_lastMouseY( 0 ),
	m_oldNotesEditHeight( 100 ),
	m_notesEditHeight( 100 ),
	m_ppt( DEFAULT_PR_PPT ),
	m_lenOfNewNotes( MidiTime( 0, DefaultTicksPerTact/4 ) ),
	m_lastNoteVolume( DefaultVolume ),
	m_lastNotePanning( DefaultPanning ),
	m_startKey( INITIAL_START_KEY ),
	m_lastKey( 0 ),
	m_editMode( ModeDraw ),
	m_mouseDownLeft( false ),
	m_mouseDownRight( false ),
	m_scrollBack( false )
{
	// gui names of edit modes
	m_nemStr.push_back( tr( "Note Volume" ) );
	m_nemStr.push_back( tr( "Note Panning" ) );

	QSignalMapper * signalMapper = new QSignalMapper( this );
	m_noteEditMenu = new QMenu( this );
	m_noteEditMenu->clear();
	for( int i=0; i<m_nemStr.size(); ++i )
	{
		QAction * act = new QAction( m_nemStr.at(i), this );
		connect( act, SIGNAL(triggered()), signalMapper, SLOT(map()) );
		signalMapper->setMapping( act, i );
		m_noteEditMenu->addAction( act );
	}
	connect( signalMapper, SIGNAL(mapped(int)),
			this, SLOT(changeNoteEditMode(int)) );

	signalMapper = new QSignalMapper( this );
	m_semiToneMarkerMenu = new QMenu( this );

	QAction * act = new QAction( tr("Mark/unmark current semitone"), this );
	connect( act, SIGNAL(triggered()), signalMapper, SLOT(map()) );
	signalMapper->setMapping( act, static_cast<int>( stmaMarkCurrentSemiTone ) );
	m_semiToneMarkerMenu->addAction( act );

	act = new QAction( tr("Mark current scale"), this );
	act->setEnabled( false );
	connect( act, SIGNAL(triggered()), signalMapper, SLOT(map()) );
	connect( this, SIGNAL(semiToneMarkerMenuScaleSetEnabled(bool)), act, SLOT(setEnabled(bool)) );
	signalMapper->setMapping( act, static_cast<int>( stmaMarkCurrentScale ) );
	m_semiToneMarkerMenu->addAction( act );

	act = new QAction( tr("Mark current chord"), this );
	act->setEnabled( false );
	connect( act, SIGNAL(triggered()), signalMapper, SLOT(map()) );
	connect( this, SIGNAL(semiToneMarkerMenuChordSetEnabled(bool)), act, SLOT(setEnabled(bool)) );
	signalMapper->setMapping( act, static_cast<int>( stmaMarkCurrentChord ) );
	m_semiToneMarkerMenu->addAction( act );

	act = new QAction( tr("Unmark all"), this );
	connect( act, SIGNAL(triggered()), signalMapper, SLOT(map()) );
	signalMapper->setMapping( act, static_cast<int>( stmaUnmarkAll ) );
	m_semiToneMarkerMenu->addAction( act );

	connect( signalMapper, SIGNAL(mapped(int)),
			this, SLOT(markSemiTone(int)) );

	// init pixmaps
	if( s_whiteKeySmallPm == NULL )
	{
		s_whiteKeySmallPm = new QPixmap( embed::getIconPixmap(
						"pr_white_key_small" ) );
	}
	if( s_whiteKeySmallPressedPm == NULL )
	{
		s_whiteKeySmallPressedPm = new QPixmap( embed::getIconPixmap(
						"pr_white_key_small_pressed" ) );
	}
	if( s_whiteKeyBigPm == NULL )
	{
		s_whiteKeyBigPm = new QPixmap( embed::getIconPixmap(
							"pr_white_key_big" ) );
	}
	if( s_whiteKeyBigPressedPm == NULL )
	{
		s_whiteKeyBigPressedPm = new QPixmap( embed::getIconPixmap(
							"pr_white_key_big_pressed" ) );
	}
	if( s_blackKeyPm == NULL )
	{
		s_blackKeyPm = new QPixmap( embed::getIconPixmap(
							"pr_black_key" ) );
	}
	if( s_blackKeyPressedPm == NULL )
	{
		s_blackKeyPressedPm = new QPixmap( embed::getIconPixmap(
							"pr_black_key_pressed" ) );
	}
	if( s_toolDraw == NULL )
	{
		s_toolDraw = new QPixmap( embed::getIconPixmap(
							"edit_draw" ) );
	}
	if( s_toolErase == NULL )
	{
		s_toolErase= new QPixmap( embed::getIconPixmap(
							"edit_erase" ) );
	}
	if( s_toolSelect == NULL )
	{
		s_toolSelect = new QPixmap( embed::getIconPixmap(
							"edit_select" ) );
	}
	if( s_toolMove == NULL )
	{
		s_toolMove = new QPixmap( embed::getIconPixmap(
							"edit_move" ) );
	}
	if( s_toolOpen == NULL )
	{
		s_toolOpen = new QPixmap( embed::getIconPixmap(
							"automation" ) );
	}

	setAttribute( Qt::WA_OpaquePaintEvent, true );

	// add time-line
	m_timeLine = new timeLine( WHITE_KEY_WIDTH, 32, m_ppt,
					engine::getSong()->getPlayPos(
						song::Mode_PlayPattern ),
						m_currentPosition, this );
	connect( this, SIGNAL( positionChanged( const MidiTime & ) ),
		m_timeLine, SLOT( updatePosition( const MidiTime & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const MidiTime & ) ),
			this, SLOT( updatePosition( const MidiTime & ) ) );

	// update timeline when in record-accompany mode
	connect( engine::getSong()->getPlayPos( song::Mode_PlaySong ).m_timeLine,
				SIGNAL( positionChanged( const MidiTime & ) ),
			this,
			SLOT( updatePositionAccompany( const MidiTime & ) ) );
	// TODO
/*	connect( engine::getSong()->getPlayPos( song::Mode_PlayBB ).m_timeLine,
				SIGNAL( positionChanged( const MidiTime & ) ),
			this,
			SLOT( updatePositionAccompany( const MidiTime & ) ) );*/


	m_toolBar = new QWidget( this );
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->move( 0, 0 );
	m_toolBar->setAutoFillBackground( true );
	QPalette pal;
	pal.setBrush( m_toolBar->backgroundRole(),
					embed::getIconPixmap( "toolbar_bg" ) );
	m_toolBar->setPalette( pal );

	QHBoxLayout * tb_layout = new QHBoxLayout( m_toolBar );
	tb_layout->setMargin( 0 );
	tb_layout->setSpacing( 0 );


	// init control-buttons at the top

	m_playButton = new toolButton( embed::getIconPixmap( "play" ),
				tr( "Play/pause current pattern (Space)" ),
					this, SLOT( play() ), m_toolBar );

	m_recordButton = new toolButton( embed::getIconPixmap( "record" ),
			tr( "Record notes from MIDI-device/channel-piano" ),
					this, SLOT( record() ), m_toolBar );
	m_recordAccompanyButton = new toolButton(
			embed::getIconPixmap( "record_accompany" ),
			tr( "Record notes from MIDI-device/channel-piano while playing song or BB track" ),
					this, SLOT( recordAccompany() ), m_toolBar );

	m_stopButton = new toolButton( embed::getIconPixmap( "stop" ),
				tr( "Stop playing of current pattern (Space)" ),
					this, SLOT( stop() ), m_toolBar );

	m_playButton->setObjectName( "playButton" );
	m_stopButton->setObjectName( "stopButton" );
	m_recordButton->setObjectName( "recordButton" );
	m_recordAccompanyButton->setObjectName( "recordAccompanyButton" );

	m_playButton->setWhatsThis(
		tr( "Click here to play the current pattern. "
			"This is useful while editing it. The pattern is "
			"automatically looped when its end is reached." ) );
	m_recordButton->setWhatsThis(
		tr( "Click here to record notes from a MIDI-"
			"device or the virtual test-piano of the according "
			"channel-window to the current pattern. When recording "
			"all notes you play will be written to this pattern "
			"and you can play and edit them afterwards." ) );
	m_recordAccompanyButton->setWhatsThis(
		tr( "Click here to record notes from a MIDI-"
			"device or the virtual test-piano of the according "
			"channel-window to the current pattern. When recording "
			"all notes you play will be written to this pattern "
			"and you will hear the song or BB track in the background." ) );
	m_stopButton->setWhatsThis(
		tr( "Click here to stop playback of current pattern." ) );


	removeSelection();

	// init scrollbars
	m_leftRightScroll = new QScrollBar( Qt::Horizontal, this );
	m_leftRightScroll->setSingleStep( 1 );
	connect( m_leftRightScroll, SIGNAL( valueChanged( int ) ), this,
						SLOT( horScrolled( int ) ) );

	m_topBottomScroll = new QScrollBar( Qt::Vertical, this );
	m_topBottomScroll->setSingleStep( 1 );
	m_topBottomScroll->setPageStep( 20 );
	connect( m_topBottomScroll, SIGNAL( valueChanged( int ) ), this,
						SLOT( verScrolled( int ) ) );

	// init edit-buttons at the top
	m_drawButton = new toolButton( embed::getIconPixmap( "edit_draw" ),
					tr( "Draw mode (Shift+D)" ),
					this, SLOT( drawButtonToggled() ),
					m_toolBar );
	m_drawButton->setCheckable( true );
	m_drawButton->setChecked( true );

	m_eraseButton = new toolButton( embed::getIconPixmap( "edit_erase" ),
					tr( "Erase mode (Shift+E)" ),
					this, SLOT( eraseButtonToggled() ),
					m_toolBar );
	m_eraseButton->setCheckable( true );

	m_selectButton = new toolButton( embed::getIconPixmap(
							"edit_select" ),
					tr( "Select mode (Shift+S)" ),
					this, SLOT( selectButtonToggled() ),
					m_toolBar );
	m_selectButton->setCheckable( true );

	m_detuneButton = new toolButton( embed::getIconPixmap( "automation"),
					tr( "Detune mode (Shift+T)" ),
					this, SLOT( detuneButtonToggled() ),
					m_toolBar );
	m_detuneButton->setCheckable( true );

	QButtonGroup * tool_button_group = new QButtonGroup( this );
	tool_button_group->addButton( m_drawButton );
	tool_button_group->addButton( m_eraseButton );
	tool_button_group->addButton( m_selectButton );
	tool_button_group->addButton( m_detuneButton );
	tool_button_group->setExclusive( true );

	m_drawButton->setWhatsThis(
		tr( "Click here and draw mode will be activated. In this "
			"mode you can add, resize and move notes. This "
			"is the default mode which is used most of the time. "
			"You can also press 'Shift+D' on your keyboard to "
			"activate this mode. In this mode, hold Ctrl to "
		    "temporarily go into select mode." ) );
	m_eraseButton->setWhatsThis(
		tr( "Click here and erase mode will be activated. In this "
			"mode you can erase notes. You can also press "
			"'Shift+E' on your keyboard to activate this mode." ) );
	m_selectButton->setWhatsThis(
		tr( "Click here and select mode will be activated. "
			"In this mode you can select notes. Alternatively, "
		    "you can hold Ctrl in draw mode to temporarily use "
		    "select mode." ) );
	m_detuneButton->setWhatsThis(
		tr( "Click here and detune mode will be activated. "
			"In this mode you can click a note to open its "
		    "automation detuning. You can utilize this to slide "
		    "notes from one to another. You can also press "
		    "'Shift+T' on your keyboard to activate this mode." ) );

	m_cutButton = new toolButton( embed::getIconPixmap( "edit_cut" ),
					tr( "Cut selected notes (Ctrl+X)" ),
					this, SLOT( cutSelectedNotes() ),
					m_toolBar );

	m_copyButton = new toolButton( embed::getIconPixmap( "edit_copy" ),
					tr( "Copy selected notes (Ctrl+C)" ),
					this, SLOT( copySelectedNotes() ),
					m_toolBar );

	m_pasteButton = new toolButton( embed::getIconPixmap( "edit_paste" ),
					tr( "Paste notes from clipboard "
								"(Ctrl+V)" ),
					this, SLOT( pasteNotes() ),
					m_toolBar );

	m_cutButton->setWhatsThis(
		tr( "Click here and the selected notes will be cut into the "
			"clipboard. You can paste them anywhere in any pattern "
			"by clicking on the paste button." ) );
	m_copyButton->setWhatsThis(
		tr( "Click here and the selected notes will be copied into the "
			"clipboard. You can paste them anywhere in any pattern "
			"by clicking on the paste button." ) );
	m_pasteButton->setWhatsThis(
		tr( "Click here and the notes from the clipboard will be "
			"pasted at the first visible measure." ) );



	QLabel * zoom_lbl = new QLabel( m_toolBar );
	zoom_lbl->setPixmap( embed::getIconPixmap( "zoom" ) );

	// setup zooming-stuff
	for( int i = 0; i < 6; ++i )
	{
		m_zoomingModel.addItem( QString::number( 25 << i ) + "%" );
	}
	m_zoomingModel.setValue( m_zoomingModel.findText( "100%" ) );
	connect( &m_zoomingModel, SIGNAL( dataChanged() ),
					this, SLOT( zoomingChanged() ) );
	m_zoomingComboBox = new comboBox( m_toolBar );
	m_zoomingComboBox->setModel( &m_zoomingModel );
	m_zoomingComboBox->setFixedSize( 64, 22 );

	// setup quantize-stuff
	QLabel * quantize_lbl = new QLabel( m_toolBar );
	quantize_lbl->setPixmap( embed::getIconPixmap( "quantize" ) );

	m_quantizeModel.addItem( tr( "Note lock" ) );
	for( int i = 0; i <= NUM_EVEN_LENGTHS; ++i )
	{
		m_quantizeModel.addItem( "1/" + QString::number( 1 << i ) );
	}
	for( int i = 0; i < NUM_TRIPLET_LENGTHS; ++i )
	{
		m_quantizeModel.addItem( "1/" + QString::number( (1 << i) * 3 ) );
	}
	m_quantizeModel.addItem( "1/192" );
	m_quantizeModel.setValue( m_quantizeModel.findText( "1/16" ) );
	m_quantizeComboBox = new comboBox( m_toolBar );
	m_quantizeComboBox->setModel( &m_quantizeModel );
	m_quantizeComboBox->setFixedSize( 64, 22 );
	connect( &m_quantizeModel, SIGNAL( dataChanged() ),
					this, SLOT( quantizeChanged() ) );


	// setup note-len-stuff
	QLabel * note_len_lbl = new QLabel( m_toolBar );
	note_len_lbl->setPixmap( embed::getIconPixmap( "note" ) );

	m_noteLenModel.addItem( tr( "Last note" ),
					new PixmapLoader( "edit_draw" ) );
	const QString pixmaps[] = { "whole", "half", "quarter", "eighth",
						"sixteenth", "thirtysecond", "triplethalf",
						"tripletquarter", "tripleteighth",
						"tripletsixteenth", "tripletthirtysecond" } ;

	for( int i = 0; i < NUM_EVEN_LENGTHS; ++i )
	{
		m_noteLenModel.addItem( "1/" + QString::number( 1 << i ),
				new PixmapLoader( "note_" + pixmaps[i] ) );
	}
	for( int i = 0; i < NUM_TRIPLET_LENGTHS; ++i )
	{
		m_noteLenModel.addItem( "1/" + QString::number( (1 << i) * 3 ),
				new PixmapLoader( "note_" + pixmaps[i+NUM_EVEN_LENGTHS] ) );
	}
	m_noteLenModel.setValue( 0 );
	m_noteLenComboBox = new comboBox( m_toolBar );
	m_noteLenComboBox->setModel( &m_noteLenModel );
	m_noteLenComboBox->setFixedSize( 105, 22 );
	// Note length change can cause a redraw if Q is set to lock
	connect( &m_noteLenModel, SIGNAL( dataChanged() ),
					this, SLOT( quantizeChanged() ) );


	const InstrumentFunctionNoteStacking::ChordTable & chord_table = InstrumentFunctionNoteStacking::ChordTable::getInstance();

	// setup scale-stuff
	QLabel * scale_lbl = new QLabel( m_toolBar );
	scale_lbl->setPixmap( embed::getIconPixmap( "scale" ) );

	m_scaleModel.addItem( tr("No scale") );
	for( int i = 0; i < chord_table.size(); ++i )
	{
		if( chord_table[i].isScale() )
		{
			m_scaleModel.addItem( chord_table[i].getName() );
		}
	}

	m_scaleModel.setValue( 0 );
	m_scaleComboBox = new comboBox( m_toolBar );
	m_scaleComboBox->setModel( &m_scaleModel );
	m_scaleComboBox->setFixedSize( 105, 22 );
	// change can update m_semiToneMarkerMenu
	connect( &m_scaleModel, SIGNAL( dataChanged() ),
					this, SLOT( updateSemiToneMarkerMenu() ) );


	// setup chord-stuff
	QLabel * chord_lbl = new QLabel( m_toolBar );
	chord_lbl->setPixmap( embed::getIconPixmap( "chord" ) );

	m_chordModel.addItem( tr("No chord") );
	for( int i = 0; i < chord_table.size(); ++i )
	{
		if( ! chord_table[i].isScale() )
		{
			m_chordModel.addItem( chord_table[i].getName() );
		}
	}

	m_chordModel.setValue( 0 );
	m_chordComboBox = new comboBox( m_toolBar );
	m_chordComboBox->setModel( &m_chordModel );
	m_chordComboBox->setFixedSize( 105, 22 );
	// change can update m_semiToneMarkerMenu
	connect( &m_chordModel, SIGNAL( dataChanged() ),
					this, SLOT( updateSemiToneMarkerMenu() ) );


	tb_layout->addSpacing( 4 );
	tb_layout->addWidget( m_playButton );
	tb_layout->addWidget( m_recordButton );
	tb_layout->addWidget( m_recordAccompanyButton );
	tb_layout->addWidget( m_stopButton );
	tb_layout->addSpacing( 7 );
	tb_layout->addWidget( m_drawButton );
	tb_layout->addWidget( m_eraseButton );
	tb_layout->addWidget( m_selectButton );
	tb_layout->addWidget( m_detuneButton );
	tb_layout->addSpacing( 7 );
	tb_layout->addWidget( m_cutButton );
	tb_layout->addWidget( m_copyButton );
	tb_layout->addWidget( m_pasteButton );
	tb_layout->addSpacing( 7 );
	m_timeLine->addToolButtons( m_toolBar );
	tb_layout->addSpacing( 7 );
	tb_layout->addWidget( zoom_lbl );
	tb_layout->addSpacing( 4 );
	tb_layout->addWidget( m_zoomingComboBox );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( quantize_lbl );
	tb_layout->addSpacing( 4 );
	tb_layout->addWidget( m_quantizeComboBox );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( note_len_lbl );
	tb_layout->addSpacing( 4 );
	tb_layout->addWidget( m_noteLenComboBox );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( scale_lbl );
	tb_layout->addSpacing( 4 );
	tb_layout->addWidget( m_scaleComboBox );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( chord_lbl );
	tb_layout->addSpacing( 4 );
	tb_layout->addWidget( m_chordComboBox );
	tb_layout->addStretch();

	// setup our actual window
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	setWindowIcon( embed::getIconPixmap( "piano" ) );
	setCurrentPattern( NULL );

	setMouseTracking( true );

	setMinimumSize( tb_layout->minimumSize().width(), 160 );

	// add us to workspace
	if( engine::mainWindow()->workspace() )
	{
		engine::mainWindow()->workspace()->addSubWindow( this );
		parentWidget()->setMinimumSize( tb_layout->minimumSize().width()+10, 200 );
		parentWidget()->resize( tb_layout->minimumSize().width()+10,
						INITIAL_PIANOROLL_HEIGHT );
		parentWidget()->move( 5, 5 );

		parentWidget()->hide();
	}
	else
	{
		resize( tb_layout->minimumSize().width(), INITIAL_PIANOROLL_HEIGHT );
		hide();
	}

	connect( engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
						this, SLOT( update() ) );
}



void PianoRoll::reset()
{
	m_lastNoteVolume = DefaultVolume;
	m_lastNotePanning = DefaultPanning;
}



void PianoRoll::changeNoteEditMode( int i )
{
	m_noteEditMode = (noteEditMode) i;
	repaint();
}


void PianoRoll::markSemiTone( int i )
{
	const int key = getKey( mapFromGlobal( m_semiToneMarkerMenu->pos() ).y() );
	const InstrumentFunctionNoteStacking::Chord * chord = 0;

	switch( static_cast<semiToneMarkerAction>( i ) )
	{
		case stmaUnmarkAll:
			m_markedSemiTones.clear();
			break;
		case stmaMarkCurrentSemiTone:
		{
			QList<int>::iterator i = qFind( m_markedSemiTones.begin(), m_markedSemiTones.end(), key );
			if( i != m_markedSemiTones.end() )
			{
				m_markedSemiTones.erase( i );
			}
			else
			{
				m_markedSemiTones.push_back( key );
			}
			break;
		}
		case stmaMarkCurrentScale:
			chord = & InstrumentFunctionNoteStacking::ChordTable::getInstance()
					.getScaleByName( m_scaleModel.currentText() );
		case stmaMarkCurrentChord:
		{
			if( ! chord )
			{
				chord = & InstrumentFunctionNoteStacking::ChordTable::getInstance()
						.getChordByName( m_chordModel.currentText() );
			}

			if( chord->isEmpty() )
			{
				break;
			}
			else if( chord->isScale() )
			{
				m_markedSemiTones.clear();
			}

			const int first = chord->isScale() ? 0 : key;
			const int last = chord->isScale() ? NumKeys : key + chord->last();
			const int cap = ( chord->isScale() || chord->last() == 0 ) ? KeysPerOctave : chord->last();

			for( int i = first; i <= last; i++ )
			{
			  //if( chord->hasSemiTone( std::abs( key - i ) % cap ) )
			  if( chord->hasSemiTone( ( i + cap - ( key % cap ) ) % cap ) )
				{
					m_markedSemiTones.push_back( i );
				}
			}
			break;
		}
		default:
			;
	}

	qSort( m_markedSemiTones.begin(), m_markedSemiTones.end(), qGreater<int>() );
	QList<int>::iterator new_end = std::unique( m_markedSemiTones.begin(), m_markedSemiTones.end() );
	m_markedSemiTones.erase( new_end, m_markedSemiTones.end() );
}


PianoRoll::~PianoRoll()
{
}


void PianoRoll::setCurrentPattern( pattern * _new_pattern )
{
	if( validPattern() )
	{
		m_pattern->instrumentTrack()->disconnect( this );
	}

	m_pattern = _new_pattern;
	m_currentPosition = 0;
	m_currentNote = NULL;
	m_startKey = INITIAL_START_KEY;

	if( validPattern() == false )
	{
		//resizeEvent( NULL );
		setWindowTitle( tr( "Piano-Roll - no pattern" ) );

		update();
		emit currentPatternChanged();
		return;
	}

	m_leftRightScroll->setValue( 0 );

	const NoteVector & notes = m_pattern->notes();
	int central_key = 0;
	if( notes.empty() == false )
	{
		// determine the central key so that we can scroll to it
		int total_notes = 0;
		for( NoteVector::ConstIterator it = notes.begin();
						it != notes.end(); ++it )
		{
			if( ( *it )->length() > 0 )
			{
				central_key += ( *it )->key();
				++total_notes;
			}
		}

		if( total_notes > 0 )
		{
			central_key = central_key / total_notes -
					( KeysPerOctave * NumOctaves -
						m_totalKeysToScroll ) / 2;
			m_startKey = tLimit( central_key, 0,
						NumOctaves * KeysPerOctave );
		}
	}
	// resizeEvent() does the rest for us (scrolling, range-checking
	// of start-notes and so on...)
	resizeEvent( NULL );

	connect( m_pattern->instrumentTrack(), SIGNAL( midiNoteOn( const note& ) ), this, SLOT( startRecordNote( const note& ) ) );
	connect( m_pattern->instrumentTrack(), SIGNAL( midiNoteOff( const note& ) ), this, SLOT( finishRecordNote( const note& ) ) );
	connect( m_pattern->instrumentTrack()->pianoModel(), SIGNAL( dataChanged() ), this, SLOT( update() ) );

	setWindowTitle( tr( "Piano-Roll - %1" ).arg( m_pattern->name() ) );

	update();
	emit currentPatternChanged();
}




void PianoRoll::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void PianoRoll::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}




void PianoRoll::setPauseIcon( bool pause )
{
	if( pause == true )
	{
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
	}
	else
	{
		m_playButton->setIcon( embed::getIconPixmap( "play" ) );
	}
}




inline void PianoRoll::drawNoteRect( QPainter & _p, int _x, int _y,
					int _width, note * _n )
{
	++_x;
	++_y;
	_width -= 2;

	if( _width <= 0 )
	{
		_width = 2;
	}

	int volVal = qMin( 255, (int) (
			( (float)( _n->getVolume() - MinVolume ) ) /
			( (float)( MaxVolume - MinVolume ) ) * 255.0f) );
	float rightPercent = qMin<float>( 1.0f,
			( (float)( _n->getPanning() - PanningLeft ) ) /
			( (float)( PanningRight - PanningLeft ) ) * 2.0f );

	float leftPercent = qMin<float>( 1.0f,
			( (float)( PanningRight - _n->getPanning() ) ) /
			( (float)( PanningRight - PanningLeft ) ) * 2.0f );

	const QColor defaultNoteColor( 0x77, 0xC7, 0xD8 );
	QColor col = defaultNoteColor;

	if( _n->length() < 0 )
	{
		//step note
		col.setRgb( 0, 255, 0 );
		_p.fillRect( _x, _y, _width, KEY_LINE_HEIGHT - 2, col );
	}
	else if( _n->selected() )
	{
		col.setRgb( 0x00, 0x40, 0xC0 );
		_p.fillRect( _x, _y, _width, KEY_LINE_HEIGHT - 2, col );
	}
	else
	{
		// adjust note to make it a bit faded if it has a lower volume
		// in stereo using gradients
		QColor lcol = QColor::fromHsv( col.hue(), col.saturation(),
							volVal * leftPercent );
		QColor rcol = QColor::fromHsv( col.hue(), col.saturation(),
							volVal * rightPercent );
		col = QColor::fromHsv( col.hue(), col.saturation(), volVal );

		QLinearGradient gradient( _x, _y, _x+_width,
							_y+KEY_LINE_HEIGHT );
		gradient.setColorAt( 0, lcol );
		gradient.setColorAt( 1, rcol );
		_p.setBrush( gradient );
		_p.setPen( Qt::NoPen );
		_p.drawRect( _x, _y, _width, KEY_LINE_HEIGHT-1 );
	}

	// hilighting lines around the note
	_p.setPen( Qt::SolidLine );
	_p.setBrush( Qt::NoBrush );

	col = defaultNoteColor;
	_p.setPen( QColor::fromHsv( col.hue(), col.saturation(),
					qMin<float>( 255, volVal*1.7f ) ) );
	_p.drawLine( _x, _y, _x + _width, _y );
	_p.drawLine( _x, _y, _x, _y + KEY_LINE_HEIGHT - 2 );

	col = defaultNoteColor;
	_p.setPen( QColor::fromHsv( col.hue(), col.saturation(), volVal/1.7 ) );
	_p.drawLine( _x + _width, _y, _x + _width, _y + KEY_LINE_HEIGHT - 2 );
	_p.drawLine( _x, _y + KEY_LINE_HEIGHT - 2, _x + _width,
						_y + KEY_LINE_HEIGHT - 2 );

	// that little tab thing on the end hinting at the user
	// to resize the note
	_p.setPen( defaultNoteColor.lighter( 200 ) );
	if( _width > 2 )
	{
		_p.drawLine( _x + _width - 3, _y + 2, _x + _width - 3,
						_y + KEY_LINE_HEIGHT - 4 );
	}
	_p.drawLine( _x + _width - 1, _y + 2, _x + _width - 1,
						_y + KEY_LINE_HEIGHT - 4 );
	_p.drawLine( _x + _width - 2, _y + 2, _x + _width - 2,
						_y + KEY_LINE_HEIGHT - 4 );
}




inline void PianoRoll::drawDetuningInfo( QPainter & _p, note * _n, int _x,
								int _y )
{
	int middle_y = _y + KEY_LINE_HEIGHT / 2;
	_p.setPen( QColor( 0x99, 0xAF, 0xFF ) );

	int old_x = 0;
	int old_y = 0;

	timeMap & map = _n->detuning()->automationPattern()->getTimeMap();
	for( timeMap::ConstIterator it = map.begin(); it != map.end(); ++it )
	{
		int pos_ticks = it.key();
		if( pos_ticks > _n->length() )
		{
			break;
		}
		int pos_x = _x + pos_ticks * m_ppt / MidiTime::ticksPerTact();

		const float level = it.value();

		int pos_y = (int)( middle_y - level * KEY_LINE_HEIGHT );

		if( old_x != 0 && old_y != 0 )
		{
			switch( _n->detuning()->automationPattern()->progressionType() )
			{
			case AutomationPattern::DiscreteProgression:
				_p.drawLine( old_x, old_y, pos_x, old_y );
				_p.drawLine( pos_x, old_y, pos_x, pos_y );
				break;
			case AutomationPattern::CubicHermiteProgression: /* TODO */
			case AutomationPattern::LinearProgression:
				_p.drawLine( old_x, old_y, pos_x, pos_y );
				break;
			}
		}

		_p.drawLine( pos_x - 1, pos_y, pos_x + 1, pos_y );
		_p.drawLine( pos_x, pos_y - 1, pos_x, pos_y + 1 );

		old_x = pos_x;
		old_y = pos_y;
	}
}




void PianoRoll::removeSelection()
{
	m_selectStartTick = 0;
	m_selectedTick = 0;
	m_selectStartKey = 0;
	m_selectedKeys = 0;


}




void PianoRoll::clearSelectedNotes()
{
	if( m_pattern != NULL )
	{
		// get note-vector of current pattern
		const NoteVector & notes = m_pattern->notes();

		// will be our iterator in the following loop
		NoteVector::ConstIterator it = notes.begin();
		while( it != notes.end() )
		{
			( *it )->setSelected( false );

			++it;
		}
	}
}




void PianoRoll::closeEvent( QCloseEvent * _ce )
{
	QApplication::restoreOverrideCursor();
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	_ce->ignore();
}




void PianoRoll::shiftSemiTone( int amount ) // shift notes by amount semitones
{
	bool useAllNotes = ! isSelection();
	const NoteVector & notes = m_pattern->notes();
	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end();
									++it )
	{
		// if none are selected, move all notes, otherwise
		// only move selected notes
		if( useAllNotes || ( *it )->selected() )
		{
			( *it )->setKey( ( *it )->key() + amount );
		}
	}

	// we modified the song
	update();
	engine::songEditor()->update();

}




void PianoRoll::shiftPos( int amount ) //shift notes pos by amount
{
	bool useAllNotes = ! isSelection();
	const NoteVector & notes = m_pattern->notes();

	bool first = true;
	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end();
									++it )
	{
		// if none are selected, move all notes, otherwise
		// only move selected notes
		if( ( *it )->selected() || (useAllNotes && ( *it )->length() > 0) )
		{
			// don't let notes go to out of bounds
			if( first )
			{
				m_moveBoundaryLeft = ( *it )->pos();
				if( m_moveBoundaryLeft + amount < 0 )
				{
					amount += 0 - (amount + m_moveBoundaryLeft);
				}
				first = false;
			}
			( *it )->setPos( ( *it )->pos() + amount );
		}
	}

	// we modified the song
	update();
	engine::songEditor()->update();
}




bool PianoRoll::isSelection() const // are any notes selected?
{
	const NoteVector & notes = m_pattern->notes();
	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end(); ++it )
	{
		if( ( *it )->selected() )
		{
			return true;
		}
	}

	return false;
}



int PianoRoll::selectionCount() const // how many notes are selected?
{
	int sum = 0;

	const NoteVector & notes = m_pattern->notes();
	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end(); ++it )
	{
		if( ( *it )->selected() )
		{
			++sum;
		}
	}

	return sum;
}



void PianoRoll::keyPressEvent( QKeyEvent* event )
{
	if( validPattern() && event->modifiers() == Qt::NoModifier )
	{
		const int key_num = PianoView::getKeyFromKeyEvent( event ) + ( DefaultOctave - 1 ) * KeysPerOctave;

		if( event->isAutoRepeat() == false && key_num > -1 )
		{
			m_pattern->instrumentTrack()->pianoModel()->handleKeyPress( key_num );
			event->accept();
		}
	}

	switch( event->key() )
	{
		case Qt::Key_Up:
			if( ( event->modifiers() & Qt::ControlModifier ) && m_action == ActionNone )
			{
				// shift selection up an octave
				// if nothing selected, shift _everything_
				shiftSemiTone( +12 );
			}
			else
			{
				// scroll
				m_topBottomScroll->setValue(
						m_topBottomScroll->value() -
							cm_scrollAmtVert );

				// if they are moving notes around or resizing,
				// recalculate the note/resize position
				if( m_action == ActionMoveNote ||
						m_action == ActionResizeNote )
				{
					dragNotes( m_lastMouseX, m_lastMouseY,
								event->modifiers() & Qt::AltModifier,
								event->modifiers() & Qt::ShiftModifier );
				}
			}
			event->accept();
			break;

		case Qt::Key_Down:
			if( event->modifiers() & Qt::ControlModifier && m_action == ActionNone )
			{
				// shift selection down an octave
				// if nothing selected, shift _everything_
				shiftSemiTone( -12 );
			}
			else
			{
				// scroll
				m_topBottomScroll->setValue(
						m_topBottomScroll->value() +
							cm_scrollAmtVert );

				// if they are moving notes around or resizing,
				// recalculate the note/resize position
				if( m_action == ActionMoveNote ||
						m_action == ActionResizeNote )
				{
					dragNotes( m_lastMouseX, m_lastMouseY,
								event->modifiers() & Qt::AltModifier,
								event->modifiers() & Qt::ShiftModifier );
				}
			}
			event->accept();
			break;

		case Qt::Key_Left:
			if( event->modifiers() & Qt::ControlModifier && m_action == ActionNone )
			{
				// move time ticker
				if( ( m_timeLine->pos() -= 16 ) < 0 )
				{
					m_timeLine->pos().setTicks( 0 );
				}
				m_timeLine->updatePosition();
			}
			else if( event->modifiers() & Qt::ShiftModifier && m_action == ActionNone)
			{
				// move notes
				bool quantized = ! ( event->modifiers() & Qt::AltModifier );
				int amt = quantized ? quantization() : 1;
				shiftPos( -amt );
			}
			else
			{
				// scroll
				m_leftRightScroll->setValue(
						m_leftRightScroll->value() -
							cm_scrollAmtHoriz );

				// if they are moving notes around or resizing,
				// recalculate the note/resize position
				if( m_action == ActionMoveNote ||
						m_action == ActionResizeNote )
				{
					dragNotes( m_lastMouseX, m_lastMouseY,
								event->modifiers() & Qt::AltModifier,
								event->modifiers() & Qt::ShiftModifier );
				}

			}
			event->accept();
			break;

		case Qt::Key_Right:
			if( event->modifiers() & Qt::ControlModifier && m_action == ActionNone)
			{
				// move time ticker
				m_timeLine->pos() += 16;
				m_timeLine->updatePosition();
			}
			else if( event->modifiers() & Qt::ShiftModifier && m_action == ActionNone)
			{
				// move notes
				bool quantized = !( event->modifiers() & Qt::AltModifier );
				int amt = quantized ? quantization() : 1;
				shiftPos( +amt );
			}
			else
			{
				// scroll
				m_leftRightScroll->setValue(
						m_leftRightScroll->value() +
							cm_scrollAmtHoriz );

				// if they are moving notes around or resizing,
				// recalculate the note/resize position
				if( m_action == ActionMoveNote ||
						m_action == ActionResizeNote )
				{
					dragNotes( m_lastMouseX, m_lastMouseY,
								event->modifiers() & Qt::AltModifier,
								event->modifiers() & Qt::ShiftModifier );
				}

			}
			event->accept();
			break;

		case Qt::Key_C:
			if( event->modifiers() & Qt::ControlModifier )
			{
				event->accept();
				copySelectedNotes();
			}
			break;

		case Qt::Key_X:
			if( event->modifiers() & Qt::ControlModifier )
			{
				event->accept();
				cutSelectedNotes();
			}
			break;

		case Qt::Key_V:
			if( event->modifiers() & Qt::ControlModifier )
			{
				event->accept();
				pasteNotes();
			}
			break;

		case Qt::Key_A:
			if( event->modifiers() & Qt::ControlModifier )
			{
				event->accept();
				m_selectButton->setChecked( true );
				selectAll();
				update();
			}
			break;

		case Qt::Key_D:
			if( event->modifiers() & Qt::ShiftModifier )
			{
				event->accept();
				m_drawButton->setChecked( true );
			}
			break;

		case Qt::Key_E:
			if( event->modifiers() & Qt::ShiftModifier )
			{
				event->accept();
				m_eraseButton->setChecked( true );
			}
			break;

		case Qt::Key_S:
			if( event->modifiers() & Qt::ShiftModifier )
			{
				event->accept();
				m_selectButton->setChecked( true );
			}
			break;

		case Qt::Key_T:
			if( event->modifiers() & Qt::ShiftModifier )
			{
				event->accept();
				m_detuneButton->setChecked( true );
			}
			break;

		case Qt::Key_Delete:
			deleteSelectedNotes();
			event->accept();
			break;

		case Qt::Key_Space:
			if( engine::getSong()->isPlaying() )
			{
				stop();
			}
			else
			{
				play();
			}
			event->accept();
			break;

		case Qt::Key_Home:
			m_timeLine->pos().setTicks( 0 );
			m_timeLine->updatePosition();
			event->accept();
			break;

		case Qt::Key_0:
		case Qt::Key_1:
		case Qt::Key_2:
		case Qt::Key_3:
		case Qt::Key_4:
		case Qt::Key_5:
		case Qt::Key_6:
		case Qt::Key_7:
		case Qt::Key_8:
		case Qt::Key_9:
		{
			int len = 1 + event->key() - Qt::Key_0;
			if( len == 10 )
			{
				len = 0;
			}
			if( event->modifiers() & ( Qt::ControlModifier | Qt::KeypadModifier ) )
			{
				m_noteLenModel.setValue( len );
				event->accept();
			}
			else if( event->modifiers() & Qt::AltModifier )
			{
				m_quantizeModel.setValue( len );
				event->accept();
			}
			break;
		}

		case Qt::Key_Control:
			m_ctrlMode = m_editMode;
			m_editMode = ModeSelect;
			QApplication::changeOverrideCursor( Qt::ArrowCursor );
			event->accept();
			break;
		default:
			break;
	}

	update();
}




void PianoRoll::keyReleaseEvent( QKeyEvent* event )
{
	if( validPattern() && event->modifiers() == Qt::NoModifier )
	{
		const int key_num = PianoView::getKeyFromKeyEvent( event ) + ( DefaultOctave - 1 ) * KeysPerOctave;

		if( event->isAutoRepeat() == false && key_num > -1 )
		{
			m_pattern->instrumentTrack()->pianoModel()->handleKeyRelease( key_num );
			event->accept();
		}
	}

	switch( event->key() )
	{
		case Qt::Key_Control:
			computeSelectedNotes( event->modifiers() & Qt::ShiftModifier);
			m_editMode = m_ctrlMode;
			update();
			break;
	}

	update();
}




void PianoRoll::leaveEvent( QEvent * _e )
{
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}

	QWidget::leaveEvent( _e );
}




inline int PianoRoll::noteEditTop() const
{
	return height() - PR_BOTTOM_MARGIN -
		m_notesEditHeight + NOTE_EDIT_RESIZE_BAR;
}




inline int PianoRoll::noteEditBottom() const
{
	return height() - PR_BOTTOM_MARGIN;
}




inline int PianoRoll::noteEditRight() const
{
	return width() - PR_RIGHT_MARGIN;
}




inline int PianoRoll::noteEditLeft() const
{
	return WHITE_KEY_WIDTH;
}




inline int PianoRoll::keyAreaTop() const
{
	return PR_TOP_MARGIN;
}




inline int PianoRoll::keyAreaBottom() const
{
	return height() - PR_BOTTOM_MARGIN - m_notesEditHeight;
}




void PianoRoll::mousePressEvent( QMouseEvent * _me )
{
	if( validPattern() == false )
	{
		return;
	}

	if( m_editMode == ModeEditDetuning && noteUnderMouse() )
	{
		noteUnderMouse()->editDetuningPattern();
		return;
	}

	// if holding control, go to selection mode
	if( _me->modifiers() & Qt::ControlModifier && m_editMode != ModeSelect )
	{
		m_ctrlMode = m_editMode;
		m_editMode = ModeSelect;
		QApplication::changeOverrideCursor( QCursor( Qt::ArrowCursor ) );
		update();
	}

	// keep track of the point where the user clicked down
	if( _me->button() == Qt::LeftButton )
	{
		m_moveStartX = _me->x();
		m_moveStartY = _me->y();
	}

	if( _me->y() > keyAreaBottom() && _me->y() < noteEditTop() )
	{
		// resizing the note edit area
		m_action = ActionResizeNoteEditArea;
		m_oldNotesEditHeight = m_notesEditHeight;
		return;
	}

	if( _me->y() > PR_TOP_MARGIN )
	{
		bool edit_note = ( _me->y() > noteEditTop() );

		int key_num = getKey( _me->y() );

		int x = _me->x();


		if( x > WHITE_KEY_WIDTH )
		{
			// set, move or resize note

			x -= WHITE_KEY_WIDTH;

			// get tick in which the user clicked
			int pos_ticks = x * MidiTime::ticksPerTact() / m_ppt +
							m_currentPosition;


			// get note-vector of current pattern
			const NoteVector & notes = m_pattern->notes();

			// will be our iterator in the following loop
			NoteVector::ConstIterator it = notes.begin()+notes.size()-1;

			// loop through whole note-vector...
			for( int i = 0; i < notes.size(); ++i )
			{
				MidiTime len = ( *it )->length();
				if( len < 0 )
				{
					len = 4;
				}
				// and check whether the user clicked on an
				// existing note or an edit-line
				if( pos_ticks >= ( *it )->pos() &&
						len > 0 &&
					(
					( edit_note == false &&
					pos_ticks <= ( *it )->pos() + len &&
					( *it )->key() == key_num )
					||
					( edit_note == true &&
					pos_ticks <= ( *it )->pos() +
							NE_LINE_WIDTH *
						MidiTime::ticksPerTact() /
								m_ppt )
					)
					)
				{
					break;
				}
				--it;
			}

			// first check whether the user clicked in note-edit-
			// area
			if( edit_note == true )
			{
				// scribble note edit changes
				mouseMoveEvent( _me );
				return;
			}
			// left button??
			else if( _me->button() == Qt::LeftButton &&
							m_editMode == ModeDraw )
			{
				// whether this action creates new note(s) or not
				bool is_new_note = false;

				note * created_new_note = NULL;
				// did it reach end of vector because
				// there's no note??
				if( it == notes.begin()-1 )
				{
					is_new_note = true;
					m_pattern->setType( pattern::MelodyPattern );

					// then set new note

					// clear selection and select this new note
					clearSelectedNotes();

					// +32 to quanitize the note correctly when placing notes with
					// the mouse.  We do this here instead of in note.quantized
					// because live notes should still be quantized at the half.
					MidiTime note_pos( pos_ticks - ( quantization() / 2 ) );
					MidiTime note_len( newNoteLen() );

					note new_note( note_len, note_pos, key_num );
					new_note.setSelected( true );
					new_note.setPanning( m_lastNotePanning );
					new_note.setVolume( m_lastNoteVolume );
					created_new_note = m_pattern->addNote( new_note );

					const InstrumentFunctionNoteStacking::Chord & chord = InstrumentFunctionNoteStacking::ChordTable::getInstance()
						.getChordByName( m_chordModel.currentText() );

					if( ! chord.isEmpty() )
					{
						// if a chord is selected, create following notes in chord
						// or arpeggio mode
						const bool arpeggio = _me->modifiers() & Qt::ShiftModifier;
						for( int i = 1; i < chord.size(); i++ )
						{
							if( arpeggio )
							{
								note_pos += note_len;
							}
							note new_note( note_len, note_pos, key_num + chord[i] );
							new_note.setSelected( true );
							new_note.setPanning( m_lastNotePanning );
							new_note.setVolume( m_lastNoteVolume );
							m_pattern->addNote( new_note );
						}
					}

					// reset it so that it can be used for
					// ops (move, resize) after this
					// code-block
					it = notes.begin();
					while( it != notes.end() &&
						*it != created_new_note )
					{
						++it;
					}


				}

				m_currentNote = *it;
				m_lastNotePanning = ( *it )->getPanning();
				m_lastNoteVolume = ( *it )->getVolume();
				m_lenOfNewNotes = ( *it )->length();

				// remember which key and tick we started with
				m_mouseDownKey = m_startKey;
				m_mouseDownTick = m_currentPosition;

				bool first = true;
				it = notes.begin();
				while( it != notes.end() )
				{

					// remember note starting positions
					( *it )->setOldKey( ( *it )->key() );
					( *it )->setOldPos( ( *it )->pos() );
					( *it )->setOldLength( ( *it )->length() );

					if( ( *it )->selected() )
					{

						// figure out the bounding box of all the selected notes
						if( first )
						{
							m_moveBoundaryLeft = ( *it )->pos().getTicks();
							m_moveBoundaryRight = ( *it )->pos() + ( *it )->length();
							m_moveBoundaryBottom = ( *it )->key();
							m_moveBoundaryTop = ( *it )->key();

							first = false;
						}
						else
						{
							m_moveBoundaryLeft = qMin(
												( *it )->pos().getTicks(),
												m_moveBoundaryLeft );
							m_moveBoundaryRight = qMax( ( *it )->pos() +
												   ( *it )->length(),
													m_moveBoundaryRight );
							m_moveBoundaryBottom = qMin( ( *it )->key(),
											   m_moveBoundaryBottom );
							m_moveBoundaryTop = qMax( ( *it )->key(),
														m_moveBoundaryTop );
						}
					}

					++it;
				}

				// if clicked on an unselected note, remove selection
				// and select that new note
				if( ! m_currentNote->selected() )
				{
					clearSelectedNotes();
					m_currentNote->setSelected( true );
					m_moveBoundaryLeft = m_currentNote->pos().getTicks();
					m_moveBoundaryRight = m_currentNote->pos() + m_currentNote->length();
					m_moveBoundaryBottom = m_currentNote->key();
					m_moveBoundaryTop = m_currentNote->key();
				}


				// clicked at the "tail" of the note?
				if( pos_ticks*m_ppt/MidiTime::ticksPerTact() >
					( m_currentNote->pos() + m_currentNote->length() )*m_ppt/ MidiTime::ticksPerTact() - RESIZE_AREA_WIDTH &&
						m_currentNote->length() > 0 )
				{
					// then resize the note
					m_action = ActionResizeNote;

					// set resize-cursor
					QCursor c( Qt::SizeHorCursor );
					QApplication::setOverrideCursor( c );
				}
				else
				{
					// otherwise move it
					m_action = ActionMoveNote;

					// set move-cursor
					QCursor c( Qt::SizeAllCursor );
					QApplication::setOverrideCursor( c );

					// if they're holding shift, copy all selected notes
					if( //*it != created_new_note &&
					   ! is_new_note && _me->modifiers() & Qt::ShiftModifier )
					{
						// vector to hold new notes until we're through the loop
						QVector<note> newNotes;
						it = notes.begin();
						while( it != notes.end() )
						{
							if( ( *it )->selected() )
							{
								// copy this note
								note noteCopy( (note) **it );
								newNotes.push_back( noteCopy );
							}
							++it;
						}

						if( newNotes.size() != 0 )
						{
							//put notes from vector into piano roll
							for( int i=0; i<newNotes.size(); ++i)
							{
								note * newNote = m_pattern->addNote( newNotes[i] );
								newNote->setSelected( false );
							}

							// added new notes, so must update engine, song, etc
							engine::getSong()->setModified();
							update();
							engine::songEditor()->update();
						}
					}

					// play the note
					testPlayNote( m_currentNote );
				}

				engine::getSong()->setModified();
			}
			else if( ( _me->buttons() == Qt::RightButton &&
							m_editMode == ModeDraw ) ||
					m_editMode == ModeErase )
			{
				// erase single note
				m_mouseDownRight = true;
				if( it != notes.begin()-1 )
				{
					if( ( *it )->length() > 0 )
					{
						m_pattern->removeNote( *it );
					}
					else
					{
						( *it )->setLength( 0 );
						m_pattern->dataChanged();
					}
					engine::getSong()->setModified();
				}
			}
			else if( _me->button() == Qt::LeftButton &&
							m_editMode == ModeSelect )
			{
				// select an area of notes

				m_selectStartTick = pos_ticks;
				m_selectedTick = 0;
				m_selectStartKey = key_num;
				m_selectedKeys = 1;
				m_action = ActionSelectNotes;


				// call mousemove to fix glitch where selection
				// appears in wrong spot on mousedown
				mouseMoveEvent( _me );
			}

			update();
		}
		else if( _me->y() < keyAreaBottom() )
		{
			// clicked on keyboard on the left
			if( _me->buttons() == Qt::RightButton )
			{
				// right click, tone marker contextual menu
				m_semiToneMarkerMenu->popup( mapToGlobal( QPoint( _me->x(), _me->y() ) ) );
			}
			else
			{
				// left click - play the note
				m_lastKey = key_num;
				//if( ! m_recording && ! engine::getSong()->isPlaying() )
				{
					int v = ( (float) x ) / ( (float) WHITE_KEY_WIDTH ) * MidiDefaultVelocity;
					m_pattern->instrumentTrack()->pianoModel()->handleKeyPress( key_num, v );
				}
			}
		}
		else
		{
			if( _me->buttons() == Qt::LeftButton )
			{
				// clicked in the box below the keys to the left of note edit area
				m_noteEditMode = (noteEditMode)(((int)m_noteEditMode)+1);
				if( m_noteEditMode == NoteEditCount )
				{
					m_noteEditMode = (noteEditMode)0;
				}
				repaint();
			}
			else if( _me->buttons() == Qt::RightButton )
			{
				// pop menu asking which one they want to edit
				m_noteEditMenu->popup( mapToGlobal( QPoint( _me->x(), _me->y() ) ) );
			}
		}
	}
}




void PianoRoll::mouseDoubleClickEvent( QMouseEvent * _me )
{
	if( validPattern() == false )
	{
		return;
	}

	// if they clicked in the note edit area, clear selection
	if( _me->x() > noteEditLeft() && _me->x() < noteEditRight()
	    && _me->y() > noteEditTop() && _me->y() < noteEditBottom() )
	{
		clearSelectedNotes();
	}
}




void PianoRoll::testPlayNote( note * n )
{
	m_lastKey = n->key();

	if( n->isPlaying() == false && m_recording == false )
	{
		n->setIsPlaying( true );

		const int baseVelocity = m_pattern->instrumentTrack()->midiPort()->baseVelocity();

		m_pattern->instrumentTrack()->pianoModel()->handleKeyPress( n->key(), n->midiVelocity( baseVelocity ) );

		MidiEvent event( MidiMetaEvent, 0, n->key(), panningToMidi( n->getPanning() ) );

		event.setMetaEvent( MidiNotePanning );

		m_pattern->instrumentTrack()->processInEvent( event, 0 );
	}
}




void PianoRoll::pauseTestNotes( bool _pause )
{
	const NoteVector & notes = m_pattern->notes();
	NoteVector::ConstIterator it = notes.begin();
	while( it != notes.end() )
	{
		if( ( *it )->isPlaying() )
		{
			if( _pause )
			{
				// stop note
				m_pattern->instrumentTrack()->pianoModel()->handleKeyRelease( ( *it )->key() );
			}
			else
			{
				// start note
				( *it )->setIsPlaying( false );
				testPlayNote( *it );
			}
		}

		++it;
	}
}




void PianoRoll::testPlayKey( int key, int velocity, int pan )
{
	// turn off old key
	m_pattern->instrumentTrack()->pianoModel()->handleKeyRelease( m_lastKey );

	// remember which one we're playing
	m_lastKey = key;

	// play new key
	m_pattern->instrumentTrack()->pianoModel()->handleKeyPress( key, velocity );
}




void PianoRoll::computeSelectedNotes(bool shift)
{
	if( m_selectStartTick == 0 &&
		m_selectedTick == 0 &&
		m_selectStartKey == 0 &&
		m_selectedKeys == 0 )
	{
		// don't bother, there's no selection
		return;
	}

	// setup selection-vars
	int sel_pos_start = m_selectStartTick;
	int sel_pos_end = m_selectStartTick+m_selectedTick;
	if( sel_pos_start > sel_pos_end )
	{
		qSwap<int>( sel_pos_start, sel_pos_end );
	}

	int sel_key_start = m_selectStartKey - m_startKey + 1;
	int sel_key_end = sel_key_start + m_selectedKeys;
	if( sel_key_start > sel_key_end )
	{
		qSwap<int>( sel_key_start, sel_key_end );
	}

	//int y_base = noteEditTop() - 1;
	if( validPattern() == true )
	{
		const NoteVector & notes = m_pattern->notes();

		for( NoteVector::ConstIterator it = notes.begin();
						it != notes.end(); ++it )
		{
			// make a new selection unless they're holding shift
			if( ! shift )
			{
				( *it )->setSelected( false );
			}

			int len_ticks = ( *it )->length();

			if( len_ticks == 0 )
			{
				continue;
			}
			else if( len_ticks < 0 )
			{
				len_ticks = 4;
			}

			const int key = ( *it )->key() - m_startKey + 1;

			int pos_ticks = ( *it )->pos();

			// if the selection even barely overlaps the note
			if( key > sel_key_start &&
				key <= sel_key_end &&
				pos_ticks + len_ticks > sel_pos_start &&
				pos_ticks < sel_pos_end )
			{
				// remove from selection when holding shift
				if( shift && ( *it )->selected() )
				{
					( *it )->setSelected(false);
				}
				else
				{
					( *it )->setSelected(true);
				}
			}
		}
	}

	removeSelection();
	update();
}




void PianoRoll::mouseReleaseEvent( QMouseEvent * _me )
{
	bool mustRepaint = false;

	if( _me->button() & Qt::LeftButton )
	{
		m_mouseDownLeft = false;
		mustRepaint = true;
	}
	if( _me->button() & Qt::RightButton )
	{
		m_mouseDownRight = false;
		mustRepaint = true;
	}

	if( _me->button() & Qt::LeftButton &&
					m_editMode == ModeSelect &&
					m_action == ActionSelectNotes )
	{
		// select the notes within the selection rectangle and
		// then destroy the selection rectangle

		computeSelectedNotes( _me->modifiers() & Qt::ShiftModifier );

	}
	else if( _me->button() & Qt::LeftButton &&
					m_action == ActionMoveNote )
	{
		// we moved one or more notes so they have to be
		// moved properly according to new starting-
		// time in the note-array of pattern

		m_pattern->rearrangeAllNotes();

	}
	if( _me->button() & Qt::LeftButton &&
	   ( m_action == ActionMoveNote || m_action == ActionResizeNote ) )
	{
		// if we only moved one note, deselect it so we can
		// edit the notes in the note edit area
		if( selectionCount() == 1 )
		{
			clearSelectedNotes();
		}
	}


	if( validPattern() == true )
	{
		// turn off all notes that are playing
		const NoteVector & notes = m_pattern->notes();

		NoteVector::ConstIterator it = notes.begin();
		while( it != notes.end() )
		{
			if( ( *it )->isPlaying() )
			{
				m_pattern->instrumentTrack()->pianoModel()->handleKeyRelease( ( *it )->key() );
				( *it )->setIsPlaying( false );
			}

			++it;
		}

		// stop playing keys that we let go of
		m_pattern->instrumentTrack()->pianoModel()->handleKeyRelease( m_lastKey );
	}

	m_currentNote = NULL;

	m_action = ActionNone;

	if( m_editMode == ModeDraw )
	{
		QApplication::restoreOverrideCursor();
	}

	if( mustRepaint )
	{
		repaint();
	}
}




void PianoRoll::mouseMoveEvent( QMouseEvent * _me )
{
	if( validPattern() == false )
	{
		update();
		return;
	}

	if( m_action == ActionNone && _me->buttons() == 0 )
	{
		if( _me->y() > keyAreaBottom() && _me->y() < noteEditTop() )
		{
			QApplication::setOverrideCursor(
					QCursor( Qt::SizeVerCursor ) );
			return;
		}
	}
	else if( m_action == ActionResizeNoteEditArea )
	{
		// change m_notesEditHeight and then repaint
		m_notesEditHeight = tLimit<int>(
					m_oldNotesEditHeight - ( _me->y() - m_moveStartY ),
					NOTE_EDIT_MIN_HEIGHT,
					height() - PR_TOP_MARGIN - NOTE_EDIT_RESIZE_BAR -
									PR_BOTTOM_MARGIN - KEY_AREA_MIN_HEIGHT );
		repaint();
		return;
	}

	if( _me->y() > PR_TOP_MARGIN || m_action != ActionNone )
	{
		bool edit_note = ( _me->y() > noteEditTop() )
						&& m_action != ActionSelectNotes;


		int key_num = getKey( _me->y() );
		int x = _me->x();

		// see if they clicked on the keyboard on the left
		if( x < WHITE_KEY_WIDTH && m_action == ActionNone
		    && ! edit_note && key_num != m_lastKey
		    && _me->buttons() & Qt::LeftButton )
		{
			// clicked on a key, play the note
			testPlayKey( key_num, ( (float) x ) / ( (float) WHITE_KEY_WIDTH ) * MidiDefaultVelocity, 0 );
			update();
			return;
		}

		x -= WHITE_KEY_WIDTH;

		if( _me->buttons() & Qt::LeftButton
			&& m_editMode == ModeDraw
			&& (m_action == ActionMoveNote || m_action == ActionResizeNote ) )
		{
			// handle moving notes and resizing them
			bool replay_note = key_num != m_lastKey
							&& m_action == ActionMoveNote;

			if( replay_note )
			{
				pauseTestNotes();
			}

			dragNotes(
				_me->x(),
				_me->y(),
				_me->modifiers() & Qt::AltModifier,
				_me->modifiers() & Qt::ShiftModifier
			);

			if( replay_note && m_action == ActionMoveNote )
			{
				pauseTestNotes( false );
			}
		}
		else if( ( edit_note == true || m_action == ActionChangeNoteProperty ) &&
				_me->buttons() & Qt::LeftButton )
		{
			// editing note properties

			// Change notes within a certain pixel range of where
			// the mouse cursor is
			int pixel_range = 14;

			// convert to ticks so that we can check which notes
			// are in the range
			int ticks_start = (x-pixel_range/2) *
					MidiTime::ticksPerTact() / m_ppt + m_currentPosition;
			int ticks_end = (x+pixel_range/2) *
					MidiTime::ticksPerTact() / m_ppt + m_currentPosition;

			// get note-vector of current pattern
			const NoteVector & notes = m_pattern->notes();

			// determine what volume/panning to set note to
			volume_t vol = tLimit<int>( MinVolume +
							( ( (float)noteEditBottom() ) - ( (float)_me->y() ) ) /
							( (float)( noteEditBottom() - noteEditTop() ) ) *
							( MaxVolume - MinVolume ),
										MinVolume, MaxVolume );
			panning_t pan = tLimit<int>( PanningLeft +
							( (float)( noteEditBottom() - _me->y() ) ) /
							( (float)( noteEditBottom() - noteEditTop() ) ) *
							( (float)( PanningRight - PanningLeft ) ),
									  PanningLeft, PanningRight);

			if( m_noteEditMode == NoteEditVolume )
			{
				m_lastNoteVolume = vol;
			}
			else if( m_noteEditMode == NoteEditPanning )
			{
				m_lastNotePanning = pan;
			}



			// loop through vector
			bool use_selection = isSelection();
			NoteVector::ConstIterator it = notes.begin()+notes.size()-1;
			for( int i = 0; i < notes.size(); ++i )
			{
				note * n = *it;
				if( n->pos().getTicks() >= ticks_start
					&& n->pos().getTicks() <= ticks_end
					&& n->length().getTicks() != 0
					&& ( n->selected() || ! use_selection ) )
				{
					m_pattern->dataChanged();

					// play the note so that the user can tell how loud it is
					// and where it is panned
					testPlayNote( n );

					if( m_noteEditMode == NoteEditVolume )
					{
						n->setVolume( vol );

						const int baseVelocity = m_pattern->instrumentTrack()->midiPort()->baseVelocity();

						m_pattern->instrumentTrack()->processInEvent( MidiEvent( MidiKeyPressure, 0, n->key(), n->midiVelocity( baseVelocity ) ) );
					}
					else if( m_noteEditMode == NoteEditPanning )
					{
						n->setPanning( pan );
						MidiEvent evt( MidiMetaEvent, 0, n->key(), panningToMidi( pan ) );
						evt.setMetaEvent( MidiNotePanning );
						m_pattern->instrumentTrack()->processInEvent( evt );
					}
				}
				else
				{
					if( n->isPlaying() )
					{
						// mouse not over this note, stop playing it.
						m_pattern->instrumentTrack()->pianoModel()->handleKeyRelease( n->key() );

						n->setIsPlaying( false );
					}
				}

				--it;

			}
		}
		else if( _me->buttons() == Qt::NoButton && m_editMode == ModeDraw )
		{
			// set move- or resize-cursor

			// get tick in which the cursor is posated
			int pos_ticks = ( x * MidiTime::ticksPerTact() ) /
						m_ppt + m_currentPosition;

			// get note-vector of current pattern
			const NoteVector & notes = m_pattern->notes();

			// will be our iterator in the following loop
			NoteVector::ConstIterator it = notes.begin()+notes.size()-1;

			// loop through whole note-vector...
			for( int i = 0; i < notes.size(); ++i )
			{
				// and check whether the cursor is over an
				// existing note
				if( pos_ticks >= ( *it )->pos() &&
			    		pos_ticks <= ( *it )->pos() +
							( *it )->length() &&
					( *it )->key() == key_num &&
					( *it )->length() > 0 )
				{
					break;
				}
				--it;
			}

			// did it reach end of vector because there's
			// no note??
			if( it != notes.begin()-1 )
			{
				// cursor at the "tail" of the note?
				if( ( *it )->length() > 0 &&
					pos_ticks*m_ppt /
						MidiTime::ticksPerTact() >
						( ( *it )->pos() +
						( *it )->length() )*m_ppt/
						MidiTime::ticksPerTact()-
							RESIZE_AREA_WIDTH )
				{
					if( QApplication::overrideCursor() )
					{
	if( QApplication::overrideCursor()->shape() != Qt::SizeHorCursor )
						{
				while( QApplication::overrideCursor() != NULL )
				{
					QApplication::restoreOverrideCursor();
				}

				QCursor c( Qt::SizeHorCursor );
				QApplication::setOverrideCursor( c );
						}
					}
					else
					{
						QCursor c( Qt::SizeHorCursor );
						QApplication::setOverrideCursor(
									c );
					}
				}
				else
				{
					if( QApplication::overrideCursor() )
					{
	if( QApplication::overrideCursor()->shape() != Qt::SizeAllCursor )
						{
				while( QApplication::overrideCursor() != NULL )
				{
					QApplication::restoreOverrideCursor();
				}

						QCursor c( Qt::SizeAllCursor );
						QApplication::setOverrideCursor(
									c );
						}
					}
					else
					{
						QCursor c( Qt::SizeAllCursor );
						QApplication::setOverrideCursor(
									c );
					}
				}
			}
			else
			{
				// the cursor is over no note, so restore cursor
				while( QApplication::overrideCursor() != NULL )
				{
					QApplication::restoreOverrideCursor();
				}
			}
		}
		else if( _me->buttons() & Qt::LeftButton &&
						m_editMode == ModeSelect &&
						m_action == ActionSelectNotes )
		{

			// change size of selection

			// get tick in which the cursor is posated
			int pos_ticks = x * MidiTime::ticksPerTact() / m_ppt +
							m_currentPosition;

			m_selectedTick = pos_ticks - m_selectStartTick;
			if( (int) m_selectStartTick + m_selectedTick < 0 )
			{
				m_selectedTick = -static_cast<int>(
							m_selectStartTick );
			}
			m_selectedKeys = key_num - m_selectStartKey;
			if( key_num <= m_selectStartKey )
			{
				--m_selectedKeys;
			}
		}
		else if( m_editMode == ModeDraw && _me->buttons() & Qt::RightButton )
		{
			// holding down right-click to delete notes

			// get tick in which the user clicked
			int pos_ticks = x * MidiTime::ticksPerTact() / m_ppt +
							m_currentPosition;


			// get note-vector of current pattern
			const NoteVector & notes = m_pattern->notes();

			// will be our iterator in the following loop
			NoteVector::ConstIterator it = notes.begin();

			// loop through whole note-vector...
			while( it != notes.end() )
			{
				MidiTime len = ( *it )->length();
				if( len < 0 )
				{
					len = 4;
				}
				// and check whether the user clicked on an
				// existing note or an edit-line
				if( pos_ticks >= ( *it )->pos() &&
						len > 0 &&
					(
					( edit_note == false &&
					pos_ticks <= ( *it )->pos() + len &&
					( *it )->key() == key_num )
					||
					( edit_note == true &&
					pos_ticks <= ( *it )->pos() +
							NE_LINE_WIDTH *
						MidiTime::ticksPerTact() /
								m_ppt )
					)
					)
				{
					// delete this note
					if( it != notes.end() )
					{
						if( ( *it )->length() > 0 )
						{
							m_pattern->removeNote( *it );
						}
						else
						{
							( *it )->setLength( 0 );
							m_pattern->dataChanged();
						}
						engine::getSong()->setModified();
					}
				}
				else
				{
					++it;
				}
			}
		}
	}
	else
	{
		if( _me->buttons() & Qt::LeftButton &&
					m_editMode == ModeSelect &&
					m_action == ActionSelectNotes )
		{

			int x = _me->x() - WHITE_KEY_WIDTH;
			if( x < 0 && m_currentPosition > 0 )
			{
				x = 0;
				QCursor::setPos( mapToGlobal( QPoint(
							WHITE_KEY_WIDTH,
							_me->y() ) ) );
				if( m_currentPosition >= 4 )
				{
					m_leftRightScroll->setValue(
							m_currentPosition - 4 );
				}
				else
				{
					m_leftRightScroll->setValue( 0 );
				}
			}
			else if( x > width() - WHITE_KEY_WIDTH )
			{
				x = width() - WHITE_KEY_WIDTH;
				QCursor::setPos( mapToGlobal( QPoint( width(),
							_me->y() ) ) );
				m_leftRightScroll->setValue( m_currentPosition +
									4 );
			}

			// get tick in which the cursor is posated
			int pos_ticks = x * MidiTime::ticksPerTact()/ m_ppt +
							m_currentPosition;

			m_selectedTick = pos_ticks -
							m_selectStartTick;
			if( (int) m_selectStartTick + m_selectedTick <
									0 )
			{
				m_selectedTick = -static_cast<int>(
							m_selectStartTick );
			}


			int key_num = getKey( _me->y() );
			int visible_keys = ( height() - PR_TOP_MARGIN -
						PR_BOTTOM_MARGIN -
						m_notesEditHeight ) /
							KEY_LINE_HEIGHT + 2;
			const int s_key = m_startKey - 1;

			if( key_num <= s_key )
			{
				QCursor::setPos( mapToGlobal( QPoint( _me->x(),
							keyAreaBottom() ) ) );
				m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
				key_num = s_key;
			}
			else if( key_num >= s_key + visible_keys )
			{
				QCursor::setPos( mapToGlobal( QPoint( _me->x(),
							PR_TOP_MARGIN ) ) );
				m_topBottomScroll->setValue(
					m_topBottomScroll->value() - 1 );
				key_num = s_key + visible_keys;
			}

			m_selectedKeys = key_num - m_selectStartKey;
			if( key_num <= m_selectStartKey )
			{
				--m_selectedKeys;
			}
		}
		QApplication::restoreOverrideCursor();
	}

	m_lastMouseX = _me->x();
	m_lastMouseY = _me->y();

	update();
}




void PianoRoll::dragNotes( int x, int y, bool alt, bool shift )
{
	// dragging one or more notes around

	// convert pixels to ticks and keys
	int off_x = x - m_moveStartX;
	int off_ticks = off_x * MidiTime::ticksPerTact() / m_ppt;
	int off_key = getKey( y ) - getKey( m_moveStartY );

	// handle scroll changes while dragging
	off_ticks -= m_mouseDownTick - m_currentPosition;
	off_key -= m_mouseDownKey - m_startKey;


	// if they're not holding alt, quantize the offset
	if( ! alt )
	{
		off_ticks = floor( off_ticks / quantization() )
						* quantization();
	}

	// make sure notes won't go outside boundary conditions
	if( m_action == ActionMoveNote )
	{
		if( m_moveBoundaryLeft + off_ticks < 0 )
		{
			off_ticks += 0 - (off_ticks + m_moveBoundaryLeft);
		}
		if( m_moveBoundaryTop + off_key > NumKeys )
		{
			off_key -= NumKeys - (m_moveBoundaryTop + off_key);
		}
		if( m_moveBoundaryBottom + off_key < 0 )
		{
			off_key += 0 - (m_moveBoundaryBottom + off_key);
		}
	}

	int shift_offset = 0;
	int shift_ref_pos = -1;

	// get note-vector of current pattern
	const NoteVector & notes = m_pattern->notes();

	// will be our iterator in the following loop
	NoteVector::ConstIterator it = notes.begin();
	while( it != notes.end() )
	{
		const int pos = ( *it )->pos().getTicks();
		// when resizing a note and holding shift: shift the following
		// notes to preserve the melody
		if( m_action == ActionResizeNote && shift )
		{
			int shifted_pos = ( *it )->oldPos().getTicks() + shift_offset;
			if( shifted_pos && pos == shift_ref_pos )
			{
				shifted_pos -= off_ticks;
			}
			( *it )->setPos( MidiTime( shifted_pos ) );
		}

		if( ( *it )->selected() )
		{

			if( m_action == ActionMoveNote )
			{
				// moving note
				int pos_ticks = ( *it )->oldPos().getTicks()
										 + off_ticks;
				int key_num = ( *it )->oldKey() + off_key;

				if( pos_ticks < 0 )
				{
					pos_ticks = 0;
				}
				// upper/lower bound checks on key_num
				if( key_num < 0 )
				{
					key_num = 0;
				}
				else if( key_num > NumKeys )
				{
					key_num = NumKeys;
				}

				( *it )->setPos( MidiTime( pos_ticks ) );
				( *it )->setKey( key_num );
			}
			else if( m_action == ActionResizeNote )
			{
				// resizing note
				int ticks_new = ( *it )->oldLength().getTicks()
														+ off_ticks;
				if( ticks_new <= 0 )
				{
					ticks_new = 1;
				}
				else if( shift )
				{
					// when holding shift: update the offset used to shift
					// the following notes
					if( pos > shift_ref_pos )
					{
						shift_offset += off_ticks;
						shift_ref_pos = pos;
					}
				}
				( *it )->setLength( MidiTime( ticks_new ) );

				m_lenOfNewNotes = ( *it )->length();
			}
		}
		++it;
	}

	m_pattern->dataChanged();
	engine::getSong()->setModified();
}

static QString calculateNoteLabel(QString note, int octave)
{
	if(note.isEmpty())
		return "";
	return note + QString::number(octave);
}

static void printNoteHeights(QPainter& p, int bottom, int width, int startKey)
{
	assert(Key_C == 0);
	assert(Key_H == 11);

	struct KeyLabel
	{
		QString key, minor, major;
	};
	const KeyLabel labels[12] = {
		{QObject::tr("C", "Note name")},
		{"", QObject::tr("Db", "Note name"), QObject::tr("C#", "Note name")},
		{QObject::tr("D", "Note name")},
		{"", QObject::tr("Eb", "Note name"), QObject::tr("D#", "Note name")},
		{QObject::tr("E", "Note name"), QObject::tr("Fb", "Note name")},
		{"F"},
		{"", QObject::tr("Gb", "Note name"), QObject::tr("F#", "Note name")},
		{QObject::tr("G", "Note name")},
		{"", QObject::tr("Ab", "Note name"),QObject::tr( "G#", "Note name")},
		{QObject::tr("A", "Note name")},
		{"", QObject::tr("Bb", "Note name"),QObject::tr( "A#", "Note name")},
		{QObject::tr("B", "Note name")}
	};

	p.setFont( pointSize<KEY_LINE_HEIGHT-4>( p.font() ) );
	p.setPen( QColor( 255, 255, 255 ) );
	for( int y = bottom, key = startKey; y > PR_TOP_MARGIN;
			y -= KEY_LINE_HEIGHT, key++)
	{
		const unsigned note = key % KeysPerOctave;
		assert( note < ( sizeof( labels ) / sizeof( *labels) ));
		const KeyLabel& noteLabel( labels[note] );
		const int octave = key / KeysPerOctave;
		const KeyLabel notes = {
			calculateNoteLabel(noteLabel.key, octave),
			calculateNoteLabel(noteLabel.minor, octave),
			calculateNoteLabel(noteLabel.major, octave),
		};


		const int drawWidth( width - WHITE_KEY_WIDTH );
		const int hspace = 300;
		const int columnCount = drawWidth/hspace + 1;
		for(int col = 0; col < columnCount; col++)
		{
			const int subOffset = 42;
			const int x = subOffset + hspace/2 + hspace * col;
			p.drawText( WHITE_KEY_WIDTH + x, y, notes.key);
			p.drawText( WHITE_KEY_WIDTH + x - subOffset, y, notes.minor);
			p.drawText( WHITE_KEY_WIDTH + x + subOffset, y, notes.major);
		}
	}
}

void PianoRoll::paintEvent( QPaintEvent * _pe )
{
	QStyleOption opt;
	opt.initFrom( this );
	QPainter p( this );
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );

	// set font-size to 8
	p.setFont( pointSize<8>( p.font() ) );

	// y_offset is used to align the piano-keys on the key-lines
	int y_offset = 0;

	// calculate y_offset according to first key
	switch( prKeyOrder[m_startKey % KeysPerOctave] )
	{
		case PR_BLACK_KEY: y_offset = KEY_LINE_HEIGHT/4; break;
		case PR_WHITE_KEY_BIG: y_offset = KEY_LINE_HEIGHT/2; break;
		case PR_WHITE_KEY_SMALL:
			if( prKeyOrder[( ( m_startKey + 1 ) %
					KeysPerOctave)] != PR_BLACK_KEY )
			{
				y_offset = KEY_LINE_HEIGHT / 2;
			}
			break;
	}
	// start drawing at the bottom
	int key_line_y = keyAreaBottom() - 1;
	// used for aligning black-keys later
	int first_white_key_height = WHITE_KEY_SMALL_HEIGHT;
	// key-counter - only needed for finding out whether the processed
	// key is the first one
	int keys_processed = 0;

	int key = m_startKey;

	// display note marks before drawing other lines
	for( int i = 0; i < m_markedSemiTones.size(); i++ )
	{
		const int key_num = m_markedSemiTones.at( i );
		const int y = keyAreaBottom() + 5
			- KEY_LINE_HEIGHT * ( key_num - m_startKey + 1 );

		if( y > keyAreaBottom() )
		{
			break;
		}

		p.fillRect( WHITE_KEY_WIDTH+1, y-KEY_LINE_HEIGHT/2,
			    width() - 10, KEY_LINE_HEIGHT,
							QColor( 0, 80 - ( key_num % KeysPerOctave ) * 3, 64 + key_num / 2) );
	}


	// draw all white keys...
	for( int y = key_line_y + 1 + y_offset; y > PR_TOP_MARGIN;
			key_line_y -= KEY_LINE_HEIGHT, ++keys_processed )
	{
		// check for white key that is only half visible on the
		// bottom of piano-roll
		if( keys_processed == 0 &&
			prKeyOrder[m_startKey % KeysPerOctave] ==
								PR_BLACK_KEY )
		{
			// draw it!
			p.drawPixmap( PIANO_X, y - WHITE_KEY_SMALL_HEIGHT,
							*s_whiteKeySmallPm );
			// update y-pos
			y -= WHITE_KEY_SMALL_HEIGHT / 2;
			// move first black key down (we didn't draw whole
			// white key so black key needs to be lifted down)
			// (default for first_white_key_height =
			// WHITE_KEY_SMALL_HEIGHT, so WHITE_KEY_SMALL_HEIGHT/2
			// is smaller)
			first_white_key_height = WHITE_KEY_SMALL_HEIGHT / 2;
		}
		// check whether to draw a big or a small white key
		if( prKeyOrder[key % KeysPerOctave] == PR_WHITE_KEY_SMALL )
		{
			// draw a small one while checking if it is pressed or not
			if( validPattern() && m_pattern->instrumentTrack()->pianoModel()->isKeyPressed( key ) )
			{
				p.drawPixmap( PIANO_X, y - WHITE_KEY_SMALL_HEIGHT, *s_whiteKeySmallPressedPm );
			}
			else
			{
				p.drawPixmap( PIANO_X, y - WHITE_KEY_SMALL_HEIGHT, *s_whiteKeySmallPm );
			}
			// update y-pos
			y -= WHITE_KEY_SMALL_HEIGHT;

		}
		else if( prKeyOrder[key % KeysPerOctave] ==
							PR_WHITE_KEY_BIG )
		{
			// draw a big one while checking if it is pressed or not
			if( validPattern() && m_pattern->instrumentTrack()->pianoModel()->isKeyPressed( key ) )
			{
				p.drawPixmap( PIANO_X, y - WHITE_KEY_BIG_HEIGHT, *s_whiteKeyBigPressedPm );
			}
			else
			{
				p.drawPixmap( PIANO_X, y-WHITE_KEY_BIG_HEIGHT, *s_whiteKeyBigPm );
			}
			// if a big white key has been the first key,
			// black keys needs to be lifted up
			if( keys_processed == 0 )
			{
				first_white_key_height = WHITE_KEY_BIG_HEIGHT;
			}
			// update y-pos
			y -= WHITE_KEY_BIG_HEIGHT;
		}
		// label C-keys...
		if( static_cast<Keys>( key % KeysPerOctave ) == Key_C )
		{
			p.setPen( QColor( 240, 240, 240 ) );
			p.drawText( C_KEY_LABEL_X + 1, y+14, "C" +
					QString::number( static_cast<int>( key /
							KeysPerOctave ) ) );
			p.setPen( QColor( 0, 0, 0 ) );
			p.drawText( C_KEY_LABEL_X, y + 13, "C" +
					QString::number( static_cast<int>( key /
							KeysPerOctave ) ) );
			p.setPen( QColor( 0x4F, 0x4F, 0x4F ) );
		}
		else
		{
			p.setPen( QColor( 0x3F, 0x3F, 0x3F ) );
		}
		// draw key-line
		p.drawLine( WHITE_KEY_WIDTH, key_line_y, width(), key_line_y );
		++key;
	}

	// reset all values, because now we're going to draw all black keys
	key = m_startKey;
	keys_processed = 0;
	int white_cnt = 0;

	// and go!
	for( int y = keyAreaBottom() + y_offset;
					y > PR_TOP_MARGIN; ++keys_processed )
	{
		// check for black key that is only half visible on the bottom
		// of piano-roll
		if( keys_processed == 0
		    // current key may not be a black one
		    && prKeyOrder[key % KeysPerOctave] != PR_BLACK_KEY
		    // but the previous one must be black (we must check this
		    // because there might be two white keys (E-F)
		    && prKeyOrder[( key - 1 ) % KeysPerOctave] ==
								PR_BLACK_KEY )
		{
			// draw the black key!
			p.drawPixmap( PIANO_X, y - BLACK_KEY_HEIGHT / 2,
								*s_blackKeyPm );
			// is the one after the start-note a black key??
			if( prKeyOrder[( key + 1 ) % KeysPerOctave] !=
								PR_BLACK_KEY )
			{
				// no, then move it up!
				y -= KEY_LINE_HEIGHT / 2;
			}
		}
		// current key black?
		if( prKeyOrder[key % KeysPerOctave] == PR_BLACK_KEY)
		{
			// then draw it (calculation of y very complicated,
			// but that's the only working solution, sorry...)
			// check if the key is pressed or not
			if( validPattern() && m_pattern->instrumentTrack()->pianoModel()->isKeyPressed( key ) )
			{
				p.drawPixmap( PIANO_X, y - ( first_white_key_height -
						WHITE_KEY_SMALL_HEIGHT ) -
						WHITE_KEY_SMALL_HEIGHT/2 - 1 -
						BLACK_KEY_HEIGHT, *s_blackKeyPressedPm );
			}
		    else
			{
				p.drawPixmap( PIANO_X, y - ( first_white_key_height -
						WHITE_KEY_SMALL_HEIGHT ) -
						WHITE_KEY_SMALL_HEIGHT/2 - 1 -
						BLACK_KEY_HEIGHT, *s_blackKeyPm );
			}
			// update y-pos
			y -= WHITE_KEY_BIG_HEIGHT;
			// reset white-counter
			white_cnt = 0;
		}
		else
		{
			// simple workaround for increasing x if there were
			// two white keys (e.g. between E and F)
			++white_cnt;
			if( white_cnt > 1 )
			{
				y -= WHITE_KEY_BIG_HEIGHT/2;
			}
		}

		++key;
	}


	// erase the area below the piano, because there might be keys that
	// should be only half-visible
	p.fillRect( QRect( 0, keyAreaBottom(),
			WHITE_KEY_WIDTH, noteEditBottom()-keyAreaBottom() ),
			QColor( 0, 0, 0 ) );

	// display note editing info
	QFont f = p.font();
	f.setBold( false );
	p.setFont( pointSize<10>( f ) );
	p.setPen( QColor( 255, 255, 255) );
	p.drawText( QRect( 0, keyAreaBottom(),
					  WHITE_KEY_WIDTH, noteEditBottom() - keyAreaBottom() ),
			   Qt::AlignCenter | Qt::TextWordWrap,
			   m_nemStr.at( m_noteEditMode ) + ":" );

	// set clipping area, because we are not allowed to paint over
	// keyboard...
	p.setClipRect( WHITE_KEY_WIDTH, PR_TOP_MARGIN,
				width() - WHITE_KEY_WIDTH,
				height() - PR_TOP_MARGIN - PR_BOTTOM_MARGIN );

	// draw vertical raster

	// triplet mode occurs if the note duration isn't a multiple of 3
	bool triplets = ( quantization() % 3 != 0 );

	int spt = MidiTime::stepsPerTact();
	float pp16th = (float)m_ppt / spt;
	int bpt = DefaultBeatsPerTact;
	if ( triplets ) {
		spt = static_cast<int>(1.5 * spt);
		bpt = static_cast<int>(bpt * 2.0/3.0);
		pp16th *= 2.0/3.0;
	}

	int tact_16th = m_currentPosition / bpt;

	const int offset = ( m_currentPosition % bpt ) *
			m_ppt / MidiTime::ticksPerTact();

	bool show32nds = ( m_zoomingModel.value() > 3 );

	// we need float here as odd time signatures might produce rounding
	// errors else and thus an unusable grid
	for( float x = WHITE_KEY_WIDTH - offset; x < width();
						x += pp16th, ++tact_16th )
	{
		if( x >= WHITE_KEY_WIDTH )
		{
			// every tact-start needs to be a bright line
			if( tact_16th % spt == 0 )
			{
	 			p.setPen( QColor( 0x7F, 0x7F, 0x7F ) );
			}
			// normal line
			else if( tact_16th % 4 == 0 )
			{
				p.setPen( QColor( 0x5F, 0x5F, 0x5F ) );
			}
			// weak line
			else
			{
				p.setPen( QColor( 0x3F, 0x3F, 0x3F ) );
			}

			p.drawLine( (int)x, PR_TOP_MARGIN, (int)x, height() -
							PR_BOTTOM_MARGIN );

			// extra 32nd's line
			if( show32nds )
			{
				p.setPen( QColor( 0x22, 0x22, 0x22 ) );
				p.drawLine( (int)(x + pp16th/2) , PR_TOP_MARGIN,
						(int)(x + pp16th/2), height() -
						PR_BOTTOM_MARGIN );
			}
		}
	}



	// following code draws all notes in visible area
	// and the note editing stuff (volume, panning, etc)

	// setup selection-vars
	int sel_pos_start = m_selectStartTick;
	int sel_pos_end = m_selectStartTick+m_selectedTick;
	if( sel_pos_start > sel_pos_end )
	{
		qSwap<int>( sel_pos_start, sel_pos_end );
	}

	int sel_key_start = m_selectStartKey - m_startKey + 1;
	int sel_key_end = sel_key_start + m_selectedKeys;
	if( sel_key_start > sel_key_end )
	{
		qSwap<int>( sel_key_start, sel_key_end );
	}

	int y_base = keyAreaBottom() - 1;
	if( validPattern() == true )
	{
		p.setClipRect( WHITE_KEY_WIDTH, PR_TOP_MARGIN,
				width() - WHITE_KEY_WIDTH,
				height() - PR_TOP_MARGIN );

		const NoteVector & notes = m_pattern->notes();

		const int visible_keys = ( keyAreaBottom()-keyAreaTop() ) /
							KEY_LINE_HEIGHT + 2;

		QPolygon editHandles;

		for( NoteVector::ConstIterator it = notes.begin();
						it != notes.end(); ++it )
		{
			int len_ticks = ( *it )->length();

			if( len_ticks == 0 )
			{
				continue;
			}
			else if( len_ticks < 0 )
			{
				len_ticks = 4;
			}

			const int key = ( *it )->key() - m_startKey + 1;

			int pos_ticks = ( *it )->pos();

			int note_width = len_ticks * m_ppt /
						MidiTime::ticksPerTact();
			const int x = ( pos_ticks - m_currentPosition ) *
					m_ppt / MidiTime::ticksPerTact();
			// skip this note if not in visible area at all
			if( !( x + note_width >= 0 &&
					x <= width() - WHITE_KEY_WIDTH ) )
			{
				continue;
			}

			// is the note in visible area?
			if( key > 0 && key <= visible_keys )
			{

				// we've done and checked all, let's draw the
				// note
				drawNoteRect( p, x + WHITE_KEY_WIDTH,
						y_base - key * KEY_LINE_HEIGHT,
								note_width, *it );
			}

			// draw note editing stuff
			int editHandleTop = 0;
			if( m_noteEditMode == NoteEditVolume )
			{
				QColor color = QColor::fromHsv( 140, 221,
						qMin(255, 60 + ( *it )->getVolume() ) );
				if( ( *it )->selected() )
				{
					color.setRgb( 0x00, 0x40, 0xC0 );
				}
				p.setPen( QPen( color, NE_LINE_WIDTH ) );

				editHandleTop = noteEditBottom() -
					( (float)( ( *it )->getVolume() - MinVolume ) ) /
					( (float)( MaxVolume - MinVolume ) ) *
					( (float)( noteEditBottom() - noteEditTop() ) );

				p.drawLine( noteEditLeft() + x, editHandleTop,
							noteEditLeft() + x, noteEditBottom() );

			}
			else if( m_noteEditMode == NoteEditPanning )
			{
				QColor color( 0x99, 0xAF, 0xFF );
				if( ( *it )->selected() )
				{
					color.setRgb( 0x00, 0x40, 0xC0 );
				}

				p.setPen( QPen( color, NE_LINE_WIDTH ) );

				editHandleTop = noteEditBottom() -
					( (float)( ( *it )->getPanning() - PanningLeft ) ) /
					( (float)( (PanningRight - PanningLeft ) ) ) *
					( (float)( noteEditBottom() - noteEditTop() ) );

				p.drawLine( noteEditLeft() + x, noteEditTop() +
						( (float)( noteEditBottom() - noteEditTop() ) ) / 2.0f,
						    noteEditLeft() + x, editHandleTop );
			}
			editHandles << QPoint( x + noteEditLeft(),
						editHandleTop+1 );

			if( ( *it )->hasDetuningInfo() )
			{
				drawDetuningInfo( p, *it,
					x + WHITE_KEY_WIDTH,
					y_base - key * KEY_LINE_HEIGHT );
			}
		}

		p.setPen( QPen( QColor( 0x99, 0xAF, 0xFF ),
				NE_LINE_WIDTH+2 ) );
		p.drawPoints( editHandles );

	}
	else
	{
		QFont f = p.font();
		f.setBold( true );
		p.setFont( pointSize<14>( f ) );
		p.setPen( QColor( 0x4A, 0xFD, 0x85 ) );
		p.drawText( WHITE_KEY_WIDTH + 20, PR_TOP_MARGIN + 40,
				tr( "Please open a pattern by double-clicking "
								"on it!" ) );
	}

	p.setClipRect( WHITE_KEY_WIDTH, PR_TOP_MARGIN, width() -
				WHITE_KEY_WIDTH, height() - PR_TOP_MARGIN -
					m_notesEditHeight - PR_BOTTOM_MARGIN );

	// now draw selection-frame
	int x = ( ( sel_pos_start - m_currentPosition ) * m_ppt ) /
						MidiTime::ticksPerTact();
	int w = ( ( ( sel_pos_end - m_currentPosition ) * m_ppt ) /
						MidiTime::ticksPerTact() ) - x;
	int y = (int) y_base - sel_key_start * KEY_LINE_HEIGHT;
	int h = (int) y_base - sel_key_end * KEY_LINE_HEIGHT - y;
	p.setPen( QColor( 0, 64, 192 ) );
	p.drawRect( x + WHITE_KEY_WIDTH, y, w, h );

	// TODO: Get this out of paint event
	int l = ( validPattern() == true )? (int) m_pattern->length() : 0;

	// reset scroll-range
	if( m_leftRightScroll->maximum() != l )
	{
		m_leftRightScroll->setRange( 0, l );
		m_leftRightScroll->setPageStep( l );
	}

	// horizontal line for the key under the cursor
	if( validPattern() == true )
	{
		int key_num = getKey( mapFromGlobal( QCursor::pos() ).y() );
		p.fillRect( 10, keyAreaBottom() + 3 - KEY_LINE_HEIGHT *
					( key_num - m_startKey + 1 ),
				width() - 10, KEY_LINE_HEIGHT - 7,
							QColor( 64, 64, 64 ) );
	}

	// bar to resize note edit area
	p.setClipRect( 0, 0, width(), height() );
	p.fillRect( QRect( 0, keyAreaBottom(),
					width()-PR_RIGHT_MARGIN, NOTE_EDIT_RESIZE_BAR ),
			   QColor( 64, 64, 64 ) );

	const QPixmap * cursor = NULL;
	// draw current edit-mode-icon below the cursor
	switch( m_editMode )
	{
		case ModeDraw:
			if( m_mouseDownRight )
			{
				cursor = s_toolErase;
			}
			else if( m_action == ActionMoveNote )
			{
				cursor = s_toolMove;
			}
			else
			{
				cursor = s_toolDraw;
			}
			break;
		case ModeErase: cursor = s_toolErase; break;
		case ModeSelect: cursor = s_toolSelect; break;
		case ModeEditDetuning: cursor = s_toolOpen; break;
	}
	if( cursor != NULL )
	{
		p.drawPixmap( mapFromGlobal( QCursor::pos() ) + QPoint( 8, 8 ),
								*cursor );
	}

	if( configManager::inst()->value( "ui", "printnotelabels").toInt() )
	{
		printNoteHeights(p, keyAreaBottom(), width(), m_startKey);
	}
}




// responsible for moving/resizing scrollbars after window-resizing
void PianoRoll::resizeEvent( QResizeEvent * )
{
	m_leftRightScroll->setGeometry( WHITE_KEY_WIDTH, height() -
								SCROLLBAR_SIZE,
					width()-WHITE_KEY_WIDTH,
							SCROLLBAR_SIZE );
	m_topBottomScroll->setGeometry( width() - SCROLLBAR_SIZE, PR_TOP_MARGIN,
						SCROLLBAR_SIZE,
						height() - PR_TOP_MARGIN -
						SCROLLBAR_SIZE );

	int total_pixels = OCTAVE_HEIGHT * NumOctaves - ( height() -
					PR_TOP_MARGIN - PR_BOTTOM_MARGIN -
							m_notesEditHeight );
	m_totalKeysToScroll = total_pixels * KeysPerOctave / OCTAVE_HEIGHT;

	m_topBottomScroll->setRange( 0, m_totalKeysToScroll );

	if( m_startKey > m_totalKeysToScroll )
	{
		m_startKey = m_totalKeysToScroll;
	}
	m_topBottomScroll->setValue( m_totalKeysToScroll - m_startKey );

	engine::getSong()->getPlayPos( song::Mode_PlayPattern
					).m_timeLine->setFixedWidth( width() );
	m_toolBar->setFixedWidth( width() );
	update();
}




void PianoRoll::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	if( _we->modifiers() & Qt::ControlModifier )
	{
		if( _we->delta() > 0 )
		{
			m_ppt = qMin( m_ppt * 2, KEY_LINE_HEIGHT *
						DefaultStepsPerTact * 8 );
		}
		else if( m_ppt >= 72 )
		{
			m_ppt /= 2;
		}
		// update combobox with zooming-factor
		m_zoomingModel.setValue(
				m_zoomingModel.findText( QString::number(
					static_cast<int>( m_ppt * 100 /
						DEFAULT_PR_PPT ) ) +"%" ) );
		// update timeline
		m_timeLine->setPixelsPerTact( m_ppt );
		update();
	}
	else if( _we->modifiers() & Qt::ShiftModifier
			 || _we->orientation() == Qt::Horizontal )
	{
		m_leftRightScroll->setValue( m_leftRightScroll->value() -
							_we->delta() * 2 / 15 );
	}
	else
	{
		m_topBottomScroll->setValue( m_topBottomScroll->value() -
							_we->delta() / 30 );
	}
}




int PianoRoll::getKey( int _y ) const
{
	int key_line_y = keyAreaBottom() - 1;
	// pressed key on piano
	int key_num = ( key_line_y - _y ) / KEY_LINE_HEIGHT;
	key_num += m_startKey;

	// some range-checking-stuff
	if( key_num < 0 )
	{
		key_num = 0;
	}

	if( key_num >= KeysPerOctave * NumOctaves )
	{
		key_num = KeysPerOctave * NumOctaves - 1;
	}

	return key_num;
}




song::PlayModes PianoRoll::desiredPlayModeForAccompany() const
{
	if( m_pattern->getTrack()->trackContainer() ==
					engine::getBBTrackContainer() )
	{
		return song::Mode_PlayBB;
	}
	return song::Mode_PlaySong;
}




void PianoRoll::play()
{
	if( validPattern() == false )
	{
		return;
	}

	if( engine::getSong()->playMode() != song::Mode_PlayPattern )
	{
		engine::getSong()->playPattern( m_pattern );
	}
	else
	{
		engine::getSong()->togglePause();
	}
}




void PianoRoll::record()
{
	if( engine::getSong()->isPlaying() )
	{
		stop();
	}
	if( m_recording == true || validPattern() == false )
	{
		return;
	}

	m_recording = true;

	engine::getSong()->playPattern( m_pattern, false );
}




void PianoRoll::recordAccompany()
{
	if( engine::getSong()->isPlaying() )
	{
		stop();
	}
	if( m_recording == true || validPattern() == false )
	{
		return;
	}

	m_recording = true;

	if( m_pattern->getTrack()->trackContainer() == engine::getSong() )
	{
		engine::getSong()->playSong();
	}
	else
	{
		engine::getSong()->playBB();
	}
}





void PianoRoll::stop()
{
	engine::getSong()->stop();
	m_recording = false;
	m_scrollBack = true;
}




void PianoRoll::startRecordNote( const note & _n )
{
	if( m_recording == true && validPattern() == true &&
		engine::getSong()->isPlaying() &&
			( engine::getSong()->playMode() ==
					desiredPlayModeForAccompany() ||
				engine::getSong()->playMode() ==
					song::Mode_PlayPattern ) )
	{
		MidiTime sub;
		if( engine::getSong()->playMode() == song::Mode_PlaySong )
		{
			sub = m_pattern->startPosition();
		}
		note n( 1, engine::getSong()->getPlayPos(
					engine::getSong()->playMode() ) - sub,
				_n.key(), _n.getVolume(), _n.getPanning() );
		if( n.pos() >= 0 )
		{
			m_recordingNotes << n;
		}
	}
}




void PianoRoll::finishRecordNote( const note & _n )
{
	if( m_recording == true && validPattern() == true &&
		engine::getSong()->isPlaying() &&
			( engine::getSong()->playMode() ==
					desiredPlayModeForAccompany() ||
				engine::getSong()->playMode() ==
					song::Mode_PlayPattern ) )
	{
		for( QList<note>::Iterator it = m_recordingNotes.begin();
					it != m_recordingNotes.end(); ++it )
		{
			if( it->key() == _n.key() )
			{
				note n( _n.length(), it->pos(),
						it->key(), it->getVolume(),
						it->getPanning() );
				n.quantizeLength( quantization() );
				m_pattern->addNote( n );
				update();
				m_recordingNotes.erase( it );
				break;
			}
		}
	}
}




void PianoRoll::horScrolled( int _new_pos )
{
	m_currentPosition = _new_pos;
	emit positionChanged( m_currentPosition );
	update();
}




void PianoRoll::verScrolled( int _new_pos )
{
	// revert value
	m_startKey = m_totalKeysToScroll - _new_pos;

	update();
}




void PianoRoll::drawButtonToggled()
{
	m_editMode = ModeDraw;
	update();
}




void PianoRoll::eraseButtonToggled()
{
	m_editMode = ModeErase;
	update();
}




void PianoRoll::selectButtonToggled()
{
	m_editMode = ModeSelect;
	update();
}



void PianoRoll::detuneButtonToggled()
{
	m_editMode = ModeEditDetuning;
	update();
}



void PianoRoll::selectAll()
{
	if( validPattern() == false )
	{
		return;
	}

	const NoteVector & notes = m_pattern->notes();

	// if first_time = true, we HAVE to set the vars for select
	bool first_time = true;

	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end(); ++it )
	{
		int len_ticks = ( *it )->length();

		if( len_ticks > 0 )
		{
			const int key = ( *it )->key();

			int pos_ticks = ( *it )->pos();
			if( key <= m_selectStartKey || first_time )
			{
				// if we move start-key down, we have to add
				// the difference between old and new start-key
				// to m_selectedKeys, otherwise the selection
				// is just moved down...
				m_selectedKeys += m_selectStartKey
								- ( key - 1 );
				m_selectStartKey = key - 1;
			}
			if( key >= m_selectedKeys+m_selectStartKey ||
								first_time )
			{
				m_selectedKeys = key - m_selectStartKey;
			}
			if( pos_ticks < m_selectStartTick ||
								first_time )
			{
				m_selectStartTick = pos_ticks;
			}
			if( pos_ticks + len_ticks >
				m_selectStartTick + m_selectedTick ||
								first_time )
			{
				m_selectedTick = pos_ticks +
							len_ticks -
							m_selectStartTick;
			}
			first_time = false;
		}
	}
}




// returns vector with pointers to all selected notes
void PianoRoll::getSelectedNotes( NoteVector & _selected_notes )
{
	if( validPattern() == false )
	{
		return;
	}

	const NoteVector & notes = m_pattern->notes();

	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end();
									++it )
	{
		if( ( *it )->selected() )
		{
			_selected_notes.push_back( *it );
		}
	}
}




void PianoRoll::copy_to_clipboard( const NoteVector & _notes ) const
{
	DataFile dataFile( DataFile::ClipboardData );
	QDomElement note_list = dataFile.createElement( "note-list" );
	dataFile.content().appendChild( note_list );

	MidiTime start_pos( _notes.front()->pos().getTact(), 0 );
	for( NoteVector::ConstIterator it = _notes.begin(); it != _notes.end();
									++it )
	{
		note clip_note( **it );
		clip_note.setPos( clip_note.pos( start_pos ) );
		clip_note.saveState( dataFile, note_list );
	}

	QMimeData * clip_content = new QMimeData;
	clip_content->setData( Clipboard::mimeType(), dataFile.toString().toUtf8() );
	QApplication::clipboard()->setMimeData( clip_content,
							QClipboard::Clipboard );
}




void PianoRoll::copySelectedNotes()
{
	NoteVector selected_notes;
	getSelectedNotes( selected_notes );

	if( selected_notes.empty() == false )
	{
		copy_to_clipboard( selected_notes );
	}
}




void PianoRoll::cutSelectedNotes()
{
	if( validPattern() == false )
	{
		return;
	}

	NoteVector selected_notes;
	getSelectedNotes( selected_notes );

	if( selected_notes.empty() == false )
	{
		copy_to_clipboard( selected_notes );

		engine::getSong()->setModified();

		for( NoteVector::Iterator it = selected_notes.begin();
					it != selected_notes.end(); ++it )
		{
			// note (the memory of it) is also deleted by
			// pattern::removeNote(...) so we don't have to do that
			m_pattern->removeNote( *it );
		}
	}

	update();
	engine::songEditor()->update();
}




void PianoRoll::pasteNotes()
{
	if( validPattern() == false )
	{
		return;
	}

	QString value = QApplication::clipboard()
				->mimeData( QClipboard::Clipboard )
						->data( Clipboard::mimeType() );

	if( !value.isEmpty() )
	{
		DataFile dataFile( value.toUtf8() );

		QDomNodeList list = dataFile.elementsByTagName( note::classNodeName() );

		// remove selection and select the newly pasted notes
		clearSelectedNotes();

		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			// create the note
			note cur_note;
			cur_note.restoreState( list.item( i ).toElement() );
			cur_note.setPos( cur_note.pos() + m_timeLine->pos() );

			// select it
			cur_note.setSelected( true );

			// add to pattern
			m_pattern->addNote( cur_note );
		}

		// we only have to do the following lines if we pasted at
		// least one note...
		engine::getSong()->setModified();
		m_ctrlMode = ModeDraw;
		m_drawButton->setChecked( true );
		update();
		engine::songEditor()->update();
	}
}




void PianoRoll::deleteSelectedNotes()
{
	if( validPattern() == false )
	{
		return;
	}

	bool update_after_delete = false;


	// get note-vector of current pattern
	const NoteVector & notes = m_pattern->notes();

	// will be our iterator in the following loop
	NoteVector::ConstIterator it = notes.begin();
	while( it != notes.end() )
	{
		if( ( *it )->selected() )
		{
			// delete this note
			m_pattern->removeNote( ( *it ) );
			update_after_delete = true;

			// start over, make sure we get all the notes
			it = notes.begin();
		}
		else
		{
			++it;
		}
	}

	if( update_after_delete == true )
	{
		engine::getSong()->setModified();
		update();
		engine::songEditor()->update();
	}

}




void PianoRoll::autoScroll( const MidiTime & _t )
{
	const int w = width() - WHITE_KEY_WIDTH;
	if( _t > m_currentPosition + w * MidiTime::ticksPerTact() / m_ppt )
	{
		m_leftRightScroll->setValue( _t.getTact() *
					MidiTime::ticksPerTact() );
	}
	else if( _t < m_currentPosition )
	{
		MidiTime t = qMax( _t - w * MidiTime::ticksPerTact() *
					MidiTime::ticksPerTact() / m_ppt, 0 );
		m_leftRightScroll->setValue( t.getTact() *
						MidiTime::ticksPerTact() );
	}
	m_scrollBack = false;
}




void PianoRoll::updatePosition( const MidiTime & _t )
{
	if( ( engine::getSong()->isPlaying() &&
			engine::getSong()->playMode() ==
					song::Mode_PlayPattern &&
		m_timeLine->autoScroll() == timeLine::AutoScrollEnabled ) ||
							m_scrollBack == true )
	{
		autoScroll( _t );
	}
}




void PianoRoll::updatePositionAccompany( const MidiTime & _t )
{
	song * s = engine::getSong();

	if( m_recording && validPattern() &&
					s->playMode() != song::Mode_PlayPattern )
	{
		MidiTime pos = _t;
		if( s->playMode() != song::Mode_PlayBB )
		{
			pos -= m_pattern->startPosition();
		}
		if( (int) pos > 0 )
		{
			s->getPlayPos( song::Mode_PlayPattern ).setTicks( pos );
			autoScroll( pos );
		}
	}
}




void PianoRoll::zoomingChanged()
{
	const QString & zfac = m_zoomingModel.currentText();
	m_ppt = zfac.left( zfac.length() - 1 ).toInt() * DEFAULT_PR_PPT / 100;
#ifdef LMMS_DEBUG
	assert( m_ppt > 0 );
#endif
	m_timeLine->setPixelsPerTact( m_ppt );
	update();

}




void PianoRoll::quantizeChanged()
{
	if( m_quantizeModel.value() == 0 &&
			m_noteLenModel.value() == 0 )
	{
		m_quantizeModel.setValue( m_quantizeModel.findText( "1/16" ) );
		return;
	}
	// Could be smarter
	update();
}


int PianoRoll::quantization() const
{
	if( m_quantizeModel.value() == 0 )
	{
		return newNoteLen();
	}
	return DefaultTicksPerTact / m_quantizeModel.currentText().right(
				m_quantizeModel.currentText().length() -
								2 ).toInt();
}




void PianoRoll::updateSemiToneMarkerMenu()
{
	const InstrumentFunctionNoteStacking::Chord & scale = InstrumentFunctionNoteStacking::ChordTable::getInstance()
					.getScaleByName( m_scaleModel.currentText() );

	const InstrumentFunctionNoteStacking::Chord & chord = InstrumentFunctionNoteStacking::ChordTable::getInstance()
					.getChordByName( m_chordModel.currentText() );

	emit semiToneMarkerMenuScaleSetEnabled( ! scale.isEmpty() );
	emit semiToneMarkerMenuChordSetEnabled( ! chord.isEmpty() );
}




MidiTime PianoRoll::newNoteLen() const
{
	if( m_noteLenModel.value() == 0 )
	{
		return m_lenOfNewNotes;
	}
	return DefaultTicksPerTact / m_noteLenModel.currentText().right(
				m_noteLenModel.currentText().length() -
								2 ).toInt();
}




bool PianoRoll::mouseOverNote()
{
	return validPattern() && noteUnderMouse() != NULL;
}




note * PianoRoll::noteUnderMouse()
{
	QPoint pos = mapFromGlobal( QCursor::pos() );

	// get note-vector of current pattern
	const NoteVector & notes = m_pattern->notes();

	if( pos.x() <= WHITE_KEY_WIDTH || pos.x() > width() - SCROLLBAR_SIZE
		|| pos.y() < PR_TOP_MARGIN
		|| pos.y() > keyAreaBottom() )
	{
		return NULL;
	}

	int key_num = getKey( pos.y() );
	int pos_ticks = ( pos.x() - WHITE_KEY_WIDTH ) *
			MidiTime::ticksPerTact() / m_ppt + m_currentPosition;

	// will be our iterator in the following loop
	NoteVector::ConstIterator it = notes.begin()+notes.size()-1;

	// loop through whole note-vector...
	int i;
	for( i = 0; i < notes.size(); ++i )
	{
		// and check whether the cursor is over an
		// existing note
		if( pos_ticks >= ( *it )->pos() &&
	    		pos_ticks <= ( *it )->pos() + ( *it )->length() &&
			( *it )->key() == key_num && ( *it )->length() > 0 )
		{
			break;
		}
		--it;
	}

	if( i == notes.size() )
	{
		return NULL;
	}

	return *it;
}




#include "moc_PianoRoll.cxx"


