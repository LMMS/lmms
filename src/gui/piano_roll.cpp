/*
 * piano_roll.cpp - implementation of piano-roll which is used for actual
 *                  writing of melodies
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "piano_roll.h"
#include "bb_track_container.h"
#include "Clipboard.h"
#include "combobox.h"
#include "debug.h"
#include "DetuningHelper.h"
#include "embed.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"
#include "MainWindow.h"
#include "midi.h"
#include "mmp.h"
#include "pattern.h"
#include "Piano.h"
#include "pixmap_button.h"
#include "song.h"
#include "song_editor.h"
#include "templates.h"
#include "text_float.h"
#include "timeline.h"
#include "tool_button.h"
#include "tooltip.h"
#include "fluiq/collapsible_widget.h"


typedef AutomationPattern::timeMap timeMap;


extern Keys whiteKeys[];	// defined in piano_widget.cpp


// some constants...
static const int InitialPianoRollWidth = 840;
static const int InitialPianoRollHeight = 480;

static const int ScrollBarSize = 16;
static const int PianoX = 0;

static const int WhiteKeyWidth = 64;
static const int BlackKeyWidth = 41;
static const int WhiteKeySmallHeight = 18;
static const int WhiteKeyBigHeight = 24;
static const int BlackKeyHeightHeight = 16;
static const int CKeyLabelX = WhiteKeyWidth - 19;
static const int KeyLineHeight = 12;
static const int OctaveHeight = KeyLineHeight * KeysPerOctave;	// = 12 * 12;

static const int NoteEditResizeBar = 6;
static const int NoteEditMinHeight = 50;
static const int KeyAreaMinHeight = 100;
static const int BottomMargin = ScrollBarSize;
static const int TopMargin = 32 + FLUIQ::CollapsibleWidgetHeader::MinimalHeight;
static const int RightMargin = ScrollBarSize;


// width of area used for resizing (the grip at the end of a note)
static const int NoteResizeAreaWidth = 4;

// width of line for setting volume/panning of note
static const int NoteEditLineWidth = 3;

// key where to start
static const int InitialStartKey = Key_C + Octave_3 * KeysPerOctave;

// number of each note to provide in quantization and note lengths
static const int NumEvenLengths = 6;
static const int NumTripletLengths = 5;

static const int DefaultPixelsPerTact = KeyLineHeight * DefaultStepsPerTact;


QPixmap * pianoRoll::s_whiteKeySmallPm = NULL;
QPixmap * pianoRoll::s_whiteKeyBigPm = NULL;
QPixmap * pianoRoll::s_blackKeyPm = NULL;
QPixmap * pianoRoll::s_toolDraw = NULL;
QPixmap * pianoRoll::s_toolErase = NULL;
QPixmap * pianoRoll::s_toolSelect = NULL;
QPixmap * pianoRoll::s_toolMove = NULL;
QPixmap * pianoRoll::s_toolOpen = NULL;

// used for drawing of piano
pianoRoll::KeyTypes pianoRoll::s_keyOrder[] =
{
	WhiteKeySmall, BlackKey, WhiteKeyBig, BlackKey,
	WhiteKeySmall, WhiteKeySmall, BlackKey, WhiteKeyBig,
	BlackKey, WhiteKeyBig, BlackKey, WhiteKeySmall
} ;




pianoRoll::pianoRoll() :
	m_nemStr( QVector<QString>() ),
	m_noteEditMenu( NULL ),
	m_signalMapper( NULL ),
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
	m_ppt( DefaultPixelsPerTact ),
	m_lenOfNewNotes( midiTime( 0, DefaultTicksPerTact/4 ) ),
	m_lastNoteVolume( DefaultVolume ),
	m_lastNotePanning( DefaultPanning ),
	m_startKey( InitialStartKey ),
	m_lastKey( 0 ),
	m_editMode( ModeDraw ),
	m_mouseDownLeft( false ),
	m_mouseDownRight( false ),
	m_scrollBack( false )
{
	// gui names of edit modes
	m_nemStr.push_back( tr( "Note Volume" ) );
	m_nemStr.push_back( tr( "Note Panning" ) );
	
	m_signalMapper = new QSignalMapper( this );
	m_noteEditMenu = new QMenu( this );
	m_noteEditMenu->clear();
	for( int i=0; i<m_nemStr.size(); ++i )
	{
		QAction * act = new QAction( m_nemStr.at( i ), this );
		connect( act, SIGNAL( triggered() ),
				m_signalMapper, SLOT( map() ) );
		m_signalMapper->setMapping( act, i );
		m_noteEditMenu->addAction( act );
	}
	connect( m_signalMapper, SIGNAL( mapped( int ) ), 
			this, SLOT( changeNoteEditMode( int ) ) );
	
	// init pixmaps
	if( s_whiteKeySmallPm == NULL )
	{
		s_whiteKeySmallPm = new QPixmap( embed::getIconPixmap(
						"pr_white_key_small" ) );
	}
	if( s_whiteKeyBigPm == NULL )
	{
		s_whiteKeyBigPm = new QPixmap( embed::getIconPixmap(
							"pr_white_key_big" ) );
	}
	if( s_blackKeyPm == NULL )
	{
		s_blackKeyPm = new QPixmap( embed::getIconPixmap(
							"pr_black_key" ) );
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
	m_timeLine = new timeLine( WhiteKeyWidth, 32, m_ppt,
					engine::getSong()->getPlayPos(
						song::Mode_PlayPattern ),
						m_currentPosition, this );
	connect( this, SIGNAL( positionChanged( const midiTime & ) ),
		m_timeLine, SLOT( updatePosition( const midiTime & ) ) );
	connect( m_timeLine, SIGNAL( positionChanged( const midiTime & ) ),
			this, SLOT( updatePosition( const midiTime & ) ) );

	// update timeline when in record-accompany mode
	connect( engine::getSong()->getPlayPos( song::Mode_PlaySong ).m_timeLine,
				SIGNAL( positionChanged( const midiTime & ) ),
			this,
			SLOT( updatePositionAccompany( const midiTime & ) ) );
	// TODO
/*	connect( engine::getSong()->getPlayPos( song::Mode_PlayBB ).m_timeLine,
				SIGNAL( positionChanged( const midiTime & ) ),
			this,
			SLOT( updatePositionAccompany( const midiTime & ) ) );*/


	m_toolBar = new QWidget( this );
	m_toolBar->setObjectName( "toolbar" );
	m_toolBar->setFixedHeight( 32 );
	m_toolBar->move( 0, 0 );
	m_toolBar->setAutoFillBackground( true );

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
	m_zoomingComboBox->setFixedSize( 80, 22 );

	// setup quantize-stuff
	QLabel * quantize_lbl = new QLabel( m_toolBar );
	quantize_lbl->setPixmap( embed::getIconPixmap( "quantize" ) );

	m_quantizeModel.addItem( tr( "Note lock" ) );
	for( int i = 0; i <= NumEvenLengths; ++i )
	{
		m_quantizeModel.addItem( "1/" + QString::number( 1 << i ) );
	}
	for( int i = 0; i < NumTripletLengths; ++i )
	{
		m_quantizeModel.addItem( "1/" + QString::number( (1 << i) * 3 ) );
	}
	m_quantizeModel.addItem( "1/192" );
	m_quantizeModel.setValue( m_quantizeModel.findText( "1/16" ) );
	m_quantizeComboBox = new comboBox( m_toolBar );
	m_quantizeComboBox->setModel( &m_quantizeModel );
	m_quantizeComboBox->setFixedSize( 80, 22 );
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

	for( int i = 0; i < NumEvenLengths; ++i )
	{
		m_noteLenModel.addItem( "1/" + QString::number( 1 << i ),
				new PixmapLoader( "note_" + pixmaps[i] ) );
	}
	for( int i = 0; i < NumTripletLengths; ++i )
	{
		m_noteLenModel.addItem( "1/" + QString::number( (1 << i) * 3 ),
				new PixmapLoader( "note_" + pixmaps[i+NumEvenLengths] ) );
	}
	m_noteLenModel.setValue( 0 );
	m_noteLenComboBox = new comboBox( m_toolBar );
	m_noteLenComboBox->setModel( &m_noteLenModel );
	m_noteLenComboBox->setFixedSize( 105, 22 );
	// Note length change can cause a redraw if Q is set to lock
	connect( &m_noteLenModel, SIGNAL( dataChanged() ),
					this, SLOT( quantizeChanged() ) );


	tb_layout->addSpacing( 5 );
	tb_layout->addWidget( m_playButton );
	tb_layout->addWidget( m_recordButton );
	tb_layout->addWidget( m_recordAccompanyButton );
	tb_layout->addWidget( m_stopButton );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( m_drawButton );
	tb_layout->addWidget( m_eraseButton );
	tb_layout->addWidget( m_selectButton );
	tb_layout->addWidget( m_detuneButton );
	tb_layout->addSpacing( 10 );
	tb_layout->addWidget( m_cutButton );
	tb_layout->addWidget( m_copyButton );
	tb_layout->addWidget( m_pasteButton );
	tb_layout->addSpacing( 10 );
	m_timeLine->addToolButtons( m_toolBar );
	tb_layout->addSpacing( 15 );
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
		parentWidget()->resize( InitialPianoRollWidth,
						InitialPianoRollHeight );
		parentWidget()->hide();
	}
	else
	{
		resize( InitialPianoRollWidth, InitialPianoRollHeight );
		hide();
	}

	connect( engine::getSong(), SIGNAL( timeSignatureChanged( int, int ) ),
						this, SLOT( update() ) );
}




pianoRoll::~pianoRoll()
{
}




void pianoRoll::changeNoteEditMode( int i )
{
	m_noteEditMode = (NoteEditMode) i;
	repaint();
}




void pianoRoll::setCurrentPattern( pattern * _new_pattern )
{
	if( validPattern() )
	{
		m_pattern->instrumentTrack()->disconnect( this );
	}

	m_pattern = _new_pattern;
	m_currentPosition = 0;
	m_currentNote = NULL;
	m_startKey = InitialStartKey;

	if( validPattern() == false )
	{
		//resizeEvent( NULL );
		setWindowTitle( tr( "Piano-Roll - no pattern" ) );

		update();
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

	connect( m_pattern->instrumentTrack(),
			SIGNAL( noteOn( const note & ) ),
			this, SLOT( startRecordNote( const note & ) ) );
	connect( m_pattern->instrumentTrack(),
			SIGNAL( noteOff( const note & ) ),
			this, SLOT( finishRecordNote( const note & ) ) );

	setWindowTitle( tr( "Piano-Roll - %1" ).arg( m_pattern->name() ) );

	update();
}




void pianoRoll::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	MainWindow::saveWidgetState( this, _this );
}




void pianoRoll::loadSettings( const QDomElement & _this )
{
	MainWindow::restoreWidgetState( this, _this );
}




inline void pianoRoll::drawNoteRect( QPainter & _p, int _x, int _y,
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

	const QColor defaultNoteColor =
			engine::getLmmsStyle()->color( LmmsStyle::PianoRollDefaultNote );
	QColor col = defaultNoteColor;
	
	if( _n->length() < 0 )
	{
		//step note
		col = engine::getLmmsStyle()->color( LmmsStyle::PianoRollStepNote );
		_p.fillRect( _x, _y, _width, KeyLineHeight - 2, col );
	}
	else if( _n->isSelected() )
	{
		col = engine::getLmmsStyle()->color( LmmsStyle::PianoRollSelectedNote );
		_p.fillRect( _x, _y, _width, KeyLineHeight - 2, col );
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
							_y+KeyLineHeight );
		gradient.setColorAt( 0, lcol );
		gradient.setColorAt( 1, rcol );
		_p.setBrush( gradient );
		_p.setPen( Qt::NoPen );
		_p.drawRect( _x, _y, _width, KeyLineHeight-1 );
	}
	
	// hilighting lines around the note
	_p.setPen( Qt::SolidLine );
	_p.setBrush( Qt::NoBrush );
	
	col = defaultNoteColor;
	_p.setPen( QColor::fromHsv( col.hue(), col.saturation(),
					qMin<float>( 255, volVal*1.7f ) ) );
	_p.drawLine( _x, _y, _x + _width, _y );
	_p.drawLine( _x, _y, _x, _y + KeyLineHeight - 2 );
	
	col = defaultNoteColor;
	_p.setPen( QColor::fromHsv( col.hue(), col.saturation(), volVal/1.7 ) );
	_p.drawLine( _x + _width, _y, _x + _width, _y + KeyLineHeight - 2 );
	_p.drawLine( _x, _y + KeyLineHeight - 2, _x + _width,
						_y + KeyLineHeight - 2 );
	
	// that little tab thing on the end hinting at the user
	// to resize the note
	_p.setPen( defaultNoteColor.lighter( 200 ) );
	if( _width > 2 )
	{
		_p.drawLine( _x + _width - 3, _y + 2, _x + _width - 3,
						_y + KeyLineHeight - 4 );
	}
	_p.drawLine( _x + _width - 1, _y + 2, _x + _width - 1,
						_y + KeyLineHeight - 4 );
	_p.drawLine( _x + _width - 2, _y + 2, _x + _width - 2,
						_y + KeyLineHeight - 4 );
}




inline void pianoRoll::drawDetuningInfo( QPainter & _p, note * _n, int _x,
								int _y )
{
	int middle_y = _y + KeyLineHeight / 2;
	_p.setPen( QColor( 0xFF, 0xDF, 0x20 ) );

	timeMap & map = _n->detuning()->automationPattern()->getTimeMap();
	for( timeMap::ConstIterator it = map.begin(); it != map.end(); ++it )
	{
		Sint32 pos_ticks = it.key();
		if( pos_ticks > _n->length() )
		{
			break;
		}
		int pos_x = _x + pos_ticks * m_ppt / midiTime::ticksPerTact();

		const float level = it.value();

		int pos_y = (int)( middle_y - level * KeyLineHeight / 10 );

		_p.drawLine( pos_x - 1, pos_y, pos_x + 1, pos_y );
		_p.drawLine( pos_x, pos_y - 1, pos_x, pos_y + 1 );
	}
}




void pianoRoll::removeSelection()
{
	m_selectStartTick = 0;
	m_selectedTick = 0;
	m_selectStartKey = 0;
	m_selectedKeys = 0;
	
	
}




void pianoRoll::clearSelectedNotes()
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




void pianoRoll::closeEvent( QCloseEvent * _ce )
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




void pianoRoll::shiftSemiTone( int amount ) // shift notes by amount semitones
{
	bool useAllNotes = ! isSelection();
	const NoteVector & notes = m_pattern->notes();
	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end();
									++it )
	{
		// if none are selected, move all notes, otherwise
		// only move selected notes
		if( useAllNotes || ( *it )->isSelected() )
		{
			( *it )->setKey( ( *it )->key() + amount );
		}
	}
	
	// we modified the song
	update();
	engine::getSongEditor()->update();
	
}




void pianoRoll::shiftPos( int amount ) //shift notes pos by amount
{
	bool useAllNotes = ! isSelection();
	const NoteVector & notes = m_pattern->notes();
	
	bool first = true;
	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end();
									++it )
	{
		// if none are selected, move all notes, otherwise
		// only move selected notes
		if( ( *it )->isSelected() || (useAllNotes && ( *it )->length() > 0) )
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
	engine::getSongEditor()->update();
}




bool pianoRoll::isSelection() const // are any notes selected?
{
	const NoteVector & notes = m_pattern->notes();
	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end();
									++it )
	{
		if( ( *it )->isSelected() )
		{
			return true;
		}
	}
	
	return false;
}



int pianoRoll::selectionCount() const // how many notes are selected?
{
	int sum = 0;
	
	const NoteVector & notes = m_pattern->notes();
	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end();
									++it )
	{
		if( ( *it )->isSelected() )
		{
			++sum;
		}
	}
	
	return sum;
}



void pianoRoll::keyPressEvent( QKeyEvent * _ke )
{
	if( validPattern() && _ke->modifiers() == Qt::NoModifier )
	{
		const int key_num = PianoView::getKeyFromKeyEvent( _ke ) +
					( DefaultOctave - 1 ) * KeysPerOctave;

		if( _ke->isAutoRepeat() == false && key_num > -1 )
		{
			m_pattern->instrumentTrack()->pianoModel()->
													handleKeyPress( key_num );
			_ke->accept();
		}
	}

	switch( _ke->key() )
	{
		case Qt::Key_Up:
			if( ( _ke->modifiers() & Qt::ControlModifier )
				&& m_action == ActionNone )
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
							_ke->modifiers() &
							Qt::AltModifier );
				}
			}
			_ke->accept();
			break;

		case Qt::Key_Down:
			if( _ke->modifiers() & Qt::ControlModifier 
			   && m_action == ActionNone )
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
							_ke->modifiers() &
							Qt::AltModifier );
				}
			}
			_ke->accept();
			break;

		case Qt::Key_Left:
			if( _ke->modifiers() & Qt::ControlModifier &&
							m_action == ActionNone )
			{
				// move time ticker
				if( ( m_timeLine->pos() -= 16 ) < 0 )
				{
					m_timeLine->pos().setTicks( 0 );
				}
				m_timeLine->updatePosition();
			}
			else if( _ke->modifiers() & Qt::ShiftModifier 
					&& m_action == ActionNone)
			{
				// move notes
				bool quantized = ! ( _ke->modifiers() &
							Qt::AltModifier );
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
							_ke->modifiers() &
							Qt::AltModifier );
				}				
				
			}
			_ke->accept();
			break;

		case Qt::Key_Right:
			if( _ke->modifiers() & Qt::ControlModifier 
			   && m_action == ActionNone)
			{
				// move time ticker
				m_timeLine->pos() += 16;
				m_timeLine->updatePosition();
			}
			else if( _ke->modifiers() & Qt::ShiftModifier 
					&& m_action == ActionNone)
			{
				// move notes
				bool quantized = !( _ke->modifiers() &
							Qt::AltModifier );
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
							_ke->modifiers() &
							Qt::AltModifier );
				}				
				
			}
			_ke->accept();
			break;

		case Qt::Key_C:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				_ke->accept();
				copySelectedNotes();
			}
			break;

		case Qt::Key_X:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				_ke->accept();
				cutSelectedNotes();
			}
			break;

		case Qt::Key_V:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				_ke->accept();
				pasteNotes();
			}
			break;

		case Qt::Key_A:
			if( _ke->modifiers() & Qt::ControlModifier )
			{
				_ke->accept();
				m_selectButton->setChecked( true );
				selectAll();
				update();
			}
			break;

		case Qt::Key_D:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				_ke->accept();
				m_drawButton->setChecked( true );
			}
			break;

		case Qt::Key_E:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				_ke->accept();
				m_eraseButton->setChecked( true );
			}
			break;

		case Qt::Key_S:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				_ke->accept();
				m_selectButton->setChecked( true );
			}
			break;
			
		case Qt::Key_T:
			if( _ke->modifiers() & Qt::ShiftModifier )
			{
				_ke->accept();
				m_detuneButton->setChecked( true );
			}
			break;
			
		case Qt::Key_Delete:
			deleteSelectedNotes();
			_ke->accept();
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
			_ke->accept();
			break;

		case Qt::Key_Home:
			m_timeLine->pos().setTicks( 0 );
			m_timeLine->updatePosition();
			_ke->accept();
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
			int len = 1 + _ke->key() - Qt::Key_0;
			if( len == 10 ) 
				len = 0;
			if( _ke->modifiers() & 
				( Qt::ControlModifier | Qt::KeypadModifier ) )
			{
				m_noteLenModel.setValue( len );
				_ke->accept();
			}
			else if( _ke->modifiers() & Qt::AltModifier )
			{
				m_quantizeModel.setValue( len );
				_ke->accept();
			}
			break;
		}

		case Qt::Key_Control:
			m_ctrlMode = m_editMode;
			m_editMode = ModeSelect;
			QApplication::changeOverrideCursor( Qt::ArrowCursor );
			update();
			_ke->accept();
			break;

		default:
			break;
	}
}




void pianoRoll::keyReleaseEvent( QKeyEvent * _ke )
{
	if( validPattern() && _ke->modifiers() == Qt::NoModifier )
	{
		const int key_num = PianoView::getKeyFromKeyEvent( _ke ) +
					( DefaultOctave - 1 ) * KeysPerOctave;

		if( _ke->isAutoRepeat() == false && key_num > -1 )
		{
			m_pattern->instrumentTrack()->pianoModel()->
													handleKeyRelease( key_num );
			_ke->accept();
		}
	}
	switch( _ke->key() )
	{
		case Qt::Key_Control:
			computeSelectedNotes( _ke->modifiers() &
							Qt::ShiftModifier);
			m_editMode = m_ctrlMode;
			update();
			break;
	}
}




void pianoRoll::leaveEvent( QEvent * _e )
{
	while( QApplication::overrideCursor() != NULL )
	{
		QApplication::restoreOverrideCursor();
	}

	QWidget::leaveEvent( _e );
}




inline int pianoRoll::noteEditTop() const
{
	return height() - BottomMargin - 
		m_notesEditHeight + NoteEditResizeBar;
}




inline int pianoRoll::noteEditBottom() const
{
	return height() - BottomMargin;
}




inline int pianoRoll::noteEditRight() const
{
	return width() - RightMargin;
}




inline int pianoRoll::noteEditLeft() const
{
	return WhiteKeyWidth;
}




inline int pianoRoll::keyAreaTop() const
{
	return TopMargin;
}




inline int pianoRoll::keyAreaBottom() const
{
	return height() - BottomMargin - m_notesEditHeight;
}




void pianoRoll::mousePressEvent( QMouseEvent * _me )
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
		QApplication::changeOverrideCursor(
						QCursor( Qt::ArrowCursor ) );
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

	if( _me->y() > TopMargin )
	{
		bool edit_note = ( _me->y() > noteEditTop() );

		int key_num = getKey( _me->y() );

		int x = _me->x();


		if( x > WhiteKeyWidth )
		{
			// set, move or resize note

			x -= WhiteKeyWidth;

			// get tick in which the user clicked
			int pos_ticks = x * midiTime::ticksPerTact() / m_ppt +
							m_currentPosition;


			// get note-vector of current pattern
			const NoteVector & notes = m_pattern->notes();

			// will be our iterator in the following loop
			NoteVector::ConstIterator it = notes.begin()+notes.size()-1;

			// loop through whole note-vector...
			for( int i = 0; i < notes.size(); ++i )
			{
				midiTime len = ( *it )->length();
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
							NoteEditLineWidth *
						midiTime::ticksPerTact() /
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
				note * created_new_note = NULL;
				// did it reach end of vector because
				// there's no note??
				if( it == notes.begin()-1 )
				{
					m_pattern->setType( pattern::MelodyPattern );

					// then set new note
					
					// clear selection and select this new note
					clearSelectedNotes();
					
					// +32 to quanitize the note correctly when placing notes with
					// the mouse.  We do this here instead of in note.quantized
					// because live notes should still be quantized at the half.
					midiTime note_pos( pos_ticks - ( quantization() / 2 ) );
					midiTime note_len( newNoteLen() );
		
					note new_note( note_len, note_pos, key_num );
					new_note.setSelected( true );
					new_note.setPanning( m_lastNotePanning );
					new_note.setVolume( m_lastNoteVolume );
					created_new_note = m_pattern->addNote( new_note );

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
					
					if( ( *it )->isSelected() )
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
				if( ! m_currentNote->isSelected() )
				{
					clearSelectedNotes();
					m_currentNote->setSelected( true );
					m_moveBoundaryLeft = m_currentNote->pos().getTicks();
					m_moveBoundaryRight = m_currentNote->pos() + m_currentNote->length();
					m_moveBoundaryBottom = m_currentNote->key();
					m_moveBoundaryTop = m_currentNote->key();	
				}

				
				// clicked at the "tail" of the note?
				if( pos_ticks*m_ppt/midiTime::ticksPerTact() >
					( m_currentNote->pos() +
					m_currentNote->length() )*m_ppt/
						midiTime::ticksPerTact() -
							NoteResizeAreaWidth &&
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
					   _me->modifiers() & Qt::ShiftModifier )
					{
						// vector to hold new notes until we're through the loop
						QVector<note> newNotes;
						it = notes.begin();
						while( it != notes.end() )
						{
							if( ( *it )->isSelected() )
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
							engine::getSongEditor()->update();
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
			// clicked on keyboard on the left - play note
			m_lastKey = key_num;
			if( ! m_recording && ! engine::getSong()->isPlaying() )
			{
				int v = ( (float) x ) / ( (float) WhiteKeyWidth ) * 127;
				m_pattern->instrumentTrack()->processInEvent(
							midiEvent( MidiNoteOn, 0, key_num, v ),
							midiTime() );
			}
		}
		else
		{
			if( _me->buttons() == Qt::LeftButton )
			{
				// clicked in the box below the keys to the left of note edit area
				m_noteEditMode = (NoteEditMode)(((int)m_noteEditMode)+1);
				if( m_noteEditMode == NoteEditCount )
				{
					m_noteEditMode = (NoteEditMode)0;
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




void pianoRoll::mouseDoubleClickEvent( QMouseEvent * _me )
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




void pianoRoll::testPlayNote( note * n )
{
	m_lastKey = n->key();
	
	if( ! n->isPlaying() && ! m_recording &&
					! engine::getSong()->isPlaying() )
	{
		n->setIsPlaying( true );
		m_pattern->instrumentTrack()->processInEvent(
			midiEvent( MidiNoteOn, 0, n->key(), 
				  n->getVolume() * 127 / 100 ), midiTime() );
		
		midiEvent evt( MidiMetaEvent, 0, n->key(), 
				panningToMidi( n->getPanning() ) );
		
		evt.m_metaEvent = MidiNotePanning;
		m_pattern->instrumentTrack()->processInEvent( evt, midiTime() );
	}
}




void pianoRoll::pauseTestNotes( bool _pause )
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
				m_pattern->instrumentTrack()->
					processInEvent(
						midiEvent( MidiNoteOff, 0, 
							  ( *it )->key(), 0 ),
								midiTime() );
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




void pianoRoll::testPlayKey( int _key, int _vol, int _pan )
{
	// turn off old key
	m_pattern->instrumentTrack()->processInEvent(
				midiEvent( MidiNoteOff, 0, m_lastKey, 0 ),
								midiTime() );
	
	// remember which one we're playing
	m_lastKey = _key;
	
	// play new key
	m_pattern->instrumentTrack()->processInEvent(
				midiEvent( MidiNoteOn, 0, _key, _vol ),
								midiTime() );
	
	// set panning of newly played key
	midiEvent evt( MidiMetaEvent, 0, _key, _pan );
	evt.m_metaEvent = MidiNotePanning;
}




void pianoRoll::computeSelectedNotes(bool shift)
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
			
			Sint32 len_ticks = ( *it )->length();

			if( len_ticks == 0 )
			{
				continue;
			}
			else if( len_ticks < 0 )
			{
				len_ticks = 4;
			}

			const int key = ( *it )->key() - m_startKey + 1;

			Sint32 pos_ticks = ( *it )->pos();

			// if the selection even barely overlaps the note
			if( key > sel_key_start &&
				key <= sel_key_end &&
				pos_ticks + len_ticks > sel_pos_start &&
				pos_ticks < sel_pos_end )
			{
				( *it )->setSelected( true );
			}
		}
	}
	
	removeSelection();
	update();
}




void pianoRoll::mouseReleaseEvent( QMouseEvent * _me )
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
				m_pattern->instrumentTrack()->
					processInEvent(
						midiEvent( MidiNoteOff, 0,
							( *it )->key(), 0 ), 
								midiTime() );
				( *it )->setIsPlaying( false );
			}
			
			++it;
		}
		
		// stop playing keys that we let go of
		m_pattern->instrumentTrack()->processInEvent(
				midiEvent( MidiNoteOff, 0, m_lastKey, 0 ),
								midiTime() );
				
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




void pianoRoll::mouseMoveEvent( QMouseEvent * _me )
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
					NoteEditMinHeight,
					height() - TopMargin - NoteEditResizeBar - 
						BottomMargin - KeyAreaMinHeight );
		repaint();
		return;
	}
	
	if( _me->y() > TopMargin || m_action != ActionNone )
	{
		bool edit_note = ( _me->y() > noteEditTop() )
						&& m_action != ActionSelectNotes;
		

		int key_num = getKey( _me->y() );
		int x = _me->x();

		// see if they clicked on the keyboard on the left
		if( x < WhiteKeyWidth && m_action == ActionNone
		    && ! edit_note && key_num != m_lastKey
		    && _me->buttons() & Qt::LeftButton )
		{
			// clicked on a key, play the note
			testPlayKey( key_num, 
						( (float) x ) / ( (float) WhiteKeyWidth ) * 127,
						0 );
			update();
			return;
		}
		
		x -= WhiteKeyWidth;

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
			
			dragNotes(_me->x(), _me->y(), _me->modifiers() & Qt::AltModifier);
			
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
					midiTime::ticksPerTact() / m_ppt + m_currentPosition;
			int ticks_end = (x+pixel_range/2) * 
					midiTime::ticksPerTact() / m_ppt + m_currentPosition;

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
					&& ( n->isSelected() || ! use_selection ) )
				{
					m_pattern->dataChanged();
					
					// play the note so that the user can tell how loud it is
					// and where it is panned
					testPlayNote( n );
					
					if( m_noteEditMode == NoteEditVolume )
					{
						n->setVolume( vol );
						m_pattern->instrumentTrack()->processInEvent(
							midiEvent( 
							  MidiKeyPressure, 
							  0, 
							  n->key(), 
							  vol * 127 / 100),
									midiTime() );
					}
					else if( m_noteEditMode == NoteEditPanning )
					{
						n->setPanning( pan );
						midiEvent evt( MidiMetaEvent, 0, 
										  n->key(), panningToMidi( pan ) );
						evt.m_metaEvent = MidiNotePanning;
						m_pattern->instrumentTrack()->processInEvent(
									evt, midiTime() );
					}
				}
				else
				{
					if( n->isPlaying() )
					{
						// mouse not over this note, stop playing it.
						m_pattern->instrumentTrack()->processInEvent(
							midiEvent( MidiNoteOff, 0,
										n->key(), 0 ), midiTime() );
						
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
			int pos_ticks = ( x * midiTime::ticksPerTact() ) /
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
						midiTime::ticksPerTact() >
						( ( *it )->pos() +
						( *it )->length() )*m_ppt/
						midiTime::ticksPerTact()-
							NoteResizeAreaWidth )
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
			int pos_ticks = x * midiTime::ticksPerTact() / m_ppt +
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
			int pos_ticks = x * midiTime::ticksPerTact() / m_ppt +
							m_currentPosition;


			// get note-vector of current pattern
			const NoteVector & notes = m_pattern->notes();

			// will be our iterator in the following loop
			NoteVector::ConstIterator it = notes.begin();

			// loop through whole note-vector...
			while( it != notes.end() )
			{
				midiTime len = ( *it )->length();
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
							NoteEditLineWidth *
						midiTime::ticksPerTact() /
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

			int x = _me->x() - WhiteKeyWidth;
			if( x < 0 && m_currentPosition > 0 )
			{
				x = 0;
				QCursor::setPos( mapToGlobal( QPoint(
							WhiteKeyWidth,
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
			else if( x > width() - WhiteKeyWidth )
			{
				x = width() - WhiteKeyWidth;
				QCursor::setPos( mapToGlobal( QPoint( width(),
							_me->y() ) ) );
				m_leftRightScroll->setValue( m_currentPosition +
									4 );
			}

			// get tick in which the cursor is posated
			int pos_ticks = x * midiTime::ticksPerTact()/ m_ppt +
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
			int visible_keys = ( height() - TopMargin -
						BottomMargin -
						m_notesEditHeight ) /
							KeyLineHeight + 2;
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
							TopMargin ) ) );
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




void pianoRoll::dragNotes( int x, int y, bool alt )
{
	// dragging one or more notes around
		
	// convert pixels to ticks and keys
	int off_x = x - m_moveStartX;
	int off_ticks = off_x * midiTime::ticksPerTact() / m_ppt;
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
	
	// get note-vector of current pattern
	const NoteVector & notes = m_pattern->notes();

	// will be our iterator in the following loop
	NoteVector::ConstIterator it = notes.begin();
	while( it != notes.end() )
	{
		if( ( *it )->isSelected() )
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
			
				( *it )->setPos( midiTime( pos_ticks ) );
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
				( *it )->setLength( midiTime( ticks_new ) );
				
				m_lenOfNewNotes = ( *it )->length();
			}
		}
		++it;
	}
	
	m_pattern->dataChanged();
	engine::getSong()->setModified();
}



void pianoRoll::paintEvent( QPaintEvent * _pe )
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
	switch( s_keyOrder[m_startKey % KeysPerOctave] )
	{
		case BlackKey: y_offset = KeyLineHeight/4; break;
		case WhiteKeyBig: y_offset = KeyLineHeight/2; break;
		case WhiteKeySmall:
			if( s_keyOrder[( ( m_startKey + 1 ) %
					KeysPerOctave)] != BlackKey )
			{
				y_offset = KeyLineHeight / 2;
			}
			break;
	}

	// start drawing at the bottom
	int key_line_y = keyAreaBottom() - 1;
	// used for aligning black-keys later
	int first_white_key_height = WhiteKeySmallHeight;
	// key-counter - only needed for finding out whether the processed 
	// key is the first one
	int keys_processed = 0;

	int key = m_startKey;

	// draw all white keys...
	for( int y = key_line_y + 1 + y_offset; y > TopMargin;
			key_line_y -= KeyLineHeight, ++keys_processed )
	{
		// check for white key that is only half visible on the 
		// bottom of piano-roll
		if( keys_processed == 0 &&
			s_keyOrder[m_startKey % KeysPerOctave] ==
								BlackKey )
		{
			// draw it!
			p.drawPixmap( PianoX, y - WhiteKeySmallHeight,
							*s_whiteKeySmallPm );
			// update y-pos
			y -= WhiteKeySmallHeight / 2;
			// move first black key down (we didn't draw whole 
			// white key so black key needs to be lifted down)
			// (default for first_white_key_height = 
			// WhiteKeySmallHeight, so WhiteKeySmallHeight/2
			// is smaller)
			first_white_key_height = WhiteKeySmallHeight / 2;
		}
		// check whether to draw a big or a small white key
		if( s_keyOrder[key % KeysPerOctave] == WhiteKeySmall )
		{
			// draw a small one...
			p.drawPixmap( PianoX, y - WhiteKeySmallHeight,
							*s_whiteKeySmallPm );
			// update y-pos
			y -= WhiteKeySmallHeight;

		}
		else if( s_keyOrder[key % KeysPerOctave] ==
							WhiteKeyBig )
		{
			// draw a big one...
			p.drawPixmap( PianoX, y-WhiteKeyBigHeight,
							*s_whiteKeyBigPm );
			// if a big white key has been the first key,
			// black keys needs to be lifted up
			if( keys_processed == 0 )
			{
				first_white_key_height = WhiteKeyBigHeight;
			}
			// update y-pos
			y -= WhiteKeyBigHeight;
		}
		// label C-keys...
		if( static_cast<Keys>( key % KeysPerOctave ) == Key_C )
		{
			p.setPen( QColor( 240, 240, 240 ) );
			p.drawText( CKeyLabelX + 1, y+14, "C" +
					QString::number( static_cast<int>( key /
							KeysPerOctave ) ) );
			p.setPen( QColor( 0, 0, 0 ) );
			p.drawText( CKeyLabelX, y + 13, "C" +
					QString::number( static_cast<int>( key /
							KeysPerOctave ) ) );
			p.setPen( QColor( 0x4F, 0x4F, 0x4F ) );
		}
		else
		{
			p.setPen( QColor( 0x3F, 0x3F, 0x3F ) );
		}
		// draw key-line
		p.drawLine( WhiteKeyWidth, key_line_y, width(), key_line_y );
		++key;
	}

	// reset all values, because now we're going to draw all black keys
	key = m_startKey;
	keys_processed = 0;
	int white_cnt = 0;

	// and go!
	for( int y = keyAreaBottom() + y_offset;
					y > TopMargin; ++keys_processed )
	{
		// check for black key that is only half visible on the bottom
		// of piano-roll
		if( keys_processed == 0
		    // current key may not be a black one
		    && s_keyOrder[key % KeysPerOctave] != BlackKey
		    // but the previous one must be black (we must check this
		    // because there might be two white keys (E-F)
		    && s_keyOrder[( key - 1 ) % KeysPerOctave] ==
								BlackKey )
		{
			// draw the black key!
			p.drawPixmap( PianoX, y - BlackKeyHeightHeight / 2,
								*s_blackKeyPm );
			// is the one after the start-note a black key??
			if( s_keyOrder[( key + 1 ) % KeysPerOctave] !=
								BlackKey )
			{
				// no, then move it up!
				y -= KeyLineHeight / 2;
			}
		}
		// current key black?
		if( s_keyOrder[key % KeysPerOctave] == BlackKey )
		{
			// then draw it (calculation of y very complicated,
			// but that's the only working solution, sorry...)
			p.drawPixmap( PianoX, y - ( first_white_key_height -
					WhiteKeySmallHeight ) -
					WhiteKeySmallHeight/2 - 1 -
					BlackKeyHeightHeight, *s_blackKeyPm );

			// update y-pos
			y -= WhiteKeyBigHeight;
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
				y -= WhiteKeyBigHeight/2;
			}
		}

		++key;
	}


	// erase the area below the piano, because there might be keys that 
	// should be only half-visible
	p.fillRect( QRect( 0, keyAreaBottom(),
			WhiteKeyWidth, noteEditBottom()-keyAreaBottom() ),
			QColor( 0, 0, 0 ) );
	
	// display note editing info
	QFont f = p.font();
	f.setBold( false );
	p.setFont( pointSize<10>( f ) );
	p.setPen( QColor( 255, 255, 0 ) );
	p.drawText( QRect( 0, keyAreaBottom(), 
					  WhiteKeyWidth, noteEditBottom() - keyAreaBottom() ),
			   Qt::AlignCenter | Qt::TextWordWrap,
			   m_nemStr.at( m_noteEditMode ) + ":" );

	// set clipping area, because we are not allowed to paint over
	// keyboard...
	p.setClipRect( WhiteKeyWidth, TopMargin,
				width() - WhiteKeyWidth,
				height() - TopMargin - BottomMargin );

	// draw vertical raster
	
	// triplet mode occurs if the note duration isn't a multiple of 3
	bool triplets = ( quantization() % 3 != 0 ); 

	int spt = midiTime::stepsPerTact(); 
	float pp16th = m_ppt / spt;
	int bpt = DefaultBeatsPerTact;
	if ( triplets ) {
		spt = static_cast<int>(1.5 * spt);
		bpt = static_cast<int>(bpt * 2.0/3.0);
		pp16th *= 2.0/3.0;
	}

	int tact_16th = m_currentPosition / bpt;

	const int offset = ( m_currentPosition % bpt ) *
			m_ppt / midiTime::ticksPerTact();

	bool show32nds = ( m_zoomingModel.value() > 3 );

	// we need float here as odd time signatures might produce rounding
	// errors else and thus an unusable grid
	for( float x = WhiteKeyWidth - offset; x < width();
						x += pp16th, ++tact_16th )
	{
		if( x >= WhiteKeyWidth )
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

			p.drawLine( (int)x, TopMargin, (int)x, height() -
							BottomMargin );

			// extra 32nd's line
			if( show32nds )
			{
				p.setPen( QColor( 0x22, 0x22, 0x22 ) );
				p.drawLine( (int)(x + pp16th/2) , TopMargin, 
						(int)(x + pp16th/2), height() -
						BottomMargin );
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
		p.setClipRect( WhiteKeyWidth, TopMargin,
				width() - WhiteKeyWidth,
				height() - TopMargin );

		const NoteVector & notes = m_pattern->notes();

		const int visible_keys = ( keyAreaBottom()-keyAreaTop() ) /
							KeyLineHeight + 2;
	
		// horizontal line for the key under the cursor
		int key_num = getKey( mapFromGlobal( QCursor::pos() ).y() );
		p.fillRect( 10, keyAreaBottom() + 3 - KeyLineHeight *
					( key_num - m_startKey + 1 ),
				width() - 10, KeyLineHeight - 7,
							QColor( 64, 64, 64 ) );
	
		QPolygon editHandles;

		for( NoteVector::ConstIterator it = notes.begin();
						it != notes.end(); ++it )
		{
			Sint32 len_ticks = ( *it )->length();

			if( len_ticks == 0 )
			{
				continue;
			}
			else if( len_ticks < 0 )
			{
				len_ticks = 4;
			}

			const int key = ( *it )->key() - m_startKey + 1;

			Sint32 pos_ticks = ( *it )->pos();

			int note_width = len_ticks * m_ppt /
						midiTime::ticksPerTact();
			const int x = ( pos_ticks - m_currentPosition ) *
					m_ppt / midiTime::ticksPerTact();
			// skip this note if not in visible area at all
			if( !( x + note_width >= 0 &&
					x <= width() - WhiteKeyWidth ) )
			{
				continue;
			}

			// is the note in visible area?
			if( key > 0 && key <= visible_keys )
			{

				// we've done and checked all, let's draw the
				// note
				drawNoteRect( p, x + WhiteKeyWidth,
						y_base - key * KeyLineHeight,
								note_width, *it );
			}
			
			// draw note editing stuff
			int editHandleTop = 0;
			if( m_noteEditMode == NoteEditVolume )
			{
				QColor color = engine::getLmmsStyle()->color(
						( *it )->isSelected() ?
						LmmsStyle::PianoRollSelectedLevel : 
						LmmsStyle::PianoRollVolumeLevel );
				p.setPen( QPen( color.lighter( 
						(*it)->getVolume() / 2.0f ),
							NoteEditLineWidth ) );

				editHandleTop = noteEditBottom() - 
					( (float)( ( *it )->getVolume() - MinVolume ) ) / 
					( (float)( MaxVolume - MinVolume ) ) * 
					( (float)( noteEditBottom() - noteEditTop() ) );
				
				p.drawLine( noteEditLeft() + x, editHandleTop, 
							noteEditLeft() + x, noteEditBottom() );

			}
			else if( m_noteEditMode == NoteEditPanning )
			{
				QColor color = engine::getLmmsStyle()->color(
						( *it )->isSelected() ?
						LmmsStyle::PianoRollSelectedLevel : 
						LmmsStyle::PianoRollPanningLevel );
				
				p.setPen( QPen( color, NoteEditLineWidth ) );
				
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
					x + WhiteKeyWidth,
					y_base - key * KeyLineHeight );
			}
		}
		
		p.setPen( QPen(engine::getLmmsStyle()->color( LmmsStyle::PianoRollEditHandle ),
				NoteEditLineWidth+2 ) );
		p.drawPoints( editHandles );
		
	}
	else
	{
		QFont f = p.font();
		f.setBold( true );
		p.setFont( pointSize<14>( f ) );
		p.setPen( QColor( 0, 255, 0 ) );
		p.drawText( WhiteKeyWidth + 20, TopMargin + 40,
				tr( "Please open a pattern by double-clicking "
								"on it!" ) );
	}

	p.setClipRect( WhiteKeyWidth, TopMargin, width() -
				WhiteKeyWidth, height() - TopMargin -
					m_notesEditHeight - BottomMargin );

	// now draw selection-frame
	int x = ( ( sel_pos_start - m_currentPosition ) * m_ppt ) /
						midiTime::ticksPerTact();
	int w = ( ( ( sel_pos_end - m_currentPosition ) * m_ppt ) /
						midiTime::ticksPerTact() ) - x;
	int y = (int) y_base - sel_key_start * KeyLineHeight;
	int h = (int) y_base - sel_key_end * KeyLineHeight - y;
	p.setPen( QColor( 0, 64, 192 ) );
	p.drawRect( x + WhiteKeyWidth, y, w, h );

	// TODO: Get this out of paint event
	int l = ( validPattern() == true )? (int) m_pattern->length() : 0;

	// reset scroll-range
	if( m_leftRightScroll->maximum() != l )
	{
		m_leftRightScroll->setRange( 0, l );
		m_leftRightScroll->setPageStep( l );
	}

	// bar to resize note edit area
	p.setClipRect( 0, 0, width(), height() );
	p.fillRect( QRect( 0, keyAreaBottom(), 
					width()-RightMargin, NoteEditResizeBar ),
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
}




// responsible for moving/resizing scrollbars after window-resizing
void pianoRoll::resizeEvent( QResizeEvent * )
{
	m_leftRightScroll->setGeometry( WhiteKeyWidth, height() -
								ScrollBarSize,
					width()-WhiteKeyWidth,
							ScrollBarSize );
	m_topBottomScroll->setGeometry( width() - ScrollBarSize, TopMargin,
						ScrollBarSize,
						height() - TopMargin -
						ScrollBarSize );

	int total_pixels = OctaveHeight * NumOctaves - ( height() -
					TopMargin - BottomMargin -
							m_notesEditHeight );
	m_totalKeysToScroll = total_pixels * KeysPerOctave / OctaveHeight;

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




void pianoRoll::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	if( _we->modifiers() & Qt::ControlModifier )
	{
		if( _we->delta() > 0 )
		{
			m_ppt = qMin( m_ppt * 2, KeyLineHeight *
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
						DefaultPixelsPerTact ) ) +"%" ) );
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




int pianoRoll::getKey( int _y ) const
{
	int key_line_y = keyAreaBottom() - 1;
	// pressed key on piano
	int key_num = ( key_line_y - _y ) / KeyLineHeight;
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




song::PlayModes pianoRoll::desiredPlayModeForAccompany() const
{
	if( m_pattern->getTrack()->getTrackContainer() ==
					engine::getBBTrackContainer() )
	{
		return song::Mode_PlayBB;
	}
	return song::Mode_PlaySong;
}




void pianoRoll::play()
{
	engine::mainWindow()->setPlaybackMode( PPM_PianoRoll );

	if( validPattern() == false )
	{
		return;
	}

	if( engine::getSong()->isPlaying() )
	{
		if( engine::getSong()->playMode() != song::Mode_PlayPattern )
		{
			engine::getSong()->stop();
			engine::getSong()->playPattern( m_pattern );
			m_playButton->setIcon( embed::getIconPixmap(
								"pause" ) );
		}
		else
		{
			engine::getSong()->pause();
			m_playButton->setIcon( embed::getIconPixmap( "play" ) );
		}
	}
	else if( engine::getSong()->isPaused() )
	{
		engine::getSong()->resumeFromPause();
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
	}
	else
	{
		m_playButton->setIcon( embed::getIconPixmap( "pause" ) );
		engine::getSong()->playPattern( m_pattern );
	}
}




void pianoRoll::record()
{
	engine::mainWindow()->setPlaybackMode( PPM_PianoRoll );
	
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




void pianoRoll::recordAccompany()
{
	engine::mainWindow()->setPlaybackMode( PPM_PianoRoll );
	
	if( engine::getSong()->isPlaying() )
	{
		stop();
	}
	if( m_recording == true || validPattern() == false )
	{
		return;
	}

	m_recording = true;

	if( m_pattern->getTrack()->getTrackContainer() == engine::getSong() )
	{
		engine::getSong()->play();
	}
	else
	{
		engine::getSong()->playBB();
	}
}





void pianoRoll::stop()
{
	engine::getSong()->stop();
	m_playButton->setIcon( embed::getIconPixmap( "play" ) );
	m_playButton->update();
	m_recording = false;
	m_scrollBack = true;
}




void pianoRoll::startRecordNote( const note & _n )
{
	if( m_recording == true && validPattern() == true &&
		engine::getSong()->isPlaying() &&
			( engine::getSong()->playMode() ==
					desiredPlayModeForAccompany() ||
				engine::getSong()->playMode() ==
					song::Mode_PlayPattern ) )
	{
		midiTime sub;
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




void pianoRoll::finishRecordNote( const note & _n )
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




void pianoRoll::horScrolled( int _new_pos )
{
	m_currentPosition = _new_pos;
	emit positionChanged( m_currentPosition );
	update();
}




void pianoRoll::verScrolled( int _new_pos )
{
	// revert value
	m_startKey = m_totalKeysToScroll - _new_pos;

	update();
}




void pianoRoll::drawButtonToggled()
{
	m_editMode = ModeDraw;
	update();
}




void pianoRoll::eraseButtonToggled()
{
	m_editMode = ModeErase;
	update();
}




void pianoRoll::selectButtonToggled()
{
	m_editMode = ModeSelect;
	update();
}



void pianoRoll::detuneButtonToggled()
{
	m_editMode = ModeEditDetuning;
	update();
}



void pianoRoll::selectAll()
{
	if( validPattern() == false )
	{
		return;
	}

	const NoteVector & notes = m_pattern->notes();

	// if first_time = true, we HAVE to set the vars for select
	bool first_time = true;

	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end();
									++it )
	{
		Uint32 len_ticks = ( *it )->length();

		if( len_ticks > 0 )
		{
			const int key = ( *it )->key();

			Uint32 pos_ticks = ( *it )->pos();
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
void pianoRoll::getSelectedNotes( NoteVector & _selected_notes )
{
	if( validPattern() == false )
	{
		return;
	}

	const NoteVector & notes = m_pattern->notes();

	for( NoteVector::ConstIterator it = notes.begin(); it != notes.end();
									++it )
	{
		if( ( *it )->isSelected() )
		{
			_selected_notes.push_back( *it );
		}
	}
}




void pianoRoll::copy_to_clipboard( const NoteVector & _notes ) const
{
	multimediaProject mmp( multimediaProject::ClipboardData );
	QDomElement note_list = mmp.createElement( "note-list" );
	mmp.content().appendChild( note_list );

	midiTime start_pos( _notes.front()->pos().getTact(), 0 );
	for( NoteVector::ConstIterator it = _notes.begin(); it != _notes.end();
									++it )
	{
		note clip_note( **it );
		clip_note.setPos( clip_note.pos( start_pos ) );
		clip_note.saveState( mmp, note_list );
	}

	QMimeData * clip_content = new QMimeData;
	clip_content->setData( Clipboard::mimeType(), mmp.toString().toUtf8() );
	QApplication::clipboard()->setMimeData( clip_content,
							QClipboard::Clipboard );
}




void pianoRoll::copySelectedNotes()
{
	NoteVector selected_notes;
	getSelectedNotes( selected_notes );

	if( selected_notes.empty() == false )
	{
		copy_to_clipboard( selected_notes );
	}
}




void pianoRoll::cutSelectedNotes()
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
	engine::getSongEditor()->update();
}




void pianoRoll::pasteNotes()
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
		multimediaProject mmp( value.toUtf8() );

		QDomNodeList list = mmp.elementsByTagName(
							note::classNodeName() );
		
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
		engine::getSongEditor()->update();
	}
}




void pianoRoll::deleteSelectedNotes()
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
		if( ( *it )->isSelected() )
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
		engine::getSongEditor()->update();
	}
	
}




void pianoRoll::autoScroll( const midiTime & _t )
{
	const int w = width() - WhiteKeyWidth;
	if( _t > m_currentPosition + w * midiTime::ticksPerTact() / m_ppt )
	{
		m_leftRightScroll->setValue( _t.getTact() *
					midiTime::ticksPerTact() );
	}
	else if( _t < m_currentPosition )
	{
		midiTime t = qMax( _t - w * midiTime::ticksPerTact() *
					midiTime::ticksPerTact() / m_ppt, 0 );
		m_leftRightScroll->setValue( t.getTact() *
						midiTime::ticksPerTact() );
	}
	m_scrollBack = false;
}




void pianoRoll::updatePosition( const midiTime & _t )
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




void pianoRoll::updatePositionAccompany( const midiTime & _t )
{
	song * s = engine::getSong();

	if( m_recording && validPattern() &&
					s->playMode() != song::Mode_PlayPattern )
	{
		midiTime pos = _t;
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




void pianoRoll::zoomingChanged()
{
	const QString & zfac = m_zoomingModel.currentText();
	m_ppt = zfac.left( zfac.length() - 1 ).toInt() * DefaultPixelsPerTact / 100;
#ifdef LMMS_DEBUG
	assert( m_ppt > 0 );
#endif
	m_timeLine->setPixelsPerTact( m_ppt );
	update();

}




void pianoRoll::quantizeChanged()
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


int pianoRoll::quantization() const
{
	if( m_quantizeModel.value() == 0 )
	{
		return newNoteLen();
	}
	return DefaultTicksPerTact / m_quantizeModel.currentText().right(
				m_quantizeModel.currentText().length() -
								2 ).toInt();
}




midiTime pianoRoll::newNoteLen() const
{
	if( m_noteLenModel.value() == 0 )
	{
		return m_lenOfNewNotes;
	}
	return midiTime::ticksPerTact() / m_noteLenModel.currentText().right(
				m_noteLenModel.currentText().length() -
								2 ).toInt();
}




bool pianoRoll::mouseOverNote()
{
	return validPattern() && noteUnderMouse() != NULL;
}




note * pianoRoll::noteUnderMouse()
{
	QPoint pos = mapFromGlobal( QCursor::pos() );

	// get note-vector of current pattern
	const NoteVector & notes = m_pattern->notes();

	if( pos.x() <= WhiteKeyWidth || pos.x() > width() - ScrollBarSize
		|| pos.y() < TopMargin
		|| pos.y() > keyAreaBottom() )
	{
		return NULL;
	}

	int key_num = getKey( pos.y() );
	int pos_ticks = ( pos.x() - WhiteKeyWidth ) *
			midiTime::ticksPerTact() / m_ppt + m_currentPosition;

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




#include "moc_piano_roll.cxx"


