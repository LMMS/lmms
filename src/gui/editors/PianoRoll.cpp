/*
 * PianoRoll.cpp - implementation of piano roll which is used for actual
 *                  writing of melodies
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008 Andrew Kelley <superjoe30/at/gmail/dot/com>
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

#include "PianoRoll.h"

#include <QtMath>
#include <QApplication>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMargins>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QScrollBar>
#include <QStyleOption>
#include <QToolButton>

#include <cmath>
#include <utility>

#include "AutomationEditor.h"
#include "ActionGroup.h"
#include "Clipboard.h"
#include "ComboBox.h"
#include "ConfigManager.h"
#include "DataFile.h"
#include "DeprecationHelper.h"
#include "DetuningHelper.h"
#include "embed.h"
#include "GuiApplication.h"
#include "FontHelper.h"
#include "InstrumentTrack.h"
#include "KeyboardShortcuts.h"
#include "lmms_math.h"
#include "MainWindow.h"
#include "MidiClip.h"
#include "PatternStore.h"
#include "PianoView.h"
#include "PositionLine.h"
#include "SimpleTextFloat.h"
#include "SongEditor.h"
#include "StepRecorderWidget.h"
#include "TextFloat.h"
#include "TimeLineWidget.h"
#include "FileDialog.h"


namespace lmms
{


using timeMap = AutomationClip::timeMap;


namespace gui
{

// some constants...
const int INITIAL_PIANOROLL_WIDTH = 970;
const int INITIAL_PIANOROLL_HEIGHT = 485;

const int SCROLLBAR_SIZE = 12;
const int PIANO_X = 0;

const int WHITE_KEY_WIDTH = 64;
const int BLACK_KEY_WIDTH = 41;

const int DEFAULT_KEY_LINE_HEIGHT = 12;
const int DEFAULT_CELL_WIDTH = 12;


const int NOTE_EDIT_RESIZE_BAR = 6;
const int NOTE_EDIT_MIN_HEIGHT = 50;
const int KEY_AREA_MIN_HEIGHT = DEFAULT_KEY_LINE_HEIGHT * 10;
const int PR_BOTTOM_MARGIN = SCROLLBAR_SIZE;
const int PR_TOP_MARGIN = 18;
const int PR_RIGHT_MARGIN = SCROLLBAR_SIZE;


// width of area used for resizing (the grip at the end of a note)
const int RESIZE_AREA_WIDTH = 9;

// width of line for setting volume/panning of note
const int NOTE_EDIT_LINE_WIDTH = 3;

// key where to start
const int INITIAL_START_KEY = Octave::Octave_4 + Key::C;

// number of each note to provide in quantization and note lengths
const int NUM_EVEN_LENGTHS = 6;
const int NUM_TRIPLET_LENGTHS = 5;

SimpleTextFloat * PianoRoll::s_textFloat = nullptr;

static std::array<QString, 12> s_noteStrings {
	"C", "C\u266F / D\u266D", "D", "D\u266F / E\u266D", "E", "F", "F\u266F / G\u266D", 
	"G", "G\u266F / A\u266D", "A", "A\u266F / B\u266D", "B"
};

static QString getNoteString(int key)
{
	return s_noteStrings[key % 12] + QString::number(static_cast<int>(FirstOctave + key / KeysPerOctave));
}

// used for drawing of piano
std::array<PianoRoll::KeyType, 12> PianoRoll::prKeyOrder
{
	KeyType::WhiteSmall, KeyType::Black, KeyType::WhiteBig, KeyType::Black,
	KeyType::WhiteSmall, KeyType::WhiteSmall, KeyType::Black, KeyType::WhiteBig,
	KeyType::Black, KeyType::WhiteBig, KeyType::Black, KeyType::WhiteSmall
} ;


const int DEFAULT_PR_PPB = DEFAULT_CELL_WIDTH * DefaultStepsPerBar;

const std::vector<float> PianoRoll::m_zoomLevels =
		{0.125f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 4.0f, 8.0f};

const std::vector<float> PianoRoll::m_zoomYLevels =
		{0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f};


PianoRoll::PianoRoll() :
	m_noteEditMenu( nullptr ),
	m_semiToneMarkerMenu( nullptr ),
	m_zoomingModel(),
	m_zoomingYModel(),
	m_quantizeModel(),
	m_noteLenModel(),
	m_scaleModel(),
	m_chordModel(),
	m_midiClip( nullptr ),
	m_currentPosition(),
	m_recording( false ),
	m_doAutoQuantization(ConfigManager::inst()->value("midi", "autoquantize").toInt() != 0),
	m_currentNote( nullptr ),
	m_action( Action::None ),
	m_noteEditMode( NoteEditMode::Volume ),
	m_moveBoundaryLeft( 0 ),
	m_moveBoundaryTop( 0 ),
	m_moveBoundaryRight( 0 ),
	m_moveBoundaryBottom( 0 ),
	m_mouseDownKey( 0 ),
	m_mouseDownTick( 0 ),
	m_lastMouseX( 0 ),
	m_lastMouseY( 0 ),
	m_notesEditHeight( 100 ),
	m_userSetNotesEditHeight(100),
	m_ppb( DEFAULT_PR_PPB ),
	m_keyLineHeight(DEFAULT_KEY_LINE_HEIGHT),
	m_whiteKeySmallHeight(qFloor(m_keyLineHeight * 1.5)),
	m_whiteKeyBigHeight(m_keyLineHeight * 2),
	m_blackKeyHeight(m_keyLineHeight),
	m_lenOfNewNotes( TimePos( 0, DefaultTicksPerBar/4 ) ),
	m_lastNoteVolume( DefaultVolume ),
	m_lastNotePanning( DefaultPanning ),
	m_minResizeLen( 0 ),
	m_startKey( INITIAL_START_KEY ),
	m_lastKey( 0 ),
	m_editMode( EditMode::Draw ),
	m_ctrlMode( EditMode::Draw ),
	m_mouseDownRight( false ),
	m_scrollBack( false ),
	m_stepRecorderWidget(this, DEFAULT_PR_PPB, PR_TOP_MARGIN, PR_BOTTOM_MARGIN + m_notesEditHeight, WHITE_KEY_WIDTH, 0),
	m_stepRecorder(*this, m_stepRecorderWidget),
	m_barLineColor( 0, 0, 0 ),
	m_beatLineColor( 0, 0, 0 ),
	m_lineColor( 0, 0, 0 ),
	m_noteModeColor( 0, 0, 0 ),
	m_noteColor( 0, 0, 0 ),
	m_stepNoteColor(0, 0, 0),
	m_currentStepNoteColor(245, 3, 139),
	m_noteTextColor(0, 0, 0),
	m_ghostNoteColor( 0, 0, 0 ),
	m_ghostNoteTextColor( 0, 0, 0 ),
	m_barColor( 0, 0, 0 ),
	m_selectedNoteColor( 0, 0, 0 ),
	m_textColor( 0, 0, 0 ),
	m_textColorLight( 0, 0, 0 ),
	m_textShadow( 0, 0, 0 ),
	m_markedSemitoneColor( 0, 0, 0 ),
	m_knifeCutLineColor(0, 0, 0),
	m_noteOpacity( 255 ),
	m_ghostNoteOpacity( 255 ),
	m_noteBorders( true ),
	m_ghostNoteBorders( true ),
	m_backgroundShade( 0, 0, 0 ),
	m_outOfBoundsShade(0, 0, 0, 128),
	m_whiteKeyWidth(WHITE_KEY_WIDTH),
	m_blackKeyWidth(BLACK_KEY_WIDTH)
{
	// gui names of edit modes
	m_nemStr.push_back( tr( "Note Velocity" ) );
	m_nemStr.push_back( tr( "Note Panning" ) );

	m_noteEditMenu = new QMenu( this );
	m_noteEditMenu->clear();
	for (auto i = std::size_t{0}; i < m_nemStr.size(); ++i)
	{
		auto act = new QAction(m_nemStr.at(i), this);
		connect( act, &QAction::triggered, [this, i](){ changeNoteEditMode(i); } );
		m_noteEditMenu->addAction( act );
	}

	m_semiToneMarkerMenu = new QMenu( this );

	auto markSemitoneAction = new QAction(tr("Mark/unmark current semitone"), this);
	auto markAllOctaveSemitonesAction = new QAction(tr("Mark/unmark all corresponding octave semitones"), this);
	auto markScaleAction = new QAction(tr("Mark current scale"), this);
	auto markChordAction = new QAction(tr("Mark current chord"), this);
	auto unmarkAllAction = new QAction(tr("Unmark all"), this);
	auto copyAllNotesAction = new QAction(tr("Select all notes on this key"), this);

	connect( markSemitoneAction, &QAction::triggered, [this](){ markSemiTone(SemiToneMarkerAction::MarkCurrentSemiTone); });
	connect( markAllOctaveSemitonesAction, &QAction::triggered, [this](){ markSemiTone(SemiToneMarkerAction::MarkAllOctaveSemiTones); });
	connect( markScaleAction, &QAction::triggered, [this](){ markSemiTone(SemiToneMarkerAction::MarkCurrentScale); });
	connect( markChordAction, &QAction::triggered, [this](){ markSemiTone(SemiToneMarkerAction::MarkCurrentChord); });
	connect( unmarkAllAction, &QAction::triggered, [this](){ markSemiTone(SemiToneMarkerAction::UnmarkAll); });
	connect( copyAllNotesAction, &QAction::triggered, [this](){ markSemiTone(SemiToneMarkerAction::CopyAllNotesOnKey); });
	connect(ConfigManager::inst(), &ConfigManager::valueChanged,
		[this](QString const& cls, QString const& attribute, QString const& value)
		{
			if (!(cls == "midi" && attribute == "autoquantize"))
			{
				return;
			}
			this->m_doAutoQuantization = (value.toInt() != 0);
		});

	markScaleAction->setEnabled( false );
	markChordAction->setEnabled( false );

	connect( this, SIGNAL(semiToneMarkerMenuScaleSetEnabled(bool)), markScaleAction, SLOT(setEnabled(bool)));
	connect( this, SIGNAL(semiToneMarkerMenuChordSetEnabled(bool)), markChordAction, SLOT(setEnabled(bool)));

	m_semiToneMarkerMenu->addAction( markSemitoneAction );
	m_semiToneMarkerMenu->addAction( markAllOctaveSemitonesAction );
	m_semiToneMarkerMenu->addAction( markScaleAction );
	m_semiToneMarkerMenu->addAction( markChordAction );
	m_semiToneMarkerMenu->addAction( unmarkAllAction );
	m_semiToneMarkerMenu->addAction( copyAllNotesAction );

	// init text-float
	if( s_textFloat == nullptr )
	{
		s_textFloat = new SimpleTextFloat;
	}

	// add time-line
	m_timeLine = new TimeLineWidget(m_whiteKeyWidth, 0, m_ppb,
		Engine::getSong()->getPlayPos(Song::PlayMode::MidiClip),
		Engine::getSong()->getTimeline(Song::PlayMode::MidiClip),
		m_currentPosition, Song::PlayMode::MidiClip, this
	);
	connect(this, &PianoRoll::positionChanged, m_timeLine, &TimeLineWidget::updatePosition);
	connect( m_timeLine, SIGNAL( positionChanged( const lmms::TimePos& ) ),
			this, SLOT( updatePosition( const lmms::TimePos& ) ) );

	// white position line follows timeline marker
	m_positionLine = new PositionLine(this, Song::PlayMode::MidiClip);

	//update timeline when in step-recording mode
	connect( &m_stepRecorderWidget, SIGNAL( positionChanged( const lmms::TimePos& ) ),
			this, SLOT( updatePositionStepRecording( const lmms::TimePos& ) ) );

	// update timeline when in record-accompany mode
	connect(m_timeLine, &TimeLineWidget::positionChanged, this, &PianoRoll::updatePositionAccompany);
	// TODO
/*	connect( engine::getSong()->getPlayPos( Song::PlayMode::Pattern ).m_timeLine,
				SIGNAL( positionChanged( const lmms::TimePos& ) ),
			this,
			SLOT( updatePositionAccompany( const lmms::TimePos& ) ) );*/

	removeSelection();

	// init scrollbars
	m_leftRightScroll = new QScrollBar( Qt::Horizontal, this );
	m_leftRightScroll->setSingleStep( 1 );
	connect( m_leftRightScroll, SIGNAL(valueChanged(int)), this,
						SLOT(horScrolled(int)));

	m_topBottomScroll = new QScrollBar( Qt::Vertical, this );
	m_topBottomScroll->setSingleStep( 1 );
	m_topBottomScroll->setPageStep( 20 );
	connect( m_topBottomScroll, SIGNAL(valueChanged(int)), this,
						SLOT(verScrolled(int)));

	// setup zooming-stuff
	for( float const & zoomLevel : m_zoomLevels )
	{
		m_zoomingModel.addItem(QString("%1%").arg(zoomLevel * 100));
	}
	m_zoomingModel.setValue( m_zoomingModel.findText( "100%" ) );
	connect( &m_zoomingModel, SIGNAL(dataChanged()),
					this, SLOT(zoomingChanged()));

	// zoom y
	for (float const & zoomLevel : m_zoomYLevels)
	{
		m_zoomingYModel.addItem(QString("%1%").arg(zoomLevel * 100));
	}
	m_zoomingYModel.setValue(m_zoomingYModel.findText("100%"));
	connect(&m_zoomingYModel, SIGNAL(dataChanged()),
					this, SLOT(zoomingYChanged()));

	// Set up quantization model
	m_quantizeModel.addItem( tr( "Note lock" ) );
	for (auto q : Quantizations) {
		m_quantizeModel.addItem(QString("1/%1").arg(q));
	}
	m_quantizeModel.setValue( m_quantizeModel.findText( "1/16" ) );

	connect( &m_quantizeModel, SIGNAL(dataChanged()),
					this, SLOT(quantizeChanged()));

	// Set up note length model
	m_noteLenModel.addItem( tr( "Last note" ),
					std::make_unique<PixmapLoader>( "edit_draw" ) );
	const auto pixmaps = std::array<std::string, 11>{"whole", "half", "quarter", "eighth",
						"sixteenth", "thirtysecond", "triplethalf",
						"tripletquarter", "tripleteighth",
						"tripletsixteenth", "tripletthirtysecond"};

	for( int i = 0; i < NUM_EVEN_LENGTHS; ++i )
	{
		auto loader = std::make_unique<PixmapLoader>( "note_" + pixmaps[i] );
		m_noteLenModel.addItem( "1/" + QString::number( 1 << i ), std::move(loader) );
	}
	for( int i = 0; i < NUM_TRIPLET_LENGTHS; ++i )
	{
		auto loader = std::make_unique<PixmapLoader>( "note_" + pixmaps[i+NUM_EVEN_LENGTHS] );
		m_noteLenModel.addItem( "1/" + QString::number( (1 << i) * 3 ), std::move(loader) );
	}
	m_noteLenModel.setValue( 0 );

	// Note length change can cause a redraw if Q is set to lock
	connect( &m_noteLenModel, SIGNAL(dataChanged()),
					this, SLOT(noteLengthChanged()));

	// Set up key selection dropdown
	m_keyModel.addItem(tr("No key"));
	// Use piano roll note strings for key dropdown
	for (const auto& noteString : s_noteStrings)
	{
		m_keyModel.addItem(noteString);
	}
	m_keyModel.setValue(0); // start with "No key"
	connect(&m_keyModel, &ComboBoxModel::dataChanged, this, &PianoRoll::keyChanged);

	// Set up scale model
	const InstrumentFunctionNoteStacking::ChordTable& chord_table =
			InstrumentFunctionNoteStacking::ChordTable::getInstance();

	m_scaleModel.addItem( tr("No scale") );
	for (const InstrumentFunctionNoteStacking::Chord& chord : chord_table.chords())
	{
		if( chord.isScale() )
		{
			m_scaleModel.addItem( chord.getName() );
		}
	}

	m_scaleModel.setValue( 0 );
	// connect scale change to key change so it auto-highlights with scale as well
	connect(&m_scaleModel, &ComboBoxModel::dataChanged, this, &PianoRoll::keyChanged);
	// change can update m_semiToneMarkerMenu
	connect( &m_scaleModel, SIGNAL(dataChanged()),
						this, SLOT(updateSemiToneMarkerMenu()));

	// Set up chord model
	m_chordModel.addItem( tr("No chord") );
	for (const InstrumentFunctionNoteStacking::Chord& chord : chord_table.chords())
	{
		if( ! chord.isScale() )
		{
			m_chordModel.addItem( chord.getName() );
		}
	}

	m_chordModel.setValue( 0 );

	// change can update m_semiToneMarkerMenu
	connect( &m_chordModel, SIGNAL(dataChanged()),
					this, SLOT(updateSemiToneMarkerMenu()));

	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	setMouseTracking( true );

	connect( &m_scaleModel, SIGNAL(dataChanged()),
					this, SLOT(updateSemiToneMarkerMenu()));

	connect( Engine::getSong(), SIGNAL(timeSignatureChanged(int,int)),
						this, SLOT(update()));

	//connection for selecion from timeline
	connect( m_timeLine, SIGNAL(regionSelectedFromPixels(int,int)),
			this, SLOT(selectRegionFromPixels(int,int)));

	// Set up snap model
	m_snapModel.addItem(tr("Nudge"));
	m_snapModel.addItem(tr("Snap"));
	m_snapModel.setValue(0);
	changeSnapMode();
	connect(&m_snapModel, SIGNAL(dataChanged()),
		this, SLOT(changeSnapMode()));

	m_stepRecorder.initialize();

	// trigger a redraw if keymap definitions change (different keys may become disabled)
	connect(Engine::getSong(), SIGNAL(keymapListChanged(int)), this, SLOT(update()));
}



void PianoRoll::reset()
{
	m_lastNoteVolume = DefaultVolume;
	m_lastNotePanning = DefaultPanning;
	clearGhostClip();
}

void PianoRoll::showTextFloat(const QString &text, const QPoint &pos, int timeout)
{
	s_textFloat->setText( text );
	// show the float, offset slightly so as to not obscure anything
	s_textFloat->moveGlobal( this, pos + QPoint(4, 16) );
	s_textFloat->showWithTimeout(timeout);
}


void PianoRoll::showVolTextFloat(volume_t vol, const QPoint &pos, int timeout)
{
	//! \todo display velocity for MIDI-based instruments
	// possibly dBFS values too? not sure if it makes sense for note volumes...
	showTextFloat( tr("Velocity: %1%").arg( vol ), pos, timeout );
}


void PianoRoll::showPanTextFloat(panning_t pan, const QPoint &pos, int timeout)
{
	QString text;
	if( pan < 0 )
	{
		text = tr("Panning: %1% left").arg( qAbs( pan ) );
	}
	else if( pan > 0 )
	{
		text = tr("Panning: %1% right").arg( qAbs( pan ) );
	}
	else
	{
		text = tr("Panning: center");
	}
	showTextFloat( text, pos, timeout );
}



void PianoRoll::changeNoteEditMode( int i )
{
	m_noteEditMode = (NoteEditMode) i;
	repaint();
}


void PianoRoll::markSemiTone(SemiToneMarkerAction i, bool fromMenu)
{
	const int key = fromMenu
		? getKey(mapFromGlobal(m_semiToneMarkerMenu->pos()).y())
		: m_keyModel.value() - 1;
	const InstrumentFunctionNoteStacking::Chord * chord = nullptr;

	// if "No key" is selected, key is -1, unmark all semitones
	// or if scale changed from toolbar to "No scale", unmark all semitones
	if (!fromMenu && (key < 0 || m_scaleModel.value() == 0)) { i = SemiToneMarkerAction::UnmarkAll; }

	switch(i)
	{
		case SemiToneMarkerAction::UnmarkAll:
			m_markedSemiTones.clear();
			break;
		case SemiToneMarkerAction::MarkCurrentSemiTone:
		{
			QList<int>::iterator it = std::find( m_markedSemiTones.begin(), m_markedSemiTones.end(), key );
			if( it != m_markedSemiTones.end() )
			{
				m_markedSemiTones.erase( it );
			}
			else
			{
				m_markedSemiTones.push_back( key );
			}
			break;
		}
		case SemiToneMarkerAction::MarkAllOctaveSemiTones:
		{
			QList<int> aok = getAllOctavesForKey(key);

			if ( m_markedSemiTones.contains(key) )
			{
				// lets erase all of the ones that match this by octave
				QList<int>::iterator i;
				for (int octave : aok)
				{
					i = std::find(m_markedSemiTones.begin(), m_markedSemiTones.end(), octave);
					if (i != m_markedSemiTones.end())
					{
						m_markedSemiTones.erase(i);
					}
				}
			}
			else
			{
				// we should add all of the ones that match this by octave
				m_markedSemiTones.append(aok);
			}

			break;
		}
		case SemiToneMarkerAction::MarkCurrentScale:
			chord = & InstrumentFunctionNoteStacking::ChordTable::getInstance()
					.getScaleByName( m_scaleModel.currentText() );
		case SemiToneMarkerAction::MarkCurrentChord:
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
			const int cap = (chord->isScale() || chord->last() == 0) ? trackOctaveSize() : chord->last();

			for( int i = first; i <= last; i++ )
			{
			  if( chord->hasSemiTone( ( i + cap - ( key % cap ) ) % cap ) )
				{
					m_markedSemiTones.push_back( i );
				}
			}
			break;
		}
		case SemiToneMarkerAction::CopyAllNotesOnKey:
		{
			selectNotesOnKey();
			break;
		}
		default:
			;
	}

	std::sort( m_markedSemiTones.begin(), m_markedSemiTones.end(), std::greater<int>() );
	QList<int>::iterator new_end = std::unique( m_markedSemiTones.begin(), m_markedSemiTones.end() );
	m_markedSemiTones.erase( new_end, m_markedSemiTones.end() );
	// until we move the mouse the window won't update, force redraw
	update();
}


void PianoRoll::setGhostMidiClip( MidiClip* newMidiClip )
{
	// Expects a pointer to a MIDI clip or nullptr.
	m_ghostNotes.clear();
	if( newMidiClip != nullptr )
	{
		for( Note *note : newMidiClip->notes() )
		{
			auto new_note = new Note(note->length(), note->pos(), note->key());
			m_ghostNotes.push_back( new_note );
		}
		emit ghostClipSet( true );
	}
}


void PianoRoll::loadGhostNotes( const QDomElement & de )
{
	// Load ghost notes from DOM element.
	if( de.isElement() )
	{
		QDomNode node = de.firstChild();
		while( !node.isNull() )
		{
			auto n = new Note;
			n->restoreState( node.toElement() );
			n->setVolume(DefaultVolume);
			m_ghostNotes.push_back( n );
			node = node.nextSibling();
		}
		emit ghostClipSet( true );
	}
}


void PianoRoll::clearGhostClip()
{
	setGhostMidiClip( nullptr );
	emit ghostClipSet( false );
	update();
}


void PianoRoll::glueNotes()
{
	if (hasValidMidiClip())
	{
		NoteVector selectedNotes = getSelectedNotes();
		if (selectedNotes.empty())
		{
			TextFloat::displayMessage( tr( "Glue notes failed" ),
					tr( "Please select notes to glue first." ),
					embed::getIconPixmap( "glue", 24, 24 ),
					3000 );
			return;
		}

		// Make undo possible
		m_midiClip->addJournalCheckPoint();

		// Sort notes on key and then pos.
		std::sort(selectedNotes.begin(), selectedNotes.end(),
			[](const Note * note, const Note * compareNote) -> bool
			{
				if (note->key() == compareNote->key())
				{
					return note->pos() < compareNote->pos();
				}
				return note->key() < compareNote->key();
			});

		QList<Note *> noteToRemove;

		NoteVector::iterator note = selectedNotes.begin();
		auto nextNote = note+1;
		NoteVector::iterator end = selectedNotes.end();

		while (note != end && nextNote != end)
		{
			// key and position match for glue. The notes are already
			// sorted so we don't need to test that nextNote is the same
			// position or next in sequence.
			if ((*note)->key() == (*nextNote)->key()
				&& (*nextNote)->pos() <= (*note)->pos()
				+ qMax(TimePos(0), (*note)->length()))
			{
				(*note)->setLength(qMax((*note)->length(),
					TimePos((*nextNote)->endPos() - (*note)->pos())));
				noteToRemove.push_back(*nextNote);
				++nextNote;
			}
			// key or position doesn't match
			else
			{
				note = nextNote;
				nextNote = note+1;
			}
		}

		// Remove old notes
		for (int i = 0; i < noteToRemove.count(); ++i)
		{
			m_midiClip->removeNote(noteToRemove[i]);
		}

		update();
	}
}

void PianoRoll::fitNoteLengths(bool fill)
{
	if (!hasValidMidiClip()) { return; }
	m_midiClip->addJournalCheckPoint();
	m_midiClip->rearrangeAllNotes();

	// Reference notes
	const NoteVector& refNotes = m_midiClip->notes();

	// Notes to edit
	NoteVector notes = getSelectedNotes();
	if (notes.empty())
	{
		notes = refNotes;
	}
	else if (!fill)
	{
		std::sort(notes.begin(), notes.end(), Note::lessThan);
	}
	if (fill)
	{
		std::sort(notes.begin(), notes.end(), [](Note* n1, Note* n2) { return n1->endPos() < n2->endPos(); });
	}

	int length;
	auto ref = refNotes.begin();
	for (Note* note : notes)
	{
		// Fast forward to next reference note
		while (ref != refNotes.end() && (fill ? (*ref)->pos() < note->endPos() : (*ref)->pos() <= note->pos()))
		{
			ref++;
		}
		if (ref == refNotes.end())
		{
			if (!fill) { break; }
			// Last notes stretch to end of last bar
			length = notes.back()->endPos().nextFullBar() * TimePos::ticksPerBar() - note->pos();
		}
		else
		{
			length = (*ref)->pos() - note->pos();
		}
		if (fill ? note->length() < length : note->length() > length)
		{
			note->setLength(length);
		}
	}

	update();
	getGUI()->songEditor()->update();
	Engine::getSong()->setModified();
}


void PianoRoll::constrainNoteLengths(bool constrainMax)
{
	if (!hasValidMidiClip()) { return; }
	m_midiClip->addJournalCheckPoint();

	const NoteVector selectedNotes = getSelectedNotes();
	const auto& notes = selectedNotes.empty() ? m_midiClip->notes() : selectedNotes;

	TimePos bound = m_lenOfNewNotes; // will be length of last note
	for (auto note : notes)
	{
		if (constrainMax ? note->length() > bound : note->length() < bound)
		{
			note->setLength(bound);
		}
	}

	update();
	getGUI()->songEditor()->update();
	Engine::getSong()->setModified();
}

void PianoRoll::reverseNotes()
{
	if (!hasValidMidiClip()) { return; }

	const NoteVector selectedNotes = getSelectedNotes();
	const auto& notes = selectedNotes.empty() ? m_midiClip->notes() : selectedNotes;

	m_midiClip->reverseNotes(notes);

	update();
	getGUI()->songEditor()->update();
	Engine::getSong()->setModified();
}


void PianoRoll::loadMarkedSemiTones(const QDomElement & de)
{
	// clear marked semitones to prevent leftover marks
	m_markedSemiTones.clear();
	if (de.isElement())
	{
		QDomNode node = de.firstChild();
		while (!node.isNull())
		{
			bool ok;
			int key = node.toElement().attribute(
				QString("key"), QString("-1")).toInt(&ok, 10);
			if (ok && key >= 0)
			{
				m_markedSemiTones.append(key);
			}
			node = node.nextSibling();
		}
	}
	// from markSemiTone, required otherwise marks will not show
	std::sort(m_markedSemiTones.begin(), m_markedSemiTones.end(), std::greater<int>());
	QList<int>::iterator new_end = std::unique(m_markedSemiTones.begin(), m_markedSemiTones.end());
	m_markedSemiTones.erase(new_end, m_markedSemiTones.end());
}


void PianoRoll::setCurrentMidiClip( MidiClip* newMidiClip )
{
	if( hasValidMidiClip() )
	{
		m_midiClip->instrumentTrack()->pianoModel()->disconnect(this);
		m_midiClip->instrumentTrack()->firstKeyModel()->disconnect(this);
		m_midiClip->instrumentTrack()->lastKeyModel()->disconnect(this);
		m_midiClip->instrumentTrack()->microtuner()->keymapModel()->disconnect(this);
		m_midiClip->instrumentTrack()->microtuner()->keyRangeImportModel()->disconnect(this);
		m_midiClip->instrumentTrack()->disconnect( this );
		m_midiClip->disconnect(this);
	}

	// force the song-editor to stop playing if it played a MIDI clip before
	if (Engine::getSong()->playMode() == Song::PlayMode::MidiClip)
	{
		Engine::getSong()->playMidiClip( nullptr );
	}

	if(m_stepRecorder.isRecording())
	{
		m_stepRecorder.stop();
	}

	// set new data
	m_midiClip = newMidiClip;
	m_currentPosition = 0;
	m_currentNote = nullptr;
	m_startKey = INITIAL_START_KEY;

	m_stepRecorder.setCurrentMidiClip(newMidiClip);

	if( ! hasValidMidiClip() )
	{
		//resizeEvent( NULL );

		update();
		emit currentMidiClipChanged();
		return;
	}

	// Scroll horizontally to the start of the clip, minus a bar for aesthetics.
	m_leftRightScroll->setValue(std::max(0, -m_midiClip->startTimeOffset() - TimePos::ticksPerBar()));

	// determine the central key so that we can scroll to it
	int central_key = 0;
	int total_notes = 0;
	for( const Note *note : m_midiClip->notes() )
	{
		if( note->length() > 0 )
		{
			central_key += note->key();
			++total_notes;
		}
	}

	if (total_notes > 0)
	{
		central_key = central_key / total_notes - (NumKeys - m_totalKeysToScroll) / 2;
		m_startKey = qBound(0, central_key, NumKeys);
	}

	// Make sure the playhead position isn't out of the clip bounds.
	Engine::getSong()->getPlayPos(Song::PlayMode::MidiClip).setTicks(std::clamp(
		Engine::getSong()->getPlayPos(Song::PlayMode::MidiClip).getTicks(),
		std::max(0, -m_midiClip->startTimeOffset()),
		m_midiClip->length() - m_midiClip->startTimeOffset()
	));

	// resizeEvent() does the rest for us (scrolling, range-checking
	// of start-notes and so on...)
	resizeEvent( nullptr );

	// make sure to always get informed about the MIDI clip being destroyed
	connect( m_midiClip, SIGNAL(destroyedMidiClip(lmms::MidiClip*)), this, SLOT(hideMidiClip(lmms::MidiClip*)));

	connect( m_midiClip->instrumentTrack(), SIGNAL( midiNoteOn( const lmms::Note& ) ), this, SLOT( startRecordNote( const lmms::Note& ) ) );
	connect( m_midiClip->instrumentTrack(), SIGNAL( midiNoteOff( const lmms::Note& ) ), this, SLOT( finishRecordNote( const lmms::Note& ) ) );
	connect( m_midiClip, SIGNAL(dataChanged()), this, SLOT(update()));
	connect( m_midiClip->instrumentTrack()->pianoModel(), SIGNAL(dataChanged()), this, SLOT(update()));

	connect(m_midiClip->instrumentTrack()->firstKeyModel(), SIGNAL(dataChanged()), this, SLOT(update()));
	connect(m_midiClip->instrumentTrack()->lastKeyModel(), SIGNAL(dataChanged()), this, SLOT(update()));
	connect(m_midiClip->instrumentTrack()->microtuner()->keymapModel(), SIGNAL(dataChanged()), this, SLOT(update()));
	connect(m_midiClip->instrumentTrack()->microtuner()->keyRangeImportModel(), SIGNAL(dataChanged()),
		this, SLOT(update()));
	connect(m_midiClip, &MidiClip::lengthChanged, this, qOverload<>(&QWidget::update));

	update();
	emit currentMidiClipChanged();
}



void PianoRoll::hideMidiClip( MidiClip* clip )
{
	if( m_midiClip == clip )
	{
		setCurrentMidiClip( nullptr );
	}
}


int PianoRoll::trackOctaveSize() const
{
	if (!m_midiClip) { return KeysPerOctave; }
	auto ut = m_midiClip->instrumentTrack()->microtuner();
	return ut->enabled() ? ut->octaveSize() : KeysPerOctave;
}


void PianoRoll::selectRegionFromPixels( int xStart, int xEnd )
{

	xStart -= m_whiteKeyWidth;
	xEnd -= m_whiteKeyWidth;

	// select an area of notes
	int posTicks = xStart * TimePos::ticksPerBar() / m_ppb +
					m_currentPosition;
	int keyNum = 0;
	m_selectStartTick = posTicks;
	m_selectedTick = 0;
	m_selectStartKey = keyNum;
	m_selectedKeys = 1;
	// change size of selection

	// get tick in which the cursor is posated
	posTicks = xEnd  * TimePos::ticksPerBar() / m_ppb +
					m_currentPosition;
	keyNum = 120;

	m_selectedTick = posTicks - m_selectStartTick;
	if( (int) m_selectStartTick + m_selectedTick < 0 )
	{
		m_selectedTick = -static_cast<int>(
					m_selectStartTick );
	}
	m_selectedKeys = keyNum - m_selectStartKey;
	if( keyNum <= m_selectStartKey )
	{
		--m_selectedKeys;
	}

	computeSelectedNotes( false );
}




void PianoRoll::drawNoteRect( QPainter & p, int x, int y,
				int width, const Note * n, const QColor & noteCol, const QColor & noteTextColor,
				const QColor & selCol, const int noteOpc, const bool borders, bool drawNoteName )
{
	++x;
	++y;
	width -= 2;

	if( width <= 0 )
	{
		width = 2;
	}

	// Volume
	auto const volumeRange = static_cast<float>(MaxVolume - MinVolume);
	auto const volumeSpan = static_cast<float>(n->getVolume() - MinVolume);
	float const volumeRatio = volumeSpan / volumeRange;
	int volVal = qMin( 255, 100 + static_cast<int>( volumeRatio * 155.0f) );

	// Panning
	auto const panningRange = static_cast<float>(PanningRight - PanningLeft);
	auto const leftPanSpan = static_cast<float>(PanningRight - n->getPanning());
	auto const rightPanSpan = static_cast<float>(n->getPanning() - PanningLeft);

	float leftPercent = qMin<float>( 1.0f, leftPanSpan / panningRange * 2.0f );
	float rightPercent = qMin<float>( 1.0f, rightPanSpan / panningRange * 2.0f );

	QColor col{noteCol};
	QPen pen;

	if( n->selected() )
	{
		col = QColor( selCol );
	}

	const int borderWidth = borders ? 1 : 0;

	const int noteHeight = m_keyLineHeight - 1 - borderWidth;
	int noteWidth = width + 1 - borderWidth;

	// adjust note to make it a bit faded if it has a lower volume
	// in stereo using gradients
	QColor lcol = QColor::fromHsv( col.hue(), col.saturation(),
				       static_cast<int>(volVal * leftPercent), noteOpc );
	QColor rcol = QColor::fromHsv( col.hue(), col.saturation(),
				       static_cast<int>(volVal * rightPercent), noteOpc );

	QLinearGradient gradient( x, y, x, y + noteHeight );
	gradient.setColorAt( 0, rcol );
	gradient.setColorAt( 1, lcol );
	p.setBrush( gradient );

	if ( borders )
	{
		p.setPen( col );
	}
	else
	{
		p.setPen( Qt::NoPen );
	}

	p.drawRect( x, y, noteWidth, noteHeight );

	// Draw note key text
	if (drawNoteName)
	{
		p.save();
		int const noteTextHeight = static_cast<int>(noteHeight * 0.8);
		if (noteTextHeight > 6)
		{
			QString noteKeyString = getNoteString(n->key());

			QFont noteFont(p.font());
			noteFont = adjustedToPixelSize(noteFont, noteTextHeight);
			QFontMetrics fontMetrics(noteFont);
			QSize textSize = fontMetrics.size(Qt::TextSingleLine, noteKeyString);

			int const distanceToBorder = 2;
			int const xOffset = borderWidth + distanceToBorder;

			// noteTextHeight, textSize are not suitable for determining vertical spacing,
			// capHeight() can be used for this, but requires Qt 5.8.
			// We use boundingRect() with QChar (the QString version returns wrong value).
			QRect const boundingRect = fontMetrics.boundingRect(QChar::fromLatin1('H'));
			int const yOffset = (noteHeight - boundingRect.top() - boundingRect.bottom()) / 2;

			if (textSize.width() < noteWidth - xOffset)
			{
				p.setPen(noteTextColor);
				p.setFont(noteFont);
				QPoint textStart(x + xOffset, y + yOffset);

				p.drawText(textStart, noteKeyString);
			}
		}
		p.restore();
	}

	// draw the note endmark, to hint the user to resize
	p.setBrush( col );
	if( width > 2 )
	{
		const int endmarkWidth = 3 - borderWidth;
		p.drawRect( x + noteWidth - endmarkWidth, y, endmarkWidth, noteHeight );
	}
}




void PianoRoll::drawDetuningInfo( QPainter & _p, const Note * _n, int _x,
								int _y ) const
{
	int middle_y = _y + m_keyLineHeight / 2;
	_p.setPen(m_noteColor);
	_p.setClipRect(
		m_whiteKeyWidth,
		PR_TOP_MARGIN,
		width() - m_whiteKeyWidth,
		keyAreaBottom() - PR_TOP_MARGIN);

	// Draw lines for the detuning automation, treating cubic hermit curves
	// as straight lines for now. Also draw discrete jumps.
	int old_x = 0;
	int old_y = 0;

	timeMap & map = _n->detuning()->automationClip()->getTimeMap();
	for (timeMap::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		// Current node values
		int cur_ticks = POS(it);
		int cur_x = _x + cur_ticks * m_ppb / TimePos::ticksPerBar();
		const float cur_level = INVAL(it);
		int cur_y = middle_y - cur_level * m_keyLineHeight;

		// First line to represent the inValue of the first node
		if (it == map.begin())
		{
			_p.drawLine(cur_x - 1, cur_y, cur_x + 1, cur_y);
			_p.drawLine(cur_x, cur_y - 1, cur_x, cur_y + 1);
		}
		// All subsequent lines will take the outValue of the previous node
		// and the inValue of the current node. It will also draw a vertical
		// line if there was a discrete jump (from old_x,old_y to pre_x,pre_y)
		else
		{
			// Previous node values (based on outValue). We just calculate
			// the y level because the x will be the same as old_x.
			const float pre_level = OUTVAL(it - 1);
			int pre_y = middle_y - pre_level * m_keyLineHeight;

			// Draws the line representing the discrete jump if there's one
			if (old_y != pre_y)
			{
				_p.drawLine(old_x, old_y, old_x, pre_y);
			}

			// Now draw the lines representing the actual progression from one
			// node to the other
			switch (_n->detuning()->automationClip()->progressionType())
			{
				case AutomationClip::ProgressionType::Discrete:
					_p.drawLine(old_x, pre_y, cur_x, pre_y);
					_p.drawLine(cur_x, pre_y, cur_x, cur_y);
					break;
				case AutomationClip::ProgressionType::CubicHermite: /* TODO */
				case AutomationClip::ProgressionType::Linear:
					_p.drawLine(old_x, pre_y, cur_x, cur_y);
					break;
			}

			// If we are in the last node and there's a discrete jump, we draw a
			// vertical line representing it
			if ((it + 1) == map.end())
			{
				const float last_level = OUTVAL(it);
				if (cur_level != last_level)
				{
					int last_y = middle_y - last_level * m_keyLineHeight;
					_p.drawLine(cur_x, cur_y, cur_x, last_y);
				}
			}
		}

		old_x = cur_x;
		old_y = cur_y;
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
	if( m_midiClip != nullptr )
	{
		for( Note *note : m_midiClip->notes() )
		{
			note->setSelected( false );
		}
	}
}



void PianoRoll::shiftSemiTone(int amount) //Shift notes by amount semitones
{
	if (!hasValidMidiClip()) { return; }

	auto selectedNotes = getSelectedNotes();
	//If no notes are selected, shift all of them, otherwise shift selection
	if (selectedNotes.empty()) { shiftSemiTone(m_midiClip->notes(), amount); }
	else { shiftSemiTone(selectedNotes, amount); }
}

void PianoRoll::shiftSemiTone(const NoteVector& notes, int amount)
{
	m_midiClip->addJournalCheckPoint();
	for (Note *note : notes) { note->setKey( note->key() + amount ); }

	m_midiClip->rearrangeAllNotes();
	m_midiClip->dataChanged();
	//We modified the song
	update();
	getGUI()->songEditor()->update();
}




void PianoRoll::shiftPos(int amount) //Shift notes pos by amount
{
	if (!hasValidMidiClip()) { return; }

	auto selectedNotes = getSelectedNotes();
	//If no notes are selected, shift all of them, otherwise shift selection
	if (selectedNotes.empty()) { shiftPos(m_midiClip->notes(), amount); }
	else { shiftPos(selectedNotes, amount); }
}

void PianoRoll::shiftPos(const NoteVector& notes, int amount)
{
	m_midiClip->addJournalCheckPoint();

	if (notes.empty()) {
		return;
	}

	auto leftMostPos = notes.front()->pos();
	//Limit leftwards shifts to prevent moving left of clip start
	auto shiftAmount = (leftMostPos > -amount) ? amount : -leftMostPos;
	if (shiftAmount == 0) { return; }

	for (Note *note : notes) { note->setPos( note->pos() + shiftAmount ); }

	m_midiClip->rearrangeAllNotes();
	m_midiClip->updateLength();
	m_midiClip->dataChanged();
	// we modified the song
	update();
	getGUI()->songEditor()->update();
}




bool PianoRoll::isSelection() const // are any notes selected?
{
	for( const Note *note : m_midiClip->notes() )
	{
		if( note->selected() )
		{
			return true;
		}
	}

	return false;
}



int PianoRoll::selectionCount() const // how many notes are selected?
{
	return getSelectedNotes().size();
}



void PianoRoll::keyPressEvent(QKeyEvent* ke)
{
	if(m_stepRecorder.isRecording())
	{
		bool handled = m_stepRecorder.keyPressEvent(ke);
		if(handled)
		{
			ke->accept();
			update();
			return;
		}
	}

	if( hasValidMidiClip() && ke->modifiers() == Qt::NoModifier )
	{
		const int key_num = PianoView::getKeyFromKeyEvent( ke );

		if (!ke->isAutoRepeat() && key_num > -1)
		{
			m_midiClip->instrumentTrack()->pianoModel()->handleKeyPress(key_num);
			//  if a chord is set, play all chord notes (simulate click on all):
			playChordNotes(key_num);
			ke->accept();
			return;
		}
	}

	switch( ke->key() )
	{
		case Qt::Key_Up:
		case Qt::Key_Down:
			{
				int direction = (ke->key() == Qt::Key_Up ? +1 : -1);
				if( ( ke->modifiers() & Qt::ControlModifier ) && m_action == Action::None )
				{
					// shift selection by one octave
					// if nothing selected, shift _everything_
					if (hasValidMidiClip())
					{
						// An octave could potentially be greater or less than twelve semitones if the microtuner is in use.
						const auto microtuner = m_midiClip->instrumentTrack()->microtuner();
						if (microtuner->enabled())
						{
							shiftSemiTone(microtuner->octaveSize() * direction);
						}
						else
						{
							shiftSemiTone(12 * direction);
						}
					}
				}
				else if((ke->modifiers() & Qt::ShiftModifier) && m_action == Action::None)
				{
					// Move selected notes by one semitone
					if (hasValidMidiClip())
					{
						shiftSemiTone( 1 * direction );
					}
				}
				else
				{
					// scroll
					m_topBottomScroll->setValue( m_topBottomScroll->value() -
						cm_scrollAmtVert * direction );

					// if they are moving notes around or resizing,
					// recalculate the note/resize position
					if( m_action == Action::MoveNote ||
							m_action == Action::ResizeNote )
					{
						dragNotes(
							m_lastMouseX,
							m_lastMouseY,
							ke->modifiers() & Qt::AltModifier,
							ke->modifiers() & Qt::ShiftModifier,
							ke->modifiers() & Qt::ControlModifier
						);
					}
				}
				ke->accept();
				break;
			}

		case Qt::Key_Right:
		case Qt::Key_Left:
			{
				int direction = (ke->key() == Qt::Key_Right ? +1 : -1);
				if( ke->modifiers() & Qt::ControlModifier && m_action == Action::None )
				{
					// Move selected notes by one bar to the left
					if (hasValidMidiClip())
					{
						shiftPos( direction * TimePos::ticksPerBar() );
					}
				}
				else if( ke->modifiers() & Qt::ShiftModifier && m_action == Action::None)
				{
					// move notes
					if (hasValidMidiClip())
					{
						bool quantized = ! ( ke->modifiers() & Qt::AltModifier );
						int amt = quantized ? quantization() : 1;
						shiftPos( direction * amt );
					}
				}
				else if( ke->modifiers() & Qt::AltModifier)
				{
					// switch to editing a clip adjacent to this one in the song editor
					if (hasValidMidiClip())
					{
						MidiClip * c = direction > 0 ? m_midiClip->nextMidiClip()
										: m_midiClip->previousMidiClip();
						if(c != nullptr)
						{
							setCurrentMidiClip(c);
						}
					}
				}
				else
				{
					// scroll
					m_leftRightScroll->setValue( m_leftRightScroll->value() +
						direction * cm_scrollAmtHoriz );

					// if they are moving notes around or resizing,
					// recalculate the note/resize position
					if( m_action == Action::MoveNote ||
							m_action == Action::ResizeNote )
					{
						dragNotes(
							m_lastMouseX,
							m_lastMouseY,
							ke->modifiers() & Qt::AltModifier,
							ke->modifiers() & Qt::ShiftModifier,
							ke->modifiers() & Qt::ControlModifier
						);
					}

				}
				ke->accept();
				break;
			}

		case Qt::Key_A:
			if (ke->modifiers() & Qt::ControlModifier && m_editMode != EditMode::Strum && m_editMode != EditMode::Knife)
			{
				ke->accept();
				if (ke->modifiers() & Qt::ShiftModifier)
				{
					// Ctrl + Shift + A = deselect all notes
					clearSelectedNotes();
				}
				else
				{
					// Ctrl + A = select all notes
					selectAll();
				}
				update();
			}
			break;

		case Qt::Key_Escape:
			// On the Knife mode or Strum mode, ESC cancels it
			if (m_editMode == EditMode::Knife)
			{
				cancelKnifeAction();
			}
			else if (m_editMode == EditMode::Strum)
			{
				cancelStrumAction();
			}
			else
			{
				// Same as Ctrl + Shift + A
				clearSelectedNotes();
			}
			break;

		case Qt::Key_Backspace:
		case Qt::Key_Delete:
			deleteSelectedNotes();
			ke->accept();
			break;

		case Qt::Key_Home:
			m_timeLine->pos().setTicks( 0 );
			m_timeLine->updatePosition();
			ke->accept();
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
			int len = 1 + ke->key() - Qt::Key_0;
			if( len == 10 )
			{
				len = 0;
			}
			if( ke->modifiers() & ( Qt::ControlModifier | Qt::KeypadModifier ) )
			{
				m_noteLenModel.setValue( len );
				ke->accept();
			}
			else if( ke->modifiers() & Qt::AltModifier )
			{
				m_quantizeModel.setValue( len );
				ke->accept();
			}
			break;
		}

		case Qt::Key_Control:
			// Ctrl will not enter selection mode if we are
			// in Knife mode, but unquantize it
			if (m_editMode == EditMode::Knife)
			{
				break;
			}
			// Enter selection mode if:
			// -> this window is active
			// -> shift is not pressed
			// (<S-C-drag> is shortcut for sticky note resize)
			if ( !( ke->modifiers() & Qt::ShiftModifier ) && isActiveWindow() )
			{
				m_ctrlMode = m_editMode;
				m_editMode = EditMode::Select;
				setCursor( Qt::ArrowCursor );
				ke->accept();
			}
			break;
		default:
			ke->ignore();
			break;
	}

	update();
}




void PianoRoll::keyReleaseEvent(QKeyEvent* ke )
{
	if( hasValidMidiClip() && ke->modifiers() == Qt::NoModifier )
	{
		const int key_num = PianoView::getKeyFromKeyEvent( ke );
		if (!ke->isAutoRepeat() && key_num > -1)
		{
			m_midiClip->instrumentTrack()->pianoModel()->handleKeyRelease(key_num);
			// if a chord is set, simulate click release on all chord notes
			pauseChordNotes(key_num);
			ke->accept();
			return;
		}
	}

	switch( ke->key() )
	{
		case Qt::Key_Control:
			if (m_editMode == EditMode::Knife)
			{
				break;
			}
			computeSelectedNotes( ke->modifiers() & Qt::ShiftModifier);
			m_editMode = m_ctrlMode;
			if (m_editMode == EditMode::Strum) { setupSelectedChords(); }
			update();
			break;

		// update after undo/redo
		case Qt::Key_Z:
		case Qt::Key_R:
			if( hasValidMidiClip() && ke->modifiers() == Qt::ControlModifier )
			{
				update();
			}
			break;
		default:
			ke->ignore();
			break;
	}

	update();
}




void PianoRoll::leaveEvent(QEvent * e )
{
	QWidget::leaveEvent( e );
	s_textFloat->hide();
	update(); // cleaning inner mouse-related graphics
}




int PianoRoll::noteEditTop() const
{
	return keyAreaBottom() + NOTE_EDIT_RESIZE_BAR;
}




int PianoRoll::noteEditBottom() const
{
	return height() - PR_BOTTOM_MARGIN;
}




int PianoRoll::noteEditRight() const
{
	return width() - PR_RIGHT_MARGIN;
}




int PianoRoll::noteEditLeft() const
{
	return m_whiteKeyWidth;
}




int PianoRoll::keyAreaTop() const
{
	return PR_TOP_MARGIN;
}




int PianoRoll::keyAreaBottom() const
{
	return height() - PR_BOTTOM_MARGIN - m_notesEditHeight;
}




void PianoRoll::mousePressEvent(QMouseEvent * me )
{
	m_startedWithShift = me->modifiers() & Qt::ShiftModifier;

	if( ! hasValidMidiClip() )
	{
		return;
	}

	// -- Knife
	if (m_editMode == EditMode::Knife && me->button() == Qt::LeftButton)
	{
		updateKnifePos(me, true);
		m_knifeDown = true;
		update();
		return;
	}

	if (m_editMode == EditMode::Strum && me->button() == Qt::LeftButton)
	{
		// Only strum if the user is dragging a selected note
		const auto& selectedNotes = getSelectedNotes();
		if (std::find(selectedNotes.begin(), selectedNotes.end(), noteUnderMouse()) != selectedNotes.end())
		{
			updateStrumPos(me, true, me->modifiers() & Qt::ShiftModifier);
			m_strumEnabled = true;
			update();
		}
		return;
	}

	if( m_editMode == EditMode::Detuning && noteUnderMouse() )
	{
		static QPointer<AutomationClip> detuningClip = nullptr;
		if (detuningClip.data() != nullptr)
		{
			detuningClip->disconnect(this);
		}
		Note* n = noteUnderMouse();
		if (n->detuning() == nullptr)
		{
			n->createDetuning();
		}
		detuningClip = n->detuning()->automationClip();
		connect(detuningClip.data(), SIGNAL(dataChanged()), this, SLOT(update()));
		getGUI()->automationEditor()->setGhostMidiClip(m_midiClip);
		getGUI()->automationEditor()->open(detuningClip);
		return;
	}

	// if holding control, go to selection mode unless shift is also pressed
	if( me->modifiers() & Qt::ControlModifier && m_editMode != EditMode::Select )
	{
		m_ctrlMode = m_editMode;
		m_editMode = EditMode::Select;
		setCursor( Qt::ArrowCursor );
		update();
	}

	// keep track of the point where the user clicked down
	if( me->button() == Qt::LeftButton )
	{
		m_moveStartX = me->x();
		m_moveStartY = me->y();
	}

	if(me->button() == Qt::LeftButton &&
		me->y() > keyAreaBottom() && me->y() < noteEditTop())
	{
		// resizing the note edit area
		m_action = Action::ResizeNoteEditArea;
		return;
	}

	if( me->y() > PR_TOP_MARGIN )
	{
		bool edit_note = ( me->y() > noteEditTop() );

		int key_num = getKey( me->y() );

		int x = me->x();


		if (x > m_whiteKeyWidth)
		{
			// set, move or resize note

			x -= m_whiteKeyWidth;

			// get tick in which the user clicked
			int pos_ticks = x * TimePos::ticksPerBar() / m_ppb +
							m_currentPosition;


			// get note-vector of current MIDI clip
			const NoteVector & notes = m_midiClip->notes();

			// will be our iterator in the following loop
			auto it = notes.rbegin();

			// loop through whole note-vector...
			while (it != notes.rend())
			{
				Note *note = *it;
				TimePos len = note->length();
				if( len < 0 )
				{
					len = 4;
				}
				// and check whether the user clicked on an
				// existing note or an edit-line
				if( pos_ticks >= note->pos() &&
						len > 0 &&
					(
					( ! edit_note &&
					pos_ticks <= note->pos() + len &&
					note->key() == key_num )
					||
					( edit_note &&
					pos_ticks <= note->pos() +
						NOTE_EDIT_LINE_WIDTH * TimePos::ticksPerBar() / m_ppb )
					)
					)
				{
					break;
				}
				++it;
			}

			// first check whether the user clicked in note-edit-
			// area
			if( edit_note )
			{
				m_midiClip->addJournalCheckPoint();
				// scribble note edit changes
				mouseMoveEvent( me );
				return;
			}
			// left button??
			else if( me->button() == Qt::LeftButton &&
							m_editMode == EditMode::Draw )
			{
				// whether this action creates new note(s) or not
				bool is_new_note = false;

				Note * created_new_note = nullptr;
				// did it reach end of vector because
				// there's no note??
				if (it == notes.rend())
				{
					is_new_note = true;
					m_midiClip->addJournalCheckPoint();

					// then set new note

					// clear selection and select this new note
					clearSelectedNotes();

					// +32 to quanitize the note correctly when placing notes with
					// the mouse.  We do this here instead of in note.quantized
					// because live notes should still be quantized at the half.
					TimePos note_pos( pos_ticks - ( quantization() / 2 ) );
					TimePos note_len( newNoteLen() );

					Note new_note( note_len, note_pos, key_num );
					new_note.setSelected( true );
					new_note.setPanning( m_lastNotePanning );
					new_note.setVolume( m_lastNoteVolume );
					created_new_note = m_midiClip->addNote( new_note );

					const InstrumentFunctionNoteStacking::Chord & chord = InstrumentFunctionNoteStacking::ChordTable::getInstance()
						.getChordByName( m_chordModel.currentText() );

					if( ! chord.isEmpty() )
					{
						// if a chord is selected, create following notes in chord
						// or arpeggio mode
						const bool arpeggio = me->modifiers() & Qt::ShiftModifier;
						for( int i = 1; i < chord.size(); i++ )
						{
							if( arpeggio )
							{
								note_pos += note_len;
							}
							Note new_note( note_len, note_pos, key_num + chord[i] );
							new_note.setSelected( true );
							new_note.setPanning( m_lastNotePanning );
							new_note.setVolume( m_lastNoteVolume );
							m_midiClip->addNote( new_note );
						}
					}

					// reset it so that it can be used for
					// ops (move, resize) after this
					// code-block
					it = notes.rbegin();
					while (it != notes.rend() && *it != created_new_note)
					{
						++it;
					}
				}

				Note *current_note = *it;
				m_currentNote = current_note;
				m_lastNotePanning = current_note->getPanning();
				m_lastNoteVolume = current_note->getVolume();
				m_lenOfNewNotes = current_note->length();

				// remember which key and tick we started with
				m_mouseDownKey = m_startKey;
				m_mouseDownTick = m_currentPosition;

				//If clicked on an unselected note, remove selection and select that new note
				if (!m_currentNote->selected())
				{
					clearSelectedNotes();
					m_currentNote->setSelected( true );
				}

				auto selectedNotes = getSelectedNotes();

				m_moveBoundaryLeft = selectedNotes.front()->pos().getTicks();
				m_moveBoundaryRight = selectedNotes.front()->endPos();
				m_moveBoundaryBottom = selectedNotes.front()->key();
				m_moveBoundaryTop = m_moveBoundaryBottom;

				//Figure out the bounding box of all the selected notes
				for (Note *note: selectedNotes)
				{
					// remember note starting positions
					note->setOldKey( note->key() );
					note->setOldPos( note->pos() );
					note->setOldLength( note->length() );

					m_moveBoundaryLeft = qMin(note->pos().getTicks(), (tick_t) m_moveBoundaryLeft);
					m_moveBoundaryRight = qMax((int) note->endPos(), m_moveBoundaryRight);
					m_moveBoundaryBottom = qMin(note->key(), m_moveBoundaryBottom);
					m_moveBoundaryTop = qMax(note->key(), m_moveBoundaryTop);
				}

				// clicked at the "tail" of the note?
				if( pos_ticks * m_ppb / TimePos::ticksPerBar() >
						m_currentNote->endPos() * m_ppb / TimePos::ticksPerBar() - RESIZE_AREA_WIDTH
					&& m_currentNote->length() > 0 )
				{
					m_midiClip->addJournalCheckPoint();
					// then resize the note
					m_action = Action::ResizeNote;

					//Calculate the minimum length we should allow when resizing
					//each note, and let all notes use the smallest one found
					m_minResizeLen = quantization();
					for (Note *note : selectedNotes)
					{
						//Notes from the pattern editor can have a negative length, so
						//change their length to the displayed one before resizing
						if (note->oldLength() <= 0) { note->setOldLength(4); }
						//Let the note be sized down by quantized increments, stopping
						//when the next step down would result in a negative length
						int thisMin = note->oldLength() % quantization();
						//The initial value for m_minResizeLen is the minimum length of
						//a note divisible by the current Q. Therefore we ignore notes
						//where thisMin == 0 when checking for a new minimum
						if (thisMin > 0 && thisMin < m_minResizeLen) { m_minResizeLen = thisMin; }
					}

					// set resize-cursor
					setCursor( Qt::SizeHorCursor );
				}
				else
				{
					if( ! created_new_note )
					{
						m_midiClip->addJournalCheckPoint();
					}

					// otherwise move it
					m_action = Action::MoveNote;

					// set move-cursor
					setCursor( Qt::SizeAllCursor );

					// if they're holding shift, copy all selected notes
					if( ! is_new_note && me->modifiers() & Qt::ShiftModifier )
					{
						for (Note *note: selectedNotes)
						{
							Note *newNote = m_midiClip->addNote(*note, false);
							newNote->setSelected(false);
						}

						if (!selectedNotes.empty())
						{
							// added new notes, so must update engine, song, etc
							Engine::getSong()->setModified();
							update();
							getGUI()->songEditor()->update();
						}
					}

					// play the note
					testPlayNote( m_currentNote );
				}

				Engine::getSong()->setModified();
			}
			else if( ( me->buttons() == Qt::RightButton &&
							m_editMode == EditMode::Draw ) ||
					m_editMode == EditMode::Erase )
			{
				// erase single note
				m_mouseDownRight = true;
				if (it != notes.rend())
				{
					m_midiClip->addJournalCheckPoint();
					m_midiClip->removeNote( *it );
					Engine::getSong()->setModified();
				}
			}
			else if( me->button() == Qt::LeftButton &&
							m_editMode == EditMode::Select )
			{
				// select an area of notes

				m_selectStartTick = pos_ticks;
				m_selectedTick = 0;
				m_selectStartKey = key_num;
				m_selectedKeys = 1;
				m_action = Action::SelectNotes;

				// call mousemove to fix glitch where selection
				// appears in wrong spot on mousedown
				mouseMoveEvent( me );
			}

			update();
		}
		else if( me->y() < keyAreaBottom() )
		{
			// reference to last key needed for both
			// right click (used for copy all keys on note)
			// and for playing the key when left-clicked
			m_lastKey = key_num;

			// clicked on keyboard on the left
			if( me->buttons() == Qt::RightButton )
			{
				// right click - tone marker contextual menu
				m_pianoKeySelected = getKey( me->y() );
				m_semiToneMarkerMenu->popup( mapToGlobal( QPoint( me->x(), me->y() ) ) );
			}
			else if( me->buttons() == Qt::LeftButton )
			{
				// left click - play the note
				int v = ((float) x) / ((float) m_whiteKeyWidth) * MidiDefaultVelocity;
				m_midiClip->instrumentTrack()->pianoModel()->handleKeyPress(key_num, v);
				// if a chord is set, play the chords notes as well:
				playChordNotes(key_num, v);
			}
		}
		else
		{
			if( me->buttons() == Qt::LeftButton )
			{
				// clicked in the box below the keys to the left of note edit area
				m_noteEditMode = (NoteEditMode)(((int)m_noteEditMode)+1);
				if( m_noteEditMode == NoteEditMode::Count )
				{
					m_noteEditMode = (NoteEditMode) 0;
				}
				repaint();
			}
			else if( me->buttons() == Qt::RightButton )
			{
				// pop menu asking which one they want to edit
				m_noteEditMenu->popup( mapToGlobal( QPoint( me->x(), me->y() ) ) );
			}
		}
	}
}




void PianoRoll::mouseDoubleClickEvent(QMouseEvent * me )
{
	if( ! hasValidMidiClip() )
	{
		return;
	}

	// if they clicked in the note edit area, enter value for the volume bar
	if( me->x() > noteEditLeft() && me->x() < noteEditRight()
		&& me->y() > noteEditTop() && me->y() < noteEditBottom() )
	{
		// get values for going through notes
		int pixel_range = 4;
		int x = me->x() - m_whiteKeyWidth;
		const int ticks_start = ( x-pixel_range/2 ) *
					TimePos::ticksPerBar() / m_ppb + m_currentPosition;
		const int ticks_end = ( x+pixel_range/2 ) *
					TimePos::ticksPerBar() / m_ppb + m_currentPosition;
		const int ticks_middle = x * TimePos::ticksPerBar() / m_ppb + m_currentPosition;

		// go through notes to figure out which one we want to change
		bool altPressed = me->modifiers() & Qt::AltModifier;
		NoteVector nv;
		for ( Note * i : m_midiClip->notes() )
		{
			if( i->withinRange( ticks_start, ticks_end ) || ( i->selected() && !altPressed ) )
			{
				nv.push_back(i);
			}
		}
		// make sure we're on a note
		if( nv.size() > 0 )
		{
			const Note * closest = nullptr;
			int closest_dist = 9999999;
			// if we caught multiple notes and we're not editing a
			// selection, find the closest...
			if( nv.size() > 1 && !isSelection() )
			{
				for ( const Note * i : nv )
				{
					const int dist = qAbs( i->pos().getTicks() - ticks_middle );
					if( dist < closest_dist ) { closest = i; closest_dist = dist; }
				}
				// ... then remove all notes from the vector that aren't on the same exact time
				auto it = nv.begin();
				while (it != nv.end())
				{
					const Note *note = *it;
					if( note->pos().getTicks() != closest->pos().getTicks() )
					{
						it = nv.erase( it );
					}
					else
					{
						it++;
					}
				}
			}
			enterValue( &nv );
		}
	}
	else
	{
		QWidget::mouseDoubleClickEvent(me);
	}
}




void PianoRoll::testPlayNote( Note * n )
{
	m_lastKey = n->key();

	if( ! n->isPlaying() && ! m_recording && ! m_stepRecorder.isRecording())
	{
		n->setIsPlaying( true );

		const int baseVelocity = m_midiClip->instrumentTrack()->midiPort()->baseVelocity();

		m_midiClip->instrumentTrack()->pianoModel()->handleKeyPress(n->key(), n->midiVelocity(baseVelocity));

		// if a chord is set, play the chords notes as well:
		playChordNotes(n->key(), n->midiVelocity(baseVelocity));

		MidiEvent event( MidiMetaEvent, -1, n->key(), panningToMidi( n->getPanning() ) );

		event.setMetaEvent( MidiNotePanning );

		m_midiClip->instrumentTrack()->processInEvent( event, 0 );
	}
}




void PianoRoll::pauseTestNotes( bool pause )
{
	for (Note *note : m_midiClip->notes())
	{
		if( note->isPlaying() )
		{
			if( pause )
			{
				// stop note
				m_midiClip->instrumentTrack()->pianoModel()->handleKeyRelease( note->key() );

				// if a chord was set, stop the chords notes as well:
				pauseChordNotes(note->key());
			}
			else
			{
				// start note
				note->setIsPlaying( false );
				testPlayNote( note );
			}
		}
	}
}

void PianoRoll::playChordNotes(int key, int velocity)
{
	// if a chord is set, play the chords notes beside the base note.
	Piano *pianoModel = m_midiClip->instrumentTrack()->pianoModel();
	const InstrumentFunctionNoteStacking::Chord & chord =
			InstrumentFunctionNoteStacking::ChordTable::getInstance().getChordByName(
				m_chordModel.currentText());
	if (!chord.isEmpty())
	{
		for (int i = 1; i < chord.size(); ++i)
		{
			pianoModel->handleKeyPress(key + chord[i], velocity);
		}
	}
}

void PianoRoll::pauseChordNotes(int key)
{
	// if a chord was set, stop the chords notes beside the base note.
	Piano *pianoModel = m_midiClip->instrumentTrack()->pianoModel();
	const InstrumentFunctionNoteStacking::Chord & chord =
			InstrumentFunctionNoteStacking::ChordTable::getInstance().getChordByName(
				m_chordModel.currentText());
	if (!chord.isEmpty())
	{
		for (int i = 1; i < chord.size(); ++i)
		{
			pianoModel->handleKeyRelease(key + chord[i]);
		}
	}
}

void PianoRoll::setKnifeAction()
{
	if (m_editMode != EditMode::Knife)
	{
		m_knifeMode = m_editMode;
		m_editMode = EditMode::Knife;
		m_action = Action::Knife;
		m_knifeDown = false;
		setCursor(Qt::ArrowCursor);
		update();
	}
}

void PianoRoll::cancelKnifeAction()
{
	m_editMode = m_knifeMode;
	m_action = Action::None;
	m_knifeDown = false;
	update();
}

void PianoRoll::setStrumAction()
{
	if (m_editMode != EditMode::Strum)
	{
		m_strumMode = m_editMode;
		m_editMode = EditMode::Strum;
		m_action = Action::Strum;
		m_strumEnabled = false;
		setupSelectedChords();
		setCursor(Qt::ArrowCursor);
		update();
	}
}

void PianoRoll::cancelStrumAction()
{
	m_editMode = m_strumMode;
	m_action = Action::None;
	m_strumEnabled = false;
	update();
}

void PianoRoll::testPlayKey( int key, int velocity, int pan )
{
	Piano *pianoModel = m_midiClip->instrumentTrack()->pianoModel();
	// turn off old key
	pianoModel->handleKeyRelease( m_lastKey );
	// if a chord was set, stop the chords notes as well
	pauseChordNotes(m_lastKey);

	// remember which one we're playing
	m_lastKey = key;

	// play new key
	pianoModel->handleKeyPress( key, velocity );
	// and if a chord is set, play chord notes:
	playChordNotes(key, velocity);
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
	if( hasValidMidiClip() )
	{
		for( Note *note : m_midiClip->notes() )
		{
			// make a new selection unless they're holding shift
			if( ! shift )
			{
				note->setSelected( false );
			}

			int len_ticks = note->length();

			if( len_ticks == 0 )
			{
				continue;
			}
			else if( len_ticks < 0 )
			{
				len_ticks = 4;
			}

			const int key = note->key() - m_startKey + 1;

			int pos_ticks = note->pos();

			// if the selection even barely overlaps the note
			if( key > sel_key_start &&
				key <= sel_key_end &&
				pos_ticks + len_ticks > sel_pos_start &&
				pos_ticks < sel_pos_end )
			{
				// remove from selection when holding shift
				bool selected = shift && note->selected();
				note->setSelected( ! selected);
			}
		}
	}

	removeSelection();
	update();
}




void PianoRoll::mouseReleaseEvent( QMouseEvent * me )
{
	bool mustRepaint = false;

	s_textFloat->hide();

	// Quit knife mode or strum mode if we pressed and released the right mouse button
	if (m_editMode == EditMode::Knife && me->button() == Qt::RightButton)
	{
		cancelKnifeAction();
	}
	else if (m_editMode == EditMode::Strum && me->button() == Qt::RightButton)
	{
		cancelStrumAction();
	}

	if( me->button() & Qt::LeftButton )
	{
		mustRepaint = true;

		if( m_action == Action::SelectNotes && m_editMode == EditMode::Select )
		{
			// select the notes within the selection rectangle and
			// then destroy the selection rectangle
			computeSelectedNotes(
					me->modifiers() & Qt::ShiftModifier );
		}
		else if( m_action == Action::MoveNote )
		{
			// we moved one or more notes so they have to be
			// moved properly according to new starting-
			// time in the note-array of clip
			m_midiClip->rearrangeAllNotes();

		}
		else if (m_action == Action::Strum || m_strumEnabled)
		{
			m_strumEnabled = false;
    }
		else if (m_action == Action::Knife && hasValidMidiClip())
		{
			bool deleteShortEnds = me->modifiers() & Qt::ShiftModifier;
			const NoteVector selectedNotes = getSelectedNotes();
			m_midiClip->splitNotesAlongLine(!selectedNotes.empty() ? selectedNotes : m_midiClip->notes(), TimePos(m_knifeStartTickPos), m_knifeStartKey, TimePos(m_knifeEndTickPos), m_knifeEndKey, deleteShortEnds);
			m_knifeDown = false;
		}

		if( m_action == Action::MoveNote || m_action == Action::ResizeNote )
		{
			// if we only moved one note, deselect it so we can
			// edit the notes in the note edit area
			if( selectionCount() == 1 )
			{
				clearSelectedNotes();
			}
		}
	}

	if( me->button() & Qt::RightButton )
	{
		m_mouseDownRight = false;
		mustRepaint = true;
	}

	if( hasValidMidiClip() )
	{
		// turn off all notes that are playing
		for ( Note *note : m_midiClip->notes() )
		{
			if( note->isPlaying() )
			{
				m_midiClip->instrumentTrack()->pianoModel()->
						handleKeyRelease( note->key() );
				pauseChordNotes(note->key());
				note->setIsPlaying( false );
			}
		}

		// stop playing keys that we let go of
		m_midiClip->instrumentTrack()->pianoModel()->
						handleKeyRelease( m_lastKey );
		pauseChordNotes(m_lastKey);
	}

	m_currentNote = nullptr;

	if (m_action != Action::Knife && m_action != Action::Strum)
	{
		m_action = Action::None;
	}

	if( m_editMode == EditMode::Draw )
	{
		setCursor( Qt::ArrowCursor );
	}

	if( mustRepaint )
	{
		repaint();
	}
}




void PianoRoll::mouseMoveEvent( QMouseEvent * me )
{
	if( ! hasValidMidiClip() )
	{
		update();
		return;
	}

	if( m_action == Action::None && me->buttons() == 0 )
	{
		// When cursor is between note editing area and volume/panning
		// area show vertical size cursor.
		if( me->y() > keyAreaBottom() && me->y() < noteEditTop() )
		{
			setCursor( Qt::SizeVerCursor );
			return;
		}
	}
	else if( m_action == Action::ResizeNoteEditArea )
	{
		// Don't try to show more keys than the full keyboard, bail if trying to
		if (m_pianoKeysVisible == NumKeys && me->y() > m_moveStartY)
		{
			return;
		}
		int newHeight = height() - me->y();
		if (me->y() < KEY_AREA_MIN_HEIGHT)
		{
			newHeight = height() - KEY_AREA_MIN_HEIGHT -
				PR_TOP_MARGIN - PR_BOTTOM_MARGIN; // - NOTE_EDIT_RESIZE_BAR
		}
		// change m_notesEditHeight and then repaint
		m_notesEditHeight = qMax(NOTE_EDIT_MIN_HEIGHT, newHeight);
		m_userSetNotesEditHeight = m_notesEditHeight;
		m_stepRecorderWidget.setBottomMargin(PR_BOTTOM_MARGIN + m_notesEditHeight);
		updateScrollbars();
		updatePositionLineHeight();
		repaint();
		return;
	}

	// Update Knife position if we are on knife mode
	if (m_editMode == EditMode::Knife)
	{
		updateKnifePos(me, false);
	}

	// Update Strum position if we are on knife mode
	if (m_editMode == EditMode::Strum && m_strumEnabled)
	{
		updateStrumPos(me, false, me->modifiers() & Qt::ShiftModifier);
	}

	if( me->y() > PR_TOP_MARGIN || m_action != Action::None )
	{
		bool edit_note = ( me->y() > noteEditTop() )
						&& m_action != Action::SelectNotes;


		int key_num = getKey( me->y() );
		int x = me->x();

		// see if they clicked on the keyboard on the left
		if (x < m_whiteKeyWidth && m_action == Action::None
		    && ! edit_note && key_num != m_lastKey
			&& me->buttons() & Qt::LeftButton )
		{
			// clicked on a key, play the note
			testPlayKey(key_num, ((float) x) / ((float) m_whiteKeyWidth) * MidiDefaultVelocity, 0);
			update();
			return;
		}

		x -= m_whiteKeyWidth;

		if( me->buttons() & Qt::LeftButton
			&& m_editMode == EditMode::Draw
			&& (m_action == Action::MoveNote || m_action == Action::ResizeNote ) )
		{
			// handle moving notes and resizing them
			bool replay_note = key_num != m_lastKey
							&& m_action == Action::MoveNote;

			if( replay_note || ( m_action == Action::MoveNote && ( me->modifiers() & Qt::ShiftModifier ) && ! m_startedWithShift ) )
			{
				pauseTestNotes();
			}

			dragNotes(
				me->x(),
				me->y(),
				me->modifiers() & Qt::AltModifier,
				me->modifiers() & Qt::ShiftModifier,
				me->modifiers() & Qt::ControlModifier
			);

			if( replay_note && m_action == Action::MoveNote && ! ( ( me->modifiers() & Qt::ShiftModifier ) && ! m_startedWithShift ) )
			{
				pauseTestNotes( false );
			}
		}
		else if( m_editMode != EditMode::Erase &&
			( edit_note || m_action == Action::ChangeNoteProperty ) &&
				( me->buttons() & Qt::LeftButton || me->buttons() & Qt::MiddleButton
				|| ( me->buttons() & Qt::RightButton && me->modifiers() & Qt::ShiftModifier ) ) )
		{
			// editing note properties

			// Change notes within a certain pixel range of where
			// the mouse cursor is
			int pixel_range = 14;

			// convert to ticks so that we can check which notes
			// are in the range
			int ticks_start = ( x-pixel_range/2 ) *
					TimePos::ticksPerBar() / m_ppb + m_currentPosition;
			int ticks_end = ( x+pixel_range/2 ) *
					TimePos::ticksPerBar() / m_ppb + m_currentPosition;

			// get note-vector of current MIDI clip
			const NoteVector & notes = m_midiClip->notes();

			// determine what volume/panning to set note to
			// if middle-click, set to defaults
			volume_t vol = DefaultVolume;
			panning_t pan = DefaultPanning;

			if( me->buttons() & Qt::LeftButton )
			{
				vol = qBound<int>( MinVolume,
								MinVolume +
								( ( (float)noteEditBottom() ) - ( (float)me->y() ) ) /
								( (float)( noteEditBottom() - noteEditTop() ) ) *
								( MaxVolume - MinVolume ),
											MaxVolume );
				pan = qBound<int>( PanningLeft,
								PanningLeft +
								( (float)( noteEditBottom() - me->y() ) ) /
								( (float)( noteEditBottom() - noteEditTop() ) ) *
								( (float)( PanningRight - PanningLeft ) ),
										  PanningRight);
			}

			if( m_noteEditMode == NoteEditMode::Volume )
			{
				m_lastNoteVolume = vol;
				showVolTextFloat( vol, me->pos() );
			}
			else if( m_noteEditMode == NoteEditMode::Panning )
			{
				m_lastNotePanning = pan;
				showPanTextFloat( pan, me->pos() );
			}

			// When alt is pressed we only edit the note under the cursor
			bool altPressed = me->modifiers() & Qt::AltModifier;
			// We iterate from last note in MIDI clip to the first,
			// chronologically
			auto it = notes.rbegin();
			while (it != notes.rend())
			{
				Note* n = *it;

				bool isUnderPosition = n->withinRange( ticks_start, ticks_end );
				// Play note under the cursor
				if ( isUnderPosition ) { testPlayNote( n ); }
				// If note is:
				// Under the cursor, when there is no selection
				// Selected, and alt is not pressed
				// Under the cursor, selected, and alt is pressed
				if ( ( isUnderPosition && !isSelection() ) ||
					  ( n->selected() && !altPressed ) ||
					  ( isUnderPosition && n->selected() && altPressed )
					)
				{
					if( m_noteEditMode == NoteEditMode::Volume )
					{
						n->setVolume( vol );

						const int baseVelocity = m_midiClip->instrumentTrack()->midiPort()->baseVelocity();

						m_midiClip->instrumentTrack()->processInEvent( MidiEvent( MidiKeyPressure, -1, n->key(), n->midiVelocity( baseVelocity ) ) );
					}
					else if( m_noteEditMode == NoteEditMode::Panning )
					{
						n->setPanning( pan );
						MidiEvent evt( MidiMetaEvent, -1, n->key(), panningToMidi( pan ) );
						evt.setMetaEvent( MidiNotePanning );
						m_midiClip->instrumentTrack()->processInEvent( evt );
					}
				}
				else if( n->isPlaying() && !isSelection() )
				{
					// mouse not over this note, stop playing it.
					m_midiClip->instrumentTrack()->pianoModel()->handleKeyRelease( n->key() );
					pauseChordNotes(n->key());

					n->setIsPlaying( false );
				}


				++it;
			}

			// Emit MIDI clip has changed
			m_midiClip->dataChanged();
		}

		else if( me->buttons() == Qt::NoButton && m_editMode == EditMode::Draw )
		{
			// set move- or resize-cursor

			// get tick in which the cursor is posated
			int pos_ticks = ( x * TimePos::ticksPerBar() ) /
						m_ppb + m_currentPosition;

			// get note-vector of current MIDI clip
			const NoteVector & notes = m_midiClip->notes();

			// will be our iterator in the following loop
			auto it = notes.rbegin();

			// loop through whole note-vector...
			while (it != notes.rend())
			{
				Note *note = *it;
				// and check whether the cursor is over an
				// existing note
				if( pos_ticks >= note->pos() &&
			    		pos_ticks <= note->pos() +
							note->length() &&
					note->key() == key_num &&
					note->length() > 0 )
				{
					break;
				}
				++it;
			}

			// did it reach end of vector because there's
			// no note??
			if (it != notes.rend())
			{
				Note *note = *it;
				// x coordinate of the right edge of the note
				int noteRightX = ( note->pos() + note->length() -
					m_currentPosition) * m_ppb/TimePos::ticksPerBar();
				// cursor at the "tail" of the note?
				bool atTail = note->length() > 0 && x > noteRightX -
							RESIZE_AREA_WIDTH;
				Qt::CursorShape cursorShape = atTail ? Qt::SizeHorCursor :
													Qt::SizeAllCursor;
				setCursor( cursorShape );
			}
			else
			{
				// the cursor is over no note, so restore cursor
				setCursor( Qt::ArrowCursor );
			}
		}
		else if( me->buttons() & Qt::LeftButton &&
						m_editMode == EditMode::Select &&
						m_action == Action::SelectNotes )
		{
			// change size of selection

			// get tick in which the cursor is posated
			int pos_ticks = x * TimePos::ticksPerBar() / m_ppb +
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
		else if( ( m_editMode == EditMode::Draw && me->buttons() & Qt::RightButton )
				|| ( m_editMode == EditMode::Erase && me->buttons() ) )
		{
			// holding down right-click to delete notes or holding down
			// any key if in erase mode

			// get tick in which the user clicked
			int pos_ticks = x * TimePos::ticksPerBar() / m_ppb +
							m_currentPosition;


			// get note-vector of current MIDI clip
			const NoteVector & notes = m_midiClip->notes();

			// will be our iterator in the following loop
			auto it = notes.begin();

			// loop through whole note-vector...
			while( it != notes.end() )
			{
				Note *note = *it;
				TimePos len = note->length();
				if( len < 0 )
				{
					len = 4;
				}
				// and check whether the user clicked on an
				// existing note or an edit-line
				if( pos_ticks >= note->pos() &&
						len > 0 &&
					(
					( ! edit_note &&
					pos_ticks <= note->pos() + len &&
					note->key() == key_num )
					||
					( edit_note &&
					pos_ticks <= note->pos() +
							NOTE_EDIT_LINE_WIDTH *
						TimePos::ticksPerBar() /
								m_ppb )
					)
					)
				{
					// delete this note
					it = m_midiClip->removeNote(it);
					Engine::getSong()->setModified();
				}
				else
				{
					++it;
				}
			}
		}
		else if (me->buttons() == Qt::NoButton && m_editMode != EditMode::Draw && m_editMode != EditMode::Knife && m_editMode != EditMode::Strum)
		{
			// Is needed to restore cursor when it previously was set to
			// Qt::SizeVerCursor (between keyAreaBottom and noteEditTop)
			setCursor( Qt::ArrowCursor );
		}
	}
	else
	{
		if( me->buttons() & Qt::LeftButton &&
					m_editMode == EditMode::Select &&
					m_action == Action::SelectNotes )
		{

			int x = me->x() - m_whiteKeyWidth;
			if( x < 0 && m_currentPosition > 0 )
			{
				x = 0;
				QCursor::setPos( mapToGlobal( QPoint(
							m_whiteKeyWidth,
							me->y() ) ) );
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
			else if (x > width() - m_whiteKeyWidth)
			{
				x = width() - m_whiteKeyWidth;
				QCursor::setPos( mapToGlobal( QPoint( width(),
							me->y() ) ) );
				m_leftRightScroll->setValue( m_currentPosition +
									4 );
			}

			// get tick in which the cursor is posated
			int pos_ticks = x * TimePos::ticksPerBar()/ m_ppb +
							m_currentPosition;

			m_selectedTick = pos_ticks -
							m_selectStartTick;
			if( (int) m_selectStartTick + m_selectedTick <
									0 )
			{
				m_selectedTick = -static_cast<int>(
							m_selectStartTick );
			}


			int key_num = getKey( me->y() );
			int visible_keys = ( height() - PR_TOP_MARGIN -
						PR_BOTTOM_MARGIN -
						m_notesEditHeight ) /
							m_keyLineHeight + 2;
			const int s_key = m_startKey - 1;

			if( key_num <= s_key )
			{
				QCursor::setPos( mapToGlobal( QPoint( me->x(),
							keyAreaBottom() ) ) );
				m_topBottomScroll->setValue(
					m_topBottomScroll->value() + 1 );
				key_num = s_key;
			}
			else if( key_num >= s_key + visible_keys )
			{
				QCursor::setPos( mapToGlobal( QPoint( me->x(),
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
		setCursor( Qt::ArrowCursor );
	}

	m_lastMouseX = me->x();
	m_lastMouseY = me->y();

	update();
}




void PianoRoll::updateKnifePos(QMouseEvent* me, bool initial)
{
	// Calculate the TimePos from the mouse
	int mouseViewportPosX = me->x() - m_whiteKeyWidth;
	int mouseViewportPosY = keyAreaBottom() - 1 - me->y();
	int mouseTickPos = mouseViewportPosX * TimePos::ticksPerBar() / m_ppb + m_currentPosition;
	int mouseKey = std::round(1.f * mouseViewportPosY / m_keyLineHeight) + m_startKey - 1;

	// If ctrl is not pressed, quantize the position
	if (!(me->modifiers() & Qt::ControlModifier))
	{
		mouseTickPos = std::round(1.f * mouseTickPos / quantization()) * quantization();
	}

	if (initial)
	{
		m_knifeStartTickPos = mouseTickPos;
		m_knifeStartKey = mouseKey;
	}
	m_knifeEndTickPos = mouseTickPos;
	m_knifeEndKey = mouseKey;
}

/*
 * Setup chords
 *
 * A chord is an island of notes--as the loop goes over the notes, if the notes overlap,
 * they are part of the same chord. Else, they are part of a new chord.
*/
void PianoRoll::setupSelectedChords()
{
	if (!hasValidMidiClip()) { return; }
	m_selectedChords.clear();
	m_midiClip->rearrangeAllNotes();

	const NoteVector& selectedNotes = getSelectedNotes();
	if (selectedNotes.empty()) { return; }

	int maxTime = -1;
	NoteVector currentChord;
	for (Note* note: selectedNotes)
	{
		// If the note is not in the current chord range (and this isn't the first chord), start a new chord.
		if (note->pos() >= maxTime && maxTime != -1)
		{
			// Sort the notes by key before adding the chord to the vector
			std::sort(currentChord.begin(), currentChord.end(), [](Note* a, Note* b){ return a->key() < b->key(); });
			m_selectedChords.push_back(currentChord);
			currentChord.clear();
			maxTime = note->endPos();
		}
		maxTime = std::max(maxTime, static_cast<int>(note->endPos()));
		currentChord.push_back(note);
	}
	// Add final chord
	std::sort(currentChord.begin(), currentChord.end(), [](Note* a, Note* b){ return a->key() < b->key(); });
	m_selectedChords.push_back(currentChord);
}

/*
 * Perform the Strum
 *
 * Notes above the clicked note (relative to each chord) will be strummed down, notes below will be strummed up.
 * Holding shift raises the amount of movement to a power, causing the strum to be curved/warped.
*/
void PianoRoll::updateStrumPos(QMouseEvent* me, bool initial, bool warp)
{
	if (!hasValidMidiClip()) { return; }
	// Calculate the TimePos from the mouse
	int mouseViewportPos = me->x() - m_whiteKeyWidth;
	int mouseTickPos = mouseViewportPos * TimePos::ticksPerBar() / m_ppb + m_currentPosition;
	// Should we add quantization? probably not?
	if (initial)
	{
		m_strumStartTime = mouseTickPos;
		m_strumStartVertical =  me->y();
	}
	m_strumCurrentTime = mouseTickPos;
	m_strumCurrentVertical =  me->y();
	int strumTicksHorizontal = m_strumCurrentTime - m_strumStartTime;
	float strumPower = fastPow10f(0.01f * (m_strumCurrentVertical - m_strumStartVertical));

	if (initial)
	{
		m_midiClip->addJournalCheckPoint();

		Note* clickedNote = noteUnderMouse();
		if (clickedNote == nullptr) { return; }

		for (NoteVector chord: m_selectedChords)
		{
			for (Note* note: chord)
			{
				// Save the current note position
				note->setOldPos(note->pos());
				// if this is the clicked note, calculate it's ratio up the chord
				if (note == clickedNote && chord.size() > 1)
				{
					m_strumHeightRatio = 1.f * std::distance(chord.begin(), std::find(chord.begin(), chord.end(), clickedNote)) / (chord.size() - 1);
				}
			}
		}
	}

	for (NoteVector chord: m_selectedChords)
	{
		// Don't strum a chord with only one note
		if (chord.size() <= 1) { continue; }
		for (size_t i = 0; i < chord.size(); ++i)
		{
			float heightRatio = 1.f * i / (chord.size() - 1);
			float ratio = 0.0f;

			if (heightRatio == m_strumHeightRatio)
			{
				ratio = 1.f;
			}
			else if (heightRatio < m_strumHeightRatio)
			{
				ratio = heightRatio / m_strumHeightRatio;
			}
			else
			{
				ratio = (1.f - heightRatio) / (1.f - m_strumHeightRatio);
			}

			if (warp)
			{
				ratio = std::pow(ratio, strumPower);
			}
			chord.at(i)->setPos(std::max(0, static_cast<tick_t>(chord.at(i)->oldPos() + ratio * strumTicksHorizontal)));
		}
	}
	m_midiClip->rearrangeAllNotes();
	m_midiClip->updateLength();
	m_midiClip->dataChanged();
}



void PianoRoll::dragNotes(int x, int y, bool alt, bool shift, bool ctrl)
{
	// dragging one or more notes around

	// convert pixels to ticks and keys
	int off_x = x - m_moveStartX;
	int off_ticks = off_x * TimePos::ticksPerBar() / m_ppb;
	int off_key = getKey( y ) - getKey( m_moveStartY );

	// handle scroll changes while dragging
	off_ticks -= m_mouseDownTick - m_currentPosition;
	off_key -= m_mouseDownKey - m_startKey;

	// get note-vector of current MIDI clip
	const NoteVector & notes = m_midiClip->notes();

	if (m_action == Action::MoveNote)
	{
		// Calculate the offset for either Nudge or Snap modes
		int noteOffset = off_ticks;
		if (m_gridMode == GridMode::Snap && quantization () > 1)
		{
			// Get the mouse timeline absolute position
			TimePos mousePos(m_currentNote->oldPos().getTicks() + off_ticks);

			// We create a mousePos that is relative to the end of the note instead
			// of the beginning. That's to see if we will snap the beginning or end
			// of the note
			TimePos mousePosEnd(mousePos);
			mousePosEnd += m_currentNote->oldLength();

			// Now we quantize the mouse position to snap it to the grid
			TimePos mousePosQ = mousePos.quantize(static_cast<float>(quantization()) / DefaultTicksPerBar);
			TimePos mousePosEndQ = mousePosEnd.quantize(static_cast<float>(quantization()) / DefaultTicksPerBar);

			bool snapEnd = std::abs(mousePosEndQ - mousePosEnd) < std::abs(mousePosQ - mousePos);

			// Set the offset
			noteOffset = snapEnd
			? mousePosEndQ.getTicks() - m_currentNote->oldPos().getTicks() - m_currentNote->oldLength().getTicks()
			: mousePosQ.getTicks() - m_currentNote->oldPos().getTicks();
		}
		else if (m_gridMode == GridMode::Nudge)
		{
			// if they're not holding alt, quantize the offset
			if (!alt)
			{
				noteOffset = floor(off_ticks / quantization()) * quantization();
			}
		}

		// Make sure notes won't go outside boundary conditions
		if (m_moveBoundaryLeft + noteOffset < 0)
		{
			noteOffset = -m_moveBoundaryLeft;
		}
		if (m_moveBoundaryTop + off_key >= NumKeys)
		{
			off_key = -m_moveBoundaryTop + NumKeys - 1;
		}
		if (m_moveBoundaryBottom + off_key < 0)
		{
			off_key = -m_moveBoundaryBottom;
		}

		// Apply offset to all selected notes
		for (Note *note : getSelectedNotes())
		{
			// Quick resize is only enabled on Nudge mode, since resizing the note
			// while in Snap mode breaks the calculation of the note offset
			if (shift && ! m_startedWithShift && m_gridMode == GridMode::Nudge)
			{
				// quick resize, toggled by holding shift after starting a note move, but not before
				int ticks_new = note->oldLength().getTicks() + noteOffset;
				if( ticks_new <= 0 )
				{
					ticks_new = 1;
				}
				note->setLength( TimePos( ticks_new ) );
				m_lenOfNewNotes = note->length();
			}
			else
			{
				// moving note

				// Final position of the note
				TimePos posTicks(note->oldPos().getTicks() + noteOffset);
				int key_num = note->oldKey() + off_key;

				note->setPos(posTicks);
				note->setKey(key_num);
			}
		}
	}
	else if (m_action == Action::ResizeNote)
	{
		// When resizing notes:
		// If shift is not pressed, resize the selected notes but do not rearrange them
		// If shift is pressed we resize and rearrange only the selected notes
		// If shift + ctrl then we also rearrange all posterior notes (sticky)
		// If shift is pressed but only one note is selected, apply sticky

		// Quantize the resizing if alt is not pressed
		if (!alt)
		{
			off_ticks = floor(off_ticks / quantization()) * quantization();
		}

		auto selectedNotes = getSelectedNotes();

		if (shift)
		{
			// Algorithm:
			// Relative to the starting point of the left-most selected note,
			//   all selected note start-points and *endpoints* (not length) should be scaled by a calculated factor.
			// This factor is such that the endpoint of the note whose handle is being dragged should lie under the cursor.
			// first, determine the start-point of the left-most selected note:
			int stretchStartTick = -1;
			for (const Note *note : selectedNotes)
			{
				if (stretchStartTick < 0 || note->oldPos().getTicks() < stretchStartTick)
				{
					stretchStartTick = note->oldPos().getTicks();
				}
			}
			// determine the ending tick of the right-most selected note
			const Note *posteriorNote = nullptr;
			for (const Note *note : selectedNotes)
			{
				if (posteriorNote == nullptr ||
					note->oldPos().getTicks() + note->oldLength().getTicks() >
					posteriorNote->oldPos().getTicks() + posteriorNote->oldLength().getTicks())
				{
					posteriorNote = note;
				}
			}
			int posteriorEndTick = posteriorNote->pos().getTicks() + posteriorNote->length().getTicks();
			// end-point of the note whose handle is being dragged:
			int stretchEndTick = m_currentNote->oldPos().getTicks() + m_currentNote->oldLength().getTicks();
			// Calculate factor by which to scale the start-point and end-point of all selected notes
			float scaleFactor = (float)(stretchEndTick - stretchStartTick + off_ticks) / qMax(1, stretchEndTick - stretchStartTick);
			scaleFactor = qMax(0.0f, scaleFactor);

			// process all selected notes & determine how much the endpoint of the right-most note was shifted
			int posteriorDeltaThisFrame = 0;
			for (Note *note : selectedNotes)
			{
				// scale relative start and end positions by scaleFactor
				int newStart = stretchStartTick + scaleFactor *
					(note->oldPos().getTicks() - stretchStartTick);
				int newEnd = stretchStartTick + scaleFactor *
					(note->oldPos().getTicks()+note->oldLength().getTicks() - stretchStartTick);
				// if  not holding alt, quantize the offsets
				if (!alt)
				{
					// quantize start time
					int oldStart = note->oldPos().getTicks();
					int startDiff = newStart - oldStart;
					startDiff = floor(startDiff / quantization()) * quantization();
					newStart = oldStart + startDiff;
					// quantize end time
					int oldEnd = oldStart + note->oldLength().getTicks();
					int endDiff = newEnd - oldEnd;
					endDiff = floor(endDiff / quantization()) * quantization();
					newEnd = oldEnd + endDiff;
				}
				int newLength = qMax(1, newEnd-newStart);
				if (note == posteriorNote)
				{
					posteriorDeltaThisFrame = (newStart+newLength) -
						(note->pos().getTicks() + note->length().getTicks());
				}
				note->setLength( TimePos(newLength) );
				note->setPos( TimePos(newStart) );

				m_lenOfNewNotes = note->length();
			}
			if (ctrl || selectionCount() == 1)
			{
				// if holding ctrl or only one note is selected, reposition posterior notes
				for (Note *note : notes)
				{
					if (!note->selected() && note->pos().getTicks() >= posteriorEndTick)
					{
						int newStart = note->pos().getTicks() + posteriorDeltaThisFrame;
						note->setPos( TimePos(newStart) );
					}
				}
			}
		}
		else
		{
			// shift is not pressed; stretch length of selected notes but not their position
			int minLength = alt ? 1 : m_minResizeLen.getTicks();

			if (m_gridMode == GridMode::Snap)
			{
				// Calculate the end point of the note being dragged
				TimePos oldEndPoint = m_currentNote->oldPos() + m_currentNote->oldLength();
				// Quantize that position
				TimePos quantizedEndPoint = Note::quantized(oldEndPoint, quantization());
				// Add that difference to the offset from the resize
				off_ticks += quantizedEndPoint - oldEndPoint;
			}

			for (Note *note : selectedNotes)
			{
				int newLength = qMax(minLength, note->oldLength() + off_ticks);
				note->setLength(TimePos(newLength));

				m_lenOfNewNotes = note->length();
			}
		}
	}

	m_midiClip->updateLength();
	m_midiClip->dataChanged();
	Engine::getSong()->setModified();
}




void PianoRoll::paintEvent(QPaintEvent * pe )
{
	bool drawNoteNames = ConfigManager::inst()->value( "ui", "printnotelabels").toInt();

	QStyleOption opt;
	opt.initFrom( this );
	QPainter p( this );
	style()->drawPrimitive( QStyle::PE_Widget, &opt, &p, this );

	QBrush bgColor = p.background();

	// fill with bg color
	p.fillRect( 0, 0, width(), height(), bgColor );

	// set font-size to 80% of key line height
	QFont f = p.font();
	int keyFontSize = m_keyLineHeight * 0.8;
	p.setFont(adjustedToPixelSize(f, keyFontSize));
	QFontMetrics fontMetrics(p.font());
	// G-1 is one of the widest; plus one pixel margin for the shadow
	QRect const boundingRect = fontMetrics.boundingRect(QString("G-1")) + QMargins(0, 0, 1, 0);

	auto xCoordOfTick = [this](int tick) {
		return m_whiteKeyWidth + (
			(tick - m_currentPosition) * m_ppb / TimePos::ticksPerBar()
		);
	};

	// Order of drawing
	// - vertical quantization lines
	// - piano roll + horizontal key lines
	// - alternating bar colors
	// - vertical beat lines
	// - vertical bar lines
	// - marked semitones
	// - note editing
	// - notes
	// - selection frame
	// - highlight hovered note
	// - note edit area resize bar
	// - cursor mode icon

	if (hasValidMidiClip())
	{
		int pianoAreaHeight = keyAreaBottom() - keyAreaTop();
		m_pianoKeysVisible = pianoAreaHeight / m_keyLineHeight;
		int partialKeyVisible = pianoAreaHeight % m_keyLineHeight;
		// check if we're below the minimum key area size
		if (m_pianoKeysVisible * m_keyLineHeight < KEY_AREA_MIN_HEIGHT)
		{
			m_pianoKeysVisible = KEY_AREA_MIN_HEIGHT / m_keyLineHeight;
			partialKeyVisible = KEY_AREA_MIN_HEIGHT % m_keyLineHeight;
			// if we have a partial key, just show it
			if (partialKeyVisible > 0)
			{
				m_pianoKeysVisible += 1;
				partialKeyVisible = 0;
			}
			// have to modifiy the notes edit area height instead
			m_notesEditHeight = height() - (m_pianoKeysVisible * m_keyLineHeight)
				- PR_TOP_MARGIN - PR_BOTTOM_MARGIN;
		}
		// check if we're trying to show more keys than available
		else if (m_pianoKeysVisible >= NumKeys)
		{
			m_pianoKeysVisible = NumKeys;
			// have to modify the notes edit area height instead
			m_notesEditHeight = height() - (NumKeys * m_keyLineHeight) -
				PR_TOP_MARGIN - PR_BOTTOM_MARGIN;
			partialKeyVisible = 0;
		}
		int topKey = std::clamp(m_startKey + m_pianoKeysVisible - 1, 0, NumKeys - 1);
		int topNote = topKey % KeysPerOctave;
		// if not resizing the note edit area, we can change m_notesEditHeight
		if (m_action != Action::ResizeNoteEditArea && partialKeyVisible != 0)
		{
			// calculate the height change adding and subtracting the partial key
			int noteAreaPlus = (m_notesEditHeight + partialKeyVisible) - m_userSetNotesEditHeight;
			int noteAreaMinus = m_userSetNotesEditHeight - (m_notesEditHeight - partialKeyVisible);
			// if adding the partial key to height is more distant from the set height
			// we want to subtract the partial key
			if (noteAreaPlus > noteAreaMinus)
			{
				m_notesEditHeight -= partialKeyVisible;
				// since we're adding a partial key, we add one to the number visible
				m_pianoKeysVisible += 1;
			}
			// otherwise we add height
			else { m_notesEditHeight += partialKeyVisible; }
		}
		int x, q = quantization(), tick;

		// draw vertical quantization lines
		// If we're over 100% zoom, we allow all quantization level grids
		if (m_zoomingModel.value() <= 3)
		{
			// we're under 100% zoom
			// allow quantization grid up to 1/24 for triplets
			if (q % 3 != 0 && q < 8) { q = 8; }
			// allow quantization grid up to 1/32 for normal notes
			else if (q < 6) { q = 6; }
		}
    
		p.setPen(m_lineColor);
		for (tick = m_currentPosition - m_currentPosition % q,
			x = xCoordOfTick(tick);
			x <= width();
			tick += q, x = xCoordOfTick(tick))
		{
			p.drawLine(x, keyAreaTop(), x, noteEditBottom());
		}

		// draw horizontal grid lines and piano notes
		p.setClipRect(0, keyAreaTop(), width(), keyAreaBottom() - keyAreaTop());
		// the first grid line from the top Y position
		int grid_line_y = keyAreaTop() + m_keyLineHeight - 1;

		// lambda function for returning the height of a key
		auto keyHeight = [&](
			const int key
		) -> int
		{
			switch (prKeyOrder[key % KeysPerOctave])
			{
			case KeyType::WhiteBig:
				return m_whiteKeyBigHeight;
			case KeyType::WhiteSmall:
				return m_whiteKeySmallHeight;
			case KeyType::Black:
				return m_blackKeyHeight;
			}
			return 0; // should never happen
		};
		// lambda function for returning the distance to the top of a key
		auto gridCorrection = [&](
			const int key
		) -> int
		{
			const int keyCode = key % KeysPerOctave;
			switch (prKeyOrder[keyCode])
			{
			case KeyType::WhiteBig:
				return m_whiteKeySmallHeight;
			case KeyType::WhiteSmall:
				// These two keys need to adjust up small height instead of only key line height
				if (static_cast<Key>(keyCode) == Key::C || static_cast<Key>(keyCode) == Key::F)
				{
					return m_whiteKeySmallHeight;
				}
			case KeyType::Black:
				return m_blackKeyHeight;
			}
			return 0; // should never happen
		};
		auto keyWidth = [&](
			const int key
		) -> int
		{
			switch (prKeyOrder[key % KeysPerOctave])
			{
			case KeyType::WhiteSmall:
			case KeyType::WhiteBig:
				return m_whiteKeyWidth;
			case KeyType::Black:
				return m_blackKeyWidth;
			}
			return 0; // should never happen
		};
		// lambda function to draw a key
		auto drawKey = [&](
			const int key,
			const int yb)
		{
			const bool mapped = m_midiClip->instrumentTrack()->isKeyMapped(key);
			const bool pressed = m_midiClip->instrumentTrack()->pianoModel()->isKeyPressed(key);
			const int keyCode = key % KeysPerOctave;
			const int yt = yb - gridCorrection(key);
			const int kh = keyHeight(key);
			const int kw = keyWidth(key);
			// set key colors
			p.setPen(QColor(0, 0, 0));
			switch (prKeyOrder[keyCode])
			{
			case KeyType::WhiteSmall:
			case KeyType::WhiteBig:
				if (mapped)
				{
					if (pressed) { p.setBrush(m_whiteKeyActiveBackground); }
					else { p.setBrush(m_whiteKeyInactiveBackground); }
				}
				else
				{
					p.setBrush(m_whiteKeyDisabledBackground);
				}
				break;
			case KeyType::Black:
				if (mapped)
				{
					if (pressed) { p.setBrush(m_blackKeyActiveBackground); }
					else { p.setBrush(m_blackKeyInactiveBackground); }
				}
				else
				{
					p.setBrush(m_blackKeyDisabledBackground);
				}
			}
			// draw key
			p.drawRect(PIANO_X, yt, kw, kh);
			// draw note name
			if (static_cast<Key>(keyCode) == Key::C || (drawNoteNames && Piano::isWhiteKey(key)))
			{
				// small font sizes have 1 pixel offset instead of 2
				auto zoomOffset = m_zoomYLevels[m_zoomingYModel.value()] > 1.0f ? 2 : 1;
				QString noteString = getNoteString(key);
				QRect textRect(
					m_whiteKeyWidth - boundingRect.width() - 2,
					yb - m_keyLineHeight + zoomOffset,
					boundingRect.width(),
					boundingRect.height()
				);
				p.setPen(pressed ? m_whiteKeyActiveTextShadow : m_whiteKeyInactiveTextShadow);
				p.drawText(textRect.adjusted(0, 1, 1, 0), Qt::AlignRight | Qt::AlignHCenter, noteString);
				p.setPen(pressed ? m_whiteKeyActiveTextColor : m_whiteKeyInactiveTextColor);
				// if (static_cast<Key>(keyCode) == Key::C) { p.setPen(textColor()); }
				// else { p.setPen(textColorLight()); }
				p.drawText(textRect, Qt::AlignRight | Qt::AlignHCenter, noteString);
			}
		};
		// lambda for drawing the horizontal grid line
		auto drawHorizontalLine = [&](
			const int key,
			const int y
		)
		{
			if (static_cast<Key>(key % KeysPerOctave) == Key::C) { p.setPen(m_beatLineColor); }
			else { p.setPen(m_lineColor); }
			p.drawLine(m_whiteKeyWidth, y, width(), y);
		};
		// correct y offset of the top key
		switch (prKeyOrder[topNote])
		{
		case KeyType::WhiteSmall:
		case KeyType::WhiteBig:
			break;
		case KeyType::Black:
			// draw extra white key
			drawKey(topKey + 1, grid_line_y - m_keyLineHeight);
		}
		// loop through visible keys
		const int lastKey = qMax(0, topKey - m_pianoKeysVisible);
		for (int key = topKey; key > lastKey; --key)
		{
			bool whiteKey = Piano::isWhiteKey(key);
			if (whiteKey)
			{
				drawKey(key, grid_line_y);
				drawHorizontalLine(key, grid_line_y);
				grid_line_y += m_keyLineHeight;
			}
			else
			{
				// draw next white key
				drawKey(key - 1, grid_line_y + m_keyLineHeight);
				drawHorizontalLine(key - 1, grid_line_y + m_keyLineHeight);
				// draw black key over previous and next white key
				drawKey(key, grid_line_y);
				drawHorizontalLine(key, grid_line_y);
				// drew two grid keys so skip ahead properly
				grid_line_y += m_keyLineHeight + m_keyLineHeight;
				// capture double key draw
				--key;
			}
		}

		// don't draw over keys
		p.setClipRect(m_whiteKeyWidth, keyAreaTop(), width(), noteEditBottom() - keyAreaTop());

		// draw alternating shading on bars
		float timeSignature =
			static_cast<float>(Engine::getSong()->getTimeSigModel().getNumerator()) /
			static_cast<float>(Engine::getSong()->getTimeSigModel().getDenominator());
		float zoomFactor = m_zoomLevels[m_zoomingModel.value()];
		//the bars which disappears at the left side by scrolling
		int leftBars = m_currentPosition * zoomFactor / TimePos::ticksPerBar();
		//iterates the visible bars and draw the shading on uneven bars
		for (int x = m_whiteKeyWidth, barCount = leftBars;
			x < width() + m_currentPosition * zoomFactor / timeSignature;
			x += m_ppb, ++barCount)
		{
			if ((barCount + leftBars) % 2 != 0)
			{
				p.fillRect(x - m_currentPosition * zoomFactor / timeSignature,
					PR_TOP_MARGIN,
					m_ppb,
					height() - (PR_BOTTOM_MARGIN + PR_TOP_MARGIN),
					m_backgroundShade);
			}
		}

		// draw vertical beat lines
		int ticksPerBeat = DefaultTicksPerBar /
			Engine::getSong()->getTimeSigModel().getDenominator();
		p.setPen(m_beatLineColor);
		for(tick = m_currentPosition - m_currentPosition % ticksPerBeat,
			x = xCoordOfTick( tick );
			x <= width();
			tick += ticksPerBeat, x = xCoordOfTick(tick))
		{
			p.drawLine(x, PR_TOP_MARGIN, x, noteEditBottom());
		}

		// draw vertical bar lines
		p.setPen(m_barLineColor);
		for(tick = m_currentPosition - m_currentPosition % TimePos::ticksPerBar(),
			x = xCoordOfTick( tick );
			x <= width();
			tick += TimePos::ticksPerBar(), x = xCoordOfTick(tick))
		{
			p.drawLine(x, PR_TOP_MARGIN, x, noteEditBottom());
		}

		// draw marked semitones after the grid
		for(x = 0; x < m_markedSemiTones.size(); ++x)
		{
			const int key_num = m_markedSemiTones.at(x);
			const int y = keyAreaBottom() - 1 - m_keyLineHeight *
				(key_num - m_startKey + 1);
			if(y >= keyAreaBottom() - 1) { break; }
			p.fillRect(m_whiteKeyWidth + 1,
				y,
				width() - 10,
				m_keyLineHeight + 1,
				m_markedSemitoneColor);
		}
	}

	// reset MIDI clip
	p.setClipRect(0, 0, width(), height());

	// erase the area below the piano, because there might be keys that
	// should be only half-visible
	p.fillRect( QRect( 0, keyAreaBottom(),
			m_whiteKeyWidth, noteEditBottom() - keyAreaBottom()), bgColor);

	// display note editing info
	f.setBold(false);
	p.setFont(adjustedToPixelSize(f, SMALL_FONT_SIZE));
	p.setPen(m_noteModeColor);
	p.drawText( QRect( 0, keyAreaBottom(),
					  m_whiteKeyWidth, noteEditBottom() - keyAreaBottom()),
			   Qt::AlignCenter | Qt::TextWordWrap,
			   m_nemStr.at(static_cast<int>(m_noteEditMode)) + ":" );

	// set clipping area, because we are not allowed to paint over
	// keyboard...
	p.setClipRect(
		m_whiteKeyWidth,
		PR_TOP_MARGIN,
		width() - m_whiteKeyWidth,
		height() - PR_TOP_MARGIN - PR_BOTTOM_MARGIN);

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
	if( hasValidMidiClip() )
	{
		p.setClipRect(
			m_whiteKeyWidth,
			PR_TOP_MARGIN,
			width() - m_whiteKeyWidth,
			height() - PR_TOP_MARGIN);

		const int topKey = qBound(0, m_startKey + m_pianoKeysVisible - 1, NumKeys - 1);
		const int bottomKey = topKey - m_pianoKeysVisible;

		QPolygonF editHandles;

		// Return a note's Y position on the grid
		auto noteYPos = [&](const int key)
		{
			return (topKey - key) * m_keyLineHeight + keyAreaTop() - 1;
		};

		// -- Begin ghost MIDI clip
		if( !m_ghostNotes.empty() )
		{
			for( const Note *note : m_ghostNotes )
			{
				int len_ticks = note->length();

				if( len_ticks == 0 )
				{
					continue;
				}
				else if( len_ticks < 0 )
				{
					len_ticks = 4;
				}

				int pos_ticks = note->pos();

				int note_width = len_ticks * m_ppb / TimePos::ticksPerBar();
				const int x = ( pos_ticks - m_currentPosition ) *
						m_ppb / TimePos::ticksPerBar();
				// skip this note if not in visible area at all
				if (!(x + note_width >= 0 && x <= width() - m_whiteKeyWidth))
				{
					continue;
				}

				// is the note in visible area?
				if (note->key() > bottomKey && note->key() <= topKey)
				{

					// we've done and checked all, let's draw the note
					drawNoteRect(
						p, x + m_whiteKeyWidth, noteYPos(note->key()), note_width,
						note, m_ghostNoteColor, m_ghostNoteTextColor, m_selectedNoteColor,
						m_ghostNoteOpacity, m_ghostNoteBorders, drawNoteNames);
				}

			}
		}
		// -- End ghost MIDI clip

		for( const Note *note : m_midiClip->notes() )
		{
			int len_ticks = note->length();

			if( len_ticks == 0 )
			{
				continue;
			}
			else if( len_ticks < 0 )
			{
				len_ticks = 4;
			}

			int pos_ticks = note->pos();

			int note_width = len_ticks * m_ppb / TimePos::ticksPerBar();
			const int x = ( pos_ticks - m_currentPosition ) *
					m_ppb / TimePos::ticksPerBar();
			// skip this note if not in visible area at all
			if (!(x + note_width >= 0 && x <= width() - m_whiteKeyWidth))
			{
				continue;
			}

			// is the note in visible area?
			if (note->key() > bottomKey && note->key() <= topKey)
			{
				// We've done and checked all, let's draw the note with
				// the appropriate color
				const auto fillColor = note->type() == Note::Type::Regular ? m_noteColor : m_stepNoteColor;

				drawNoteRect(
					p, x + m_whiteKeyWidth, noteYPos(note->key()), note_width,
					note, fillColor, m_noteTextColor, m_selectedNoteColor,
					m_noteOpacity, m_noteBorders, drawNoteNames
				);
			}

			// draw note editing stuff
			int editHandleTop = 0;
			if( m_noteEditMode == NoteEditMode::Volume )
			{
				QColor color = m_barColor.lighter(30 + (note->getVolume() * 90 / MaxVolume));
				if( note->selected() )
				{
					color = m_selectedNoteColor;
				}
				p.setPen( QPen( color, NOTE_EDIT_LINE_WIDTH ) );

				editHandleTop = noteEditBottom() -
					( (float)( note->getVolume() - MinVolume ) ) /
					( (float)( MaxVolume - MinVolume ) ) *
					( (float)( noteEditBottom() - noteEditTop() ) );

				p.drawLine( QLineF ( noteEditLeft() + x + 0.5, editHandleTop + 0.5,
							noteEditLeft() + x + 0.5, noteEditBottom() + 0.5 ) );

			}
			else if( m_noteEditMode == NoteEditMode::Panning )
			{
				QColor color = m_noteColor;
				if( note->selected() )
				{
					color = m_selectedNoteColor;
				}

				p.setPen( QPen( color, NOTE_EDIT_LINE_WIDTH ) );

				editHandleTop = noteEditBottom() -
					( (float)( note->getPanning() - PanningLeft ) ) /
					( (float)( (PanningRight - PanningLeft ) ) ) *
					( (float)( noteEditBottom() - noteEditTop() ) );

				p.drawLine( QLine( noteEditLeft() + x, noteEditTop() +
						( (float)( noteEditBottom() - noteEditTop() ) ) / 2.0f,
						    noteEditLeft() + x , editHandleTop ) );
			}
			editHandles << QPoint ( x + noteEditLeft(),
						editHandleTop );

			if( note->hasDetuningInfo() )
			{
				drawDetuningInfo(p, note, x + m_whiteKeyWidth, noteYPos(note->key()));
				p.setClipRect(
					m_whiteKeyWidth,
					PR_TOP_MARGIN,
					width() - m_whiteKeyWidth,
					height() - PR_TOP_MARGIN);
			}
		}

		// draw clip bounds
		p.fillRect(
			xCoordOfTick(m_midiClip->length() - m_midiClip->startTimeOffset()),
			PR_TOP_MARGIN,
			width() - 10,
			noteEditBottom(),
			m_outOfBoundsShade
		);
		p.fillRect(
			0,
			PR_TOP_MARGIN,
			xCoordOfTick(-m_midiClip->startTimeOffset()),
			noteEditBottom(),
			m_outOfBoundsShade
		);

		// -- Knife tool (draw cut line)
		if (m_action == Action::Knife && m_knifeDown)
		{
			int x1 = xCoordOfTick(m_knifeStartTickPos);
			int y1 = y_base - (m_knifeStartKey - m_startKey + 1) * m_keyLineHeight;
			int x2 = xCoordOfTick(m_knifeEndTickPos);
			int y2 = y_base - (m_knifeEndKey - m_startKey + 1) * m_keyLineHeight;

			p.setPen(QPen(m_knifeCutLineColor, 1));
			p.drawLine(x1, y1, x2, y2);
		}
		// -- End knife tool

		//draw current step recording notes
		for( const Note *note : m_stepRecorder.getCurStepNotes() )
		{
			int len_ticks = note->length();

			if( len_ticks == 0 )
			{
				continue;
			}


			int pos_ticks = note->pos();

			int note_width = len_ticks * m_ppb / TimePos::ticksPerBar();
			const int x = ( pos_ticks - m_currentPosition ) *
					m_ppb / TimePos::ticksPerBar();
			// skip this note if not in visible area at all
			if (!(x + note_width >= 0 && x <= width() - m_whiteKeyWidth))
			{
				continue;
			}

			// is the note in visible area?
			if (note->key() > bottomKey && note->key() <= topKey)
			{

				// we've done and checked all, let's draw the note
				drawNoteRect(
					p, x + m_whiteKeyWidth, noteYPos(note->key()), note_width,
					note, m_currentStepNoteColor, m_noteTextColor, m_selectedNoteColor,
					m_noteOpacity, m_noteBorders, drawNoteNames);
			}
		}

		p.setPen(QPen(m_noteColor, NOTE_EDIT_LINE_WIDTH + 2));
		p.drawPoints( editHandles );

	}
	else
	{
		QFont f = font();
		f.setBold(true);
		p.setFont(f);
		p.setPen( QApplication::palette().color( QPalette::Active,
							QPalette::BrightText ) );
		p.drawText(m_whiteKeyWidth + 20, PR_TOP_MARGIN + 40,
				tr( "Please open a clip by double-clicking "
								"on it!" ) );
	}

	p.setClipRect(
		m_whiteKeyWidth,
		PR_TOP_MARGIN,
		width() - m_whiteKeyWidth,
		height() - PR_TOP_MARGIN - m_notesEditHeight - PR_BOTTOM_MARGIN);

	// now draw selection-frame
	int x = ( ( sel_pos_start - m_currentPosition ) * m_ppb ) /
						TimePos::ticksPerBar();
	int w = ( ( ( sel_pos_end - m_currentPosition ) * m_ppb ) /
						TimePos::ticksPerBar() ) - x;
	int y = (int) y_base - sel_key_start * m_keyLineHeight;
	int h = (int) y_base - sel_key_end * m_keyLineHeight - y;
	p.setPen(m_selectedNoteColor);
	p.setBrush( Qt::NoBrush );
	p.drawRect(x + m_whiteKeyWidth, y, w, h);

	// TODO: Get this out of paint event
	int l = ( hasValidMidiClip() )? (int) m_midiClip->length() - m_midiClip->startTimeOffset() : 0;

	// reset scroll-range
	if( m_leftRightScroll->maximum() != l )
	{
		m_leftRightScroll->setRange( 0, l );
		m_leftRightScroll->setPageStep( l );
	}

	// set line colors
	auto editAreaCol = QColor(m_lineColor);
	auto currentKeyCol = QColor(m_beatLineColor);

	editAreaCol.setAlpha( 64 );
	currentKeyCol.setAlpha( 64 );

	// horizontal line for the key under the cursor
	if(hasValidMidiClip() && getGUI()->pianoRoll()->hasFocus())
	{
		int key_num = getKey( mapFromGlobal( QCursor::pos() ).y() );
		p.fillRect( 10, keyAreaBottom() + 3 - m_keyLineHeight *
					( key_num - m_startKey + 1 ), width() - 10, m_keyLineHeight - 7, currentKeyCol );
	}

	// bar to resize note edit area
	p.setClipRect( 0, 0, width(), height() );
	p.fillRect( QRect( 0, keyAreaBottom(),
					width()-PR_RIGHT_MARGIN, NOTE_EDIT_RESIZE_BAR ), editAreaCol );

	if (getGUI()->pianoRoll()->hasFocus())
	{
		const QPixmap * cursor = nullptr;
		// draw current edit-mode-icon below the cursor
		switch( m_editMode )
		{
			case EditMode::Draw:
				if( m_mouseDownRight )
				{
					cursor = &m_toolErase;
				}
				else if( m_action == Action::MoveNote )
				{
					cursor = &m_toolMove;
				}
				else
				{
					cursor = &m_toolDraw;
				}
				break;
			case EditMode::Erase:
				cursor = &m_toolErase;
				break;
			case EditMode::Select:
				cursor = &m_toolSelect;
				break;
			case EditMode::Detuning:
				cursor = &m_toolOpen;
				break;
			case EditMode::Knife:
				cursor = &m_toolKnife;
				break;
			case EditMode::Strum:
				cursor = &m_toolStrum;
				break;
		}
		QPoint mousePosition = mapFromGlobal( QCursor::pos() );
		if( cursor != nullptr && mousePosition.y() > keyAreaTop() && mousePosition.x() > noteEditLeft())
		{
			p.drawPixmap( mousePosition + QPoint( 8, 8 ), *cursor );
		}
	}
}




void PianoRoll::updateScrollbars()
{
	m_leftRightScroll->setGeometry(
		m_whiteKeyWidth,
		height() - SCROLLBAR_SIZE,
		width() - m_whiteKeyWidth,
		SCROLLBAR_SIZE
	);
	m_topBottomScroll->setGeometry(
		width() - SCROLLBAR_SIZE,
		PR_TOP_MARGIN,
		SCROLLBAR_SIZE,
		height() - PR_TOP_MARGIN - SCROLLBAR_SIZE
	);
	int pianoAreaHeight = keyAreaBottom() - PR_TOP_MARGIN;
	int numKeysVisible = pianoAreaHeight / m_keyLineHeight;
	m_totalKeysToScroll = qMax(0, NumKeys - numKeysVisible);
	m_topBottomScroll->setRange(0, m_totalKeysToScroll);
	if (m_startKey > m_totalKeysToScroll)
	{
		m_startKey = qMax(0, m_totalKeysToScroll);
	}
	m_topBottomScroll->setValue(m_totalKeysToScroll - m_startKey);
}

// responsible for moving/resizing scrollbars after window-resizing
void PianoRoll::resizeEvent(QResizeEvent* re)
{
	updatePositionLineHeight();
	updateScrollbars();
	m_timeLine->setFixedWidth(width());
	update();
}


void PianoRoll::adjustLeftRightScoll(int value)
{
	m_leftRightScroll->setValue(m_leftRightScroll->value() -
							value * 0.3f / m_zoomLevels[m_zoomingModel.value()]);
}



void PianoRoll::wheelEvent(QWheelEvent * we )
{
	we->accept();
	// handle wheel events for note edit area - for editing note vol/pan with mousewheel
	if(position(we).x() > noteEditLeft() && position(we).x() < noteEditRight()
	&& position(we).y() > noteEditTop() && position(we).y() < noteEditBottom())
	{
		if (!hasValidMidiClip()) {return;}
		// get values for going through notes
		int pixel_range = 8;
		int x = position(we).x() - m_whiteKeyWidth;
		int ticks_start = ( x - pixel_range / 2 ) *
					TimePos::ticksPerBar() / m_ppb + m_currentPosition;
		int ticks_end = ( x + pixel_range / 2 ) *
					TimePos::ticksPerBar() / m_ppb + m_currentPosition;

		// When alt is pressed we only edit the note under the cursor
		bool altPressed = we->modifiers() & Qt::AltModifier;
		// go through notes to figure out which one we want to change
		NoteVector nv;
		for ( Note * i : m_midiClip->notes() )
		{
			if( i->withinRange( ticks_start, ticks_end ) || ( i->selected() && !altPressed ) )
			{
				nv.push_back(i);
			}
		}
		if( nv.size() > 0 )
		{
			const int step = (we->angleDelta().y() > 0 ? 1 : -1) * (we->inverted() ? -1 : 1);

			if( m_noteEditMode == NoteEditMode::Volume )
			{
				for ( Note * n : nv )
				{
					volume_t vol = qBound<int>( MinVolume, n->getVolume() + step, MaxVolume );
					n->setVolume( vol );
				}
				bool allVolumesEqual = std::all_of( nv.begin(), nv.end(),
					[nv](const Note *note)
					{
						return note->getVolume() == nv[0]->getVolume();
					});
				if ( allVolumesEqual )
				{
					// show the volume hover-text only if all notes have the
					// same volume
					showVolTextFloat(nv[0]->getVolume(), position(we), 1000);
				}
			}
			else if( m_noteEditMode == NoteEditMode::Panning )
			{
				for ( Note * n : nv )
				{
					panning_t pan = qBound<int>( PanningLeft, n->getPanning() + step, PanningRight );
					n->setPanning( pan );
				}
				bool allPansEqual = std::all_of( nv.begin(), nv.end(),
					[nv](const Note *note)
					{
						return note->getPanning() == nv[0]->getPanning();
					});
				if ( allPansEqual )
				{
					// show the pan hover-text only if all notes have the same
					// panning
					showPanTextFloat( nv[0]->getPanning(), position( we ), 1000 );
				}
			}
			update();
		}
	}

	// not in note edit area, so handle scrolling/zooming and quantization change
	else
	if( we->modifiers() & Qt::ControlModifier && we->modifiers() & Qt::AltModifier )
	{
		int q = m_quantizeModel.value();
		if((we->angleDelta().x() + we->angleDelta().y()) > 0) // alt + scroll becomes horizontal scroll on KDE
		{
			q--;
		}
		else if((we->angleDelta().x() + we->angleDelta().y()) < 0) // alt + scroll becomes horizontal scroll on KDE
		{
			q++;
		}
		q = qBound( 0, q, m_quantizeModel.size() - 1 );
		m_quantizeModel.setValue( q );
	}
	else if( we->modifiers() & Qt::ControlModifier && we->modifiers() & Qt::ShiftModifier )
	{
		int l = m_noteLenModel.value();
		if(we->angleDelta().y() > 0)
		{
			l--;
		}
		else if(we->angleDelta().y() < 0)
		{
			l++;
		}
		l = qBound( 0, l, m_noteLenModel.size() - 1 );
		m_noteLenModel.setValue( l );
	}
	else if( we->modifiers() & Qt::ControlModifier )
	{
		int z = m_zoomingModel.value();
		if(we->angleDelta().y() > 0)
		{
			z++;
		}
		else if(we->angleDelta().y() < 0)
		{
			z--;
		}
		z = qBound( 0, z, m_zoomingModel.size() - 1 );

		int x = (position(we).x() - m_whiteKeyWidth) * TimePos::ticksPerBar();
		// ticks based on the mouse x-position where the scroll wheel was used
		int ticks = x / m_ppb;
		// what would be the ticks in the new zoom level on the very same mouse x
		int newTicks = x / (DEFAULT_PR_PPB * m_zoomLevels[z]);
		// scroll so the tick "selected" by the mouse x doesn't move on the screen
		m_leftRightScroll->setValue(m_leftRightScroll->value() + ticks - newTicks);
		// update combobox with zooming-factor
		m_zoomingModel.setValue( z );
	}

	// FIXME: Reconsider if determining orientation is necessary in Qt6.
	else if (std::abs(we->angleDelta().x()) > std::abs(we->angleDelta().y())) // scrolling is horizontal
	{
		adjustLeftRightScoll(we->angleDelta().x());
	}
	else if(we->modifiers() & Qt::ShiftModifier)
	{
		adjustLeftRightScoll(we->angleDelta().y());
	}
	else
	{
		m_topBottomScroll->setValue(m_topBottomScroll->value() -
							we->angleDelta().y() / 30);
	}
}




void PianoRoll::focusOutEvent( QFocusEvent * )
{
	if( hasValidMidiClip() )
	{
		for( int i = 0; i < NumKeys; ++i )
		{
			m_midiClip->instrumentTrack()->pianoModel()->midiEventProcessor()->processInEvent( MidiEvent( MidiNoteOff, -1, i, 0 ) );
			m_midiClip->instrumentTrack()->pianoModel()->setKeyState( i, false );
		}
	}
	if (m_editMode == EditMode::Knife) {
		m_editMode = m_knifeMode;
		m_action = Action::None;
	}
	else if (m_editMode == EditMode::Strum)
	{
		m_editMode = m_strumMode;
		m_action = Action::None;
	}
	else
	{
		m_editMode = m_ctrlMode;
	}
	update();
}

void PianoRoll::focusInEvent( QFocusEvent * ev )
{
	if ( hasValidMidiClip() )
	{
		// Assign midi device
		m_midiClip->instrumentTrack()->autoAssignMidiDevice(true);
	}
	QWidget::focusInEvent(ev);
}



int PianoRoll::getKey(int y) const
{
	// handle case that very top pixel maps to next key above
	if (y - keyAreaTop() <= 1) { y = keyAreaTop() + 2; }
	int key_num = qBound(
		0,
		// add + 1 to stay within the grid lines
		((keyAreaBottom() - y + 1) / m_keyLineHeight) + m_startKey,
		NumKeys - 1
	);
	return key_num;
}

QList<int> PianoRoll::getAllOctavesForKey( int keyToMirror ) const
{
	QList<int> keys;

	int trackKeysPerOctave = trackOctaveSize();
	for (int i = keyToMirror % trackKeysPerOctave; i < NumKeys; i += trackKeysPerOctave)
	{
		keys.append(i);
	}

	return keys;
}

Song::PlayMode PianoRoll::desiredPlayModeForAccompany() const
{
	if (m_midiClip->getTrack()->trackContainer() ==	Engine::patternStore())
	{
		return Song::PlayMode::Pattern;
	}
	return Song::PlayMode::Song;
}




void PianoRoll::play()
{
	if( ! hasValidMidiClip() )
	{
		return;
	}

	if( Engine::getSong()->playMode() != Song::PlayMode::MidiClip )
	{
		Engine::getSong()->playMidiClip( m_midiClip );
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}




void PianoRoll::record()
{
	if( Engine::getSong()->isPlaying() )
	{
		stop();
	}
	if( m_recording || ! hasValidMidiClip() )
	{
		return;
	}

	m_midiClip->addJournalCheckPoint();
	m_recording = true;

	Engine::getSong()->playMidiClip( m_midiClip, false );

	m_timeLine->isRecoridng = true;
	m_positionLine->isRecording = true;
}




void PianoRoll::recordAccompany()
{
	if( Engine::getSong()->isPlaying() )
	{
		stop();
	}
	if( m_recording || ! hasValidMidiClip() )
	{
		return;
	}

	m_midiClip->addJournalCheckPoint();
	m_recording = true;

	if( m_midiClip->getTrack()->trackContainer() == Engine::getSong() )
	{
		Engine::getSong()->playSong();
	}
	else
	{
		Engine::getSong()->playPattern();
	}

	auto* songEditor = GuiApplication::instance()->songEditor()->m_editor;

	songEditor->timeLine->isRecoridng = true;
	songEditor->positionLine->isRecording = true;
	m_positionLine->update();
}




bool PianoRoll::toggleStepRecording()
{
	if(m_stepRecorder.isRecording())
	{
		m_stepRecorder.stop();
	}
	else
	{
		if(hasValidMidiClip())
		{
			if(Engine::getSong()->isPlaying())
			{
				m_stepRecorder.start(0, newNoteLen());
			}
			else
			{
				m_stepRecorder.start(
					Engine::getSong()->getPlayPos(
						Song::PlayMode::MidiClip), newNoteLen());
			}
		}
	}

	bool isRecording = m_stepRecorder.isRecording();

	// hide playhead when step recording
	m_timeLine->isPlayheadVisible = !isRecording;
	m_positionLine->setVisible(!isRecording);
	m_positionLine->update();

	return isRecording;
}




void PianoRoll::stop()
{
	Engine::getSong()->stop();
	m_recording = false;
	m_scrollBack = m_timeLine->autoScroll() != TimeLineWidget::AutoScrollState::Disabled;

	auto* songEditor = GuiApplication::instance()->songEditor()->m_editor;

	songEditor->timeLine->isRecoridng = false;
	songEditor->positionLine->isRecording = false;
	m_timeLine->isRecoridng = false;
	m_positionLine->isRecording = false;
	m_positionLine->update();
}




void PianoRoll::startRecordNote(const Note & n )
{
	if(hasValidMidiClip())
	{
		if( m_recording &&
			Engine::getSong()->isPlaying() &&
			(Engine::getSong()->playMode() == desiredPlayModeForAccompany() ||
			Engine::getSong()->playMode() == Song::PlayMode::MidiClip ))
		{
			TimePos sub;
			if( Engine::getSong()->playMode() == Song::PlayMode::Song )
			{
				sub = m_midiClip->startPosition();
			}
			Note n1( 1, Engine::getSong()->getPlayPos(
						Engine::getSong()->playMode() ) - sub,
					n.key(), n.getVolume(), n.getPanning() );
			if( n1.pos() >= 0 )
			{
				m_recordingNotes << n1;
			}
		}
		else if (m_stepRecorder.isRecording())
		{
			m_stepRecorder.notePressed(n);
		}
	}
}




void PianoRoll::finishRecordNote(const Note & n )
{
	if(hasValidMidiClip())
	{
		if( m_recording &&
			Engine::getSong()->isPlaying() &&
				( Engine::getSong()->playMode() ==
						desiredPlayModeForAccompany() ||
					Engine::getSong()->playMode() ==
						Song::PlayMode::MidiClip ) )
		{
			for( QList<Note>::Iterator it = m_recordingNotes.begin();
						it != m_recordingNotes.end(); ++it )
			{
				if( it->key() == n.key() )
				{
					Note n1(n.length(), it->pos(),
							it->key(), it->getVolume(),
							it->getPanning(), n.detuning());

					if (m_doAutoQuantization)
					{
						n1.quantizeLength(quantization());
						n1.quantizePos(quantization());
					}
					m_midiClip->addNote(n1, false);
					update();
					m_recordingNotes.erase( it );
					break;
				}
			}
		}
		else if (m_stepRecorder.isRecording())
		{
			m_stepRecorder.noteReleased(n);
		}
	}
}




void PianoRoll::horScrolled(int new_pos )
{
	m_currentPosition = new_pos;
	m_stepRecorderWidget.setCurrentPosition(m_currentPosition);
	emit positionChanged( m_currentPosition );
	update();
}




void PianoRoll::verScrolled( int new_pos )
{
	// revert value
	m_startKey = qMax(0, m_totalKeysToScroll - new_pos);

	update();
}




void PianoRoll::setEditMode(int mode)
{
	m_ctrlMode = m_editMode = (EditMode) mode;
}




void PianoRoll::selectAll()
{
	if( ! hasValidMidiClip() )
	{
		return;
	}

	// if first_time = true, we HAVE to set the vars for select
	bool first_time = true;

	for( const Note *note : m_midiClip->notes() )
	{
		int len_ticks = static_cast<int>( note->length() ) > 0 ?
				static_cast<int>( note->length() ) : 1;

		const int key = note->key();

		int pos_ticks = note->pos();
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
		if( key >= m_selectedKeys + m_selectStartKey ||
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




// returns vector with pointers to all selected notes
NoteVector PianoRoll::getSelectedNotes() const
{
	NoteVector selectedNotes;

	if (hasValidMidiClip())
	{
		for( Note *note : m_midiClip->notes() )
		{
			if( note->selected() )
			{
				selectedNotes.push_back( note );
			}
		}
	}
	return selectedNotes;
}

// selects all notess associated with m_lastKey
void PianoRoll::selectNotesOnKey()
{
	if (hasValidMidiClip()) {
		for (Note * note : m_midiClip->notes()) {
			if (note->key() == m_lastKey) {
				note->setSelected(true);
			}
		}
	}
}

void PianoRoll::enterValue( NoteVector* nv )
{

	if( m_noteEditMode == NoteEditMode::Volume )
	{
		bool ok;
		int new_val;
		new_val = QInputDialog::getInt(	this, "Piano roll: note velocity",
					tr( "Please enter a new value between %1 and %2:" ).
						arg( MinVolume ).arg( MaxVolume ),
					(*nv)[0]->getVolume(),
					MinVolume, MaxVolume, 1, &ok );

		if( ok )
		{
			for ( Note * n : *nv )
			{
				n->setVolume( new_val );
			}
			m_lastNoteVolume = new_val;
		}
	}
	else if( m_noteEditMode == NoteEditMode::Panning )
	{
		bool ok;
		int new_val;
		new_val = QInputDialog::getInt(	this, "Piano roll: note panning",
					tr( "Please enter a new value between %1 and %2:" ).
							arg( PanningLeft ).arg( PanningRight ),
						(*nv)[0]->getPanning(),
						PanningLeft, PanningRight, 1, &ok );

		if( ok )
		{
			for ( Note * n : *nv )
			{
				n->setPanning( new_val );
			}
			m_lastNotePanning = new_val;
		}

	}
}


void PianoRoll::updateYScroll()
{
	m_topBottomScroll->setGeometry(width() - SCROLLBAR_SIZE, PR_TOP_MARGIN,
						SCROLLBAR_SIZE,
						height() - PR_TOP_MARGIN -
						SCROLLBAR_SIZE);

	const int visible_space = keyAreaBottom() - keyAreaTop();
	m_totalKeysToScroll = qMax(0, NumKeys - 1 - visible_space / m_keyLineHeight);

	m_topBottomScroll->setRange(0, m_totalKeysToScroll);

	if(m_startKey > m_totalKeysToScroll)
	{
		m_startKey = m_totalKeysToScroll;
	}
	m_topBottomScroll->setValue(m_totalKeysToScroll - m_startKey);
}


void PianoRoll::copyToClipboard( const NoteVector & notes ) const
{
	// For copyString() and MimeType enum class
	using namespace Clipboard;

	DataFile dataFile( DataFile::Type::ClipboardData );
	QDomElement note_list = dataFile.createElement( "note-list" );
	dataFile.content().appendChild( note_list );

	TimePos start_pos( notes.front()->pos().getBar(), 0 );
	for( const Note *note : notes )
	{
		Note clip_note( *note );
		clip_note.setPos( clip_note.pos( start_pos ) );
		clip_note.saveState( dataFile, note_list );
	}

	copyString( dataFile.toString(), MimeType::Default );
}




void PianoRoll::copySelectedNotes()
{
	NoteVector selected_notes = getSelectedNotes();

	if( ! selected_notes.empty() )
	{
		copyToClipboard( selected_notes );
	}
}




void PianoRoll::cutSelectedNotes()
{
	if( ! hasValidMidiClip() )
	{
		return;
	}

	NoteVector selected_notes = getSelectedNotes();

	if( ! selected_notes.empty() )
	{
		m_midiClip->addJournalCheckPoint();

		copyToClipboard( selected_notes );

		Engine::getSong()->setModified();

		for( Note *note : selected_notes )
		{
			// note (the memory of it) is also deleted by
			// MidiClip::removeNote(...) so we don't have to do that
			m_midiClip->removeNote( note );
		}
	}

	update();
	getGUI()->songEditor()->update();
}




void PianoRoll::pasteNotes()
{
	// For getString() and MimeType enum class
	using namespace Clipboard;

	if( ! hasValidMidiClip() )
	{
		return;
	}

	QString value = getString( MimeType::Default );

	if( ! value.isEmpty() )
	{
		DataFile dataFile( value.toUtf8() );

		QDomNodeList list = dataFile.elementsByTagName( Note::classNodeName() );

		// remove selection and select the newly pasted notes
		clearSelectedNotes();

		if( ! list.isEmpty() )
		{
			m_midiClip->addJournalCheckPoint();
		}

		for( int i = 0; ! list.item( i ).isNull(); ++i )
		{
			// create the note
			Note cur_note;
			cur_note.restoreState( list.item( i ).toElement() );
			cur_note.setPos( cur_note.pos() + Note::quantized( m_timeLine->pos(), quantization() ) );

			// select it
			cur_note.setSelected( true );

			// add to MIDI clip
			m_midiClip->addNote( cur_note, false );
		}

		// we only have to do the following lines if we pasted at
		// least one note...
		Engine::getSong()->setModified();
		update();
		getGUI()->songEditor()->update();
	}
}




//Return false if no notes are deleted
bool PianoRoll::deleteSelectedNotes()
{
	if (!hasValidMidiClip()) { return false; }

	auto selectedNotes = getSelectedNotes();
	if (selectedNotes.empty()) { return false; }

	m_midiClip->addJournalCheckPoint();

	for (Note* note: selectedNotes) { m_midiClip->removeNote( note ); }

	Engine::getSong()->setModified();
	update();
	getGUI()->songEditor()->update();
	return true;
}




void PianoRoll::autoScroll( const TimePos & t )
{
	const int w = width() - m_whiteKeyWidth;
	if (m_timeLine->autoScroll() == TimeLineWidget::AutoScrollState::Stepped) 
	{
		if (t > m_currentPosition + w * TimePos::ticksPerBar() / m_ppb)
		{
			m_leftRightScroll->setValue(t.getBar() * TimePos::ticksPerBar());
		}
		else if (t < m_currentPosition)
		{
			TimePos t2 = std::max(t - w * TimePos::ticksPerBar() *
						TimePos::ticksPerBar() / m_ppb, static_cast<tick_t>(0));
			m_leftRightScroll->setValue(t2.getBar() * TimePos::ticksPerBar());
		}
	}
	else if (m_timeLine->autoScroll() == TimeLineWidget::AutoScrollState::Continuous)
	{
		m_leftRightScroll->setValue(std::max(t.getTicks() - w * TimePos::ticksPerBar() / m_ppb / 2, 0));
	}
	m_scrollBack = false;
}



void PianoRoll::updatePosition(const TimePos & t)
{
	if ((Engine::getSong()->isPlaying()
			&& Engine::getSong()->playMode() == Song::PlayMode::MidiClip
			&& m_timeLine->autoScroll() != TimeLineWidget::AutoScrollState::Disabled
		) || m_scrollBack)
	{
		autoScroll(t);
	}
	// ticks relative to m_currentPosition
	// < 0 = outside viewport left
	// > width = outside viewport right
	const int pos = (static_cast<int>(m_timeLine->pos()) - m_currentPosition) * m_ppb / TimePos::ticksPerBar();
	// if pos is within visible range, show it
	if (pos >= 0 && pos <= width() - m_whiteKeyWidth)
	{
		m_positionLine->show();
		// adjust pos for piano keys width and self line width (align to rightmost of line)
		m_positionLine->move(pos + m_whiteKeyWidth - (m_positionLine->width() - 1), keyAreaTop());
	}
	else
	{
		m_positionLine->hide();
	}
}


void PianoRoll::updatePositionLineHeight()
{
	m_positionLine->setFixedHeight(keyAreaBottom() - keyAreaTop());
}




void PianoRoll::updatePositionAccompany( const TimePos & t )
{
	Song * s = Engine::getSong();

	if( m_recording && hasValidMidiClip() &&
					s->playMode() != Song::PlayMode::MidiClip )
	{
		TimePos pos = t;
		if (s->playMode() != Song::PlayMode::Pattern)
		{
			pos -= m_midiClip->startPosition();
		}
		if( (int) pos > 0 )
		{
			s->getPlayPos( Song::PlayMode::MidiClip ).setTicks( pos );
			autoScroll( pos );
		}
	}
}


void PianoRoll::updatePositionStepRecording( const TimePos & t )
{
	if( m_stepRecorder.isRecording() )
	{
		autoScroll( t );
	}
}


void PianoRoll::zoomingChanged()
{
	m_ppb = m_zoomLevels[m_zoomingModel.value()] * DEFAULT_PR_PPB;

	assert( m_ppb > 0 );

	m_timeLine->setPixelsPerBar( m_ppb );
	m_stepRecorderWidget.setPixelsPerBar( m_ppb );
	m_positionLine->zoomChange(m_zoomLevels[m_zoomingModel.value()]);

	update();
}


void PianoRoll::zoomingYChanged()
{
	m_keyLineHeight = m_zoomYLevels[m_zoomingYModel.value()] * DEFAULT_KEY_LINE_HEIGHT;
	m_whiteKeySmallHeight = qFloor(m_keyLineHeight * 1.5);
	m_whiteKeyBigHeight = m_keyLineHeight * 2;
	m_blackKeyHeight = m_keyLineHeight; //round(m_keyLineHeight * 1.3333);

	updateYScroll();
	update();
}


void PianoRoll::quantizeChanged()
{
	update();
}

void PianoRoll::noteLengthChanged()
{
	m_stepRecorder.setStepsLength(newNoteLen());
	update();
}

void PianoRoll::keyChanged()
{
	markSemiTone(SemiToneMarkerAction::MarkCurrentScale, false);
}

int PianoRoll::quantization() const
{
	if( m_quantizeModel.value() == 0 )
	{
		if( m_noteLenModel.value() > 0 )
		{
			return newNoteLen();
		}
		else
		{
			return DefaultTicksPerBar / 16;
		}
	}

	return DefaultTicksPerBar / Quantizations[m_quantizeModel.value() - 1];
}


void PianoRoll::quantizeNotes(QuantizeAction mode)
{
	if( ! hasValidMidiClip() )
	{
		return;
	}

	m_midiClip->addJournalCheckPoint();

	NoteVector notes = getSelectedNotes();

	if( notes.empty() )
	{
		for( Note* n : m_midiClip->notes() )
		{
			notes.push_back( n );
		}
	}

	for( Note* n : notes )
	{
		if( n->length() == TimePos( 0 ) )
		{
			continue;
		}

		Note copy(*n);
		m_midiClip->removeNote( n );
		if (mode == QuantizeAction::Both || mode == QuantizeAction::Pos)
		{
			copy.quantizePos(quantization());
		}
		if (mode == QuantizeAction::Both || mode == QuantizeAction::Length)
		{
			copy.quantizeLength(quantization());
		}
		m_midiClip->addNote(copy, false);
	}

	update();
	getGUI()->songEditor()->update();
	Engine::getSong()->setModified();
}




void PianoRoll::updateSemiToneMarkerMenu()
{
	const InstrumentFunctionNoteStacking::ChordTable& chord_table =
			InstrumentFunctionNoteStacking::ChordTable::getInstance();
	const InstrumentFunctionNoteStacking::Chord& scale =
			chord_table.getScaleByName( m_scaleModel.currentText() );
	const InstrumentFunctionNoteStacking::Chord& chord =
			chord_table.getChordByName( m_chordModel.currentText() );

	emit semiToneMarkerMenuScaleSetEnabled( ! scale.isEmpty() );
	emit semiToneMarkerMenuChordSetEnabled( ! chord.isEmpty() );
}




TimePos PianoRoll::newNoteLen() const
{
	if( m_noteLenModel.value() == 0 )
	{
		return m_lenOfNewNotes;
	}

	QString text = m_noteLenModel.currentText();
	return DefaultTicksPerBar / text.right( text.length() - 2 ).toInt();
}




bool PianoRoll::mouseOverNote()
{
	return hasValidMidiClip() && noteUnderMouse() != nullptr;
}




Note * PianoRoll::noteUnderMouse()
{
	QPoint pos = mapFromGlobal( QCursor::pos() );

	if (pos.x() <= m_whiteKeyWidth
		|| pos.x() > width() - SCROLLBAR_SIZE
		|| pos.y() < PR_TOP_MARGIN
		|| pos.y() > keyAreaBottom() )
	{
		return nullptr;
	}

	int key_num = getKey( pos.y() );
	int pos_ticks = (pos.x() - m_whiteKeyWidth) *
			TimePos::ticksPerBar() / m_ppb + m_currentPosition;

	// loop through whole note-vector...
	for( Note* const& note : m_midiClip->notes() )
	{
		// and check whether the cursor is over an
		// existing note
		if( pos_ticks >= note->pos()
				&& pos_ticks <= note->endPos()
				&& note->key() == key_num
				&& note->length() > 0 )
		{
			return note;
		}
	}

	return nullptr;
}

void PianoRoll::changeSnapMode()
{
	//	GridMode::Nudge,
	//	GridMode::Snap,
	//	GridMode::Free - to be implemented

	m_gridMode = static_cast<GridMode>(m_snapModel.value());
}

PianoRollWindow::PianoRollWindow() :
	Editor(true, true),
	m_editor(new PianoRoll())
{
	setCentralWidget( m_editor );

	m_playAction->setToolTip(tr( "Play/pause current clip (Space)" ) );
	m_recordAction->setToolTip(tr( "Record notes from MIDI-device/channel-piano" ) );
	m_recordAccompanyAction->setToolTip( tr( "Record notes from MIDI-device/channel-piano while playing song or pattern track" ) );
	m_toggleStepRecordingAction->setToolTip( tr( "Record notes from MIDI-device/channel-piano, one step at the time" ) );
	m_stopAction->setToolTip( tr( "Stop playing of current clip (Space)" ) );

	DropToolBar *notesActionsToolBar = addDropToolBarToTop( tr( "Edit actions" ) );

	// init edit-buttons at the top
	auto editModeGroup = new ActionGroup(this);
	QAction* drawAction = editModeGroup->addAction( embed::getIconPixmap( "edit_draw" ), tr( "Draw mode (Shift+D)" ) );
	QAction* eraseAction = editModeGroup->addAction( embed::getIconPixmap( "edit_erase" ), tr("Erase mode (Shift+E)" ) );
	QAction* selectAction = editModeGroup->addAction( embed::getIconPixmap( "edit_select" ), tr( "Select mode (Shift+S)" ) );
	QAction* pitchBendAction = editModeGroup->addAction( embed::getIconPixmap( "automation" ), tr("Pitch Bend mode (Shift+T)" ) );

	drawAction->setChecked( true );

	drawAction->setShortcut(combine(Qt::SHIFT, Qt::Key_D));
	eraseAction->setShortcut(combine(Qt::SHIFT, Qt::Key_E));
	selectAction->setShortcut(combine(Qt::SHIFT, Qt::Key_S));
	pitchBendAction->setShortcut(combine(Qt::SHIFT, Qt::Key_T));

	connect( editModeGroup, SIGNAL(triggered(int)), m_editor, SLOT(setEditMode(int)));

	// Quantize combo button
	auto quantizeButton = new QToolButton(notesActionsToolBar);
	auto quantizeButtonMenu = new QMenu(quantizeButton);

	auto quantizeAction = new QAction(embed::getIconPixmap("quantize"), tr("Quantize"), this);
	auto quantizePosAction = new QAction(tr("Quantize positions"), this);
	auto quantizeLengthAction = new QAction(tr("Quantize lengths"), this);

	connect(quantizeAction, &QAction::triggered, [this](){ m_editor->quantizeNotes(); });
	connect(quantizePosAction, &QAction::triggered, [this](){ m_editor->quantizeNotes(PianoRoll::QuantizeAction::Pos); });
	connect(quantizeLengthAction, &QAction::triggered, [this](){ m_editor->quantizeNotes(PianoRoll::QuantizeAction::Length); });

	quantizeButton->setPopupMode(QToolButton::MenuButtonPopup);
	quantizeButton->setDefaultAction(quantizeAction);
	quantizeButton->setMenu(quantizeButtonMenu);
	quantizeButtonMenu->addAction(quantizePosAction);
	quantizeButtonMenu->addAction(quantizeLengthAction);

	notesActionsToolBar->addAction( drawAction );
	notesActionsToolBar->addAction( eraseAction );
	notesActionsToolBar->addAction( selectAction );
	notesActionsToolBar->addAction( pitchBendAction );
	notesActionsToolBar->addSeparator();
	notesActionsToolBar->addWidget(quantizeButton);

	// -- File actions
	DropToolBar* fileActionsToolBar = addDropToolBarToTop(tr("File actions"));

	// -- File ToolButton
	m_fileToolsButton = new QToolButton(m_toolBar);
	m_fileToolsButton->setIcon(embed::getIconPixmap("file"));
	m_fileToolsButton->setPopupMode(QToolButton::InstantPopup);

	// Import / export
	auto importAction = new QAction(embed::getIconPixmap("project_import"), tr("Import clip"), m_fileToolsButton);

	auto exportAction = new QAction(embed::getIconPixmap("project_export"), tr("Export clip"), m_fileToolsButton);

	m_fileToolsButton->addAction(importAction);
	m_fileToolsButton->addAction(exportAction);
	fileActionsToolBar->addWidget(m_fileToolsButton);

	connect(importAction, SIGNAL(triggered()), this, SLOT(importMidiClip()));
	connect(exportAction, SIGNAL(triggered()), this, SLOT(exportMidiClip()));
	// -- End File actions

	// Copy + paste actions
	DropToolBar *copyPasteActionsToolBar =  addDropToolBarToTop( tr( "Copy paste controls" ) );

	auto cutAction = new QAction(embed::getIconPixmap("edit_cut"), tr("Cut (%1+X)").arg(UI_CTRL_KEY), this);

	auto copyAction = new QAction(embed::getIconPixmap("edit_copy"), tr("Copy (%1+C)").arg(UI_CTRL_KEY), this);

	auto pasteAction = new QAction(embed::getIconPixmap("edit_paste"), tr("Paste (%1+V)").arg(UI_CTRL_KEY), this);

	cutAction->setShortcut(combine(Qt::CTRL, Qt::Key_X));
	copyAction->setShortcut(combine(Qt::CTRL, Qt::Key_C));
	pasteAction->setShortcut(combine(Qt::CTRL, Qt::Key_V));

	connect( cutAction, SIGNAL(triggered()), m_editor, SLOT(cutSelectedNotes()));
	connect( copyAction, SIGNAL(triggered()), m_editor, SLOT(copySelectedNotes()));
	connect( pasteAction, SIGNAL(triggered()), m_editor, SLOT(pasteNotes()));

	copyPasteActionsToolBar->addAction( cutAction );
	copyPasteActionsToolBar->addAction( copyAction );
	copyPasteActionsToolBar->addAction( pasteAction );


	DropToolBar *timeLineToolBar = addDropToolBarToTop( tr( "Timeline controls" ) );
	m_editor->m_timeLine->addToolButtons( timeLineToolBar );

	// -- Note modifier tools
	auto noteToolsButton = new QToolButton(m_toolBar);
	noteToolsButton->setIcon(embed::getIconPixmap("tool"));
	noteToolsButton->setPopupMode(QToolButton::InstantPopup);

	auto glueAction = new QAction(embed::getIconPixmap("glue"), tr("Glue"), noteToolsButton);
	connect(glueAction, SIGNAL(triggered()), m_editor, SLOT(glueNotes()));
	glueAction->setShortcut(combine(Qt::SHIFT, Qt::Key_G));

	auto knifeAction = new QAction(embed::getIconPixmap("edit_knife"), tr("Knife"), noteToolsButton);
	connect(knifeAction, &QAction::triggered, m_editor, &PianoRoll::setKnifeAction);
	knifeAction->setShortcut(combine(Qt::SHIFT, Qt::Key_K));

	auto strumAction = new QAction(embed::getIconPixmap("arp_free"), tr("Strum"), noteToolsButton);
	connect(strumAction, &QAction::triggered, m_editor, &PianoRoll::setStrumAction);
	strumAction->setShortcut(combine(Qt::SHIFT, Qt::Key_J));

	auto fillAction = new QAction(embed::getIconPixmap("fill"), tr("Fill"), noteToolsButton);
	connect(fillAction, &QAction::triggered, [this](){ m_editor->fitNoteLengths(true); });
	fillAction->setShortcut(combine(Qt::SHIFT, Qt::Key_F));

	auto cutOverlapsAction = new QAction(embed::getIconPixmap("cut_overlaps"), tr("Cut overlaps"), noteToolsButton);
	connect(cutOverlapsAction, &QAction::triggered, [this](){ m_editor->fitNoteLengths(false); });
	cutOverlapsAction->setShortcut(combine(Qt::SHIFT, Qt::Key_C));

	auto minLengthAction = new QAction(embed::getIconPixmap("min_length"), tr("Min length as last"), noteToolsButton);
	connect(minLengthAction, &QAction::triggered, [this](){ m_editor->constrainNoteLengths(false); });

	auto maxLengthAction = new QAction(embed::getIconPixmap("max_length"), tr("Max length as last"), noteToolsButton);
	connect(maxLengthAction, &QAction::triggered, [this](){ m_editor->constrainNoteLengths(true); });

	auto reverseAction = new QAction(embed::getIconPixmap("flip_x"), tr("Reverse Notes"), noteToolsButton);
	connect(reverseAction, &QAction::triggered, [this](){ m_editor->reverseNotes(); });
	reverseAction->setShortcut(combine(Qt::SHIFT, Qt::Key_R));

	noteToolsButton->addAction(glueAction);
	noteToolsButton->addAction(knifeAction);
	noteToolsButton->addAction(strumAction);
	noteToolsButton->addAction(fillAction);
	noteToolsButton->addAction(cutOverlapsAction);
	noteToolsButton->addAction(minLengthAction);
	noteToolsButton->addAction(maxLengthAction);
	noteToolsButton->addAction(reverseAction);

	notesActionsToolBar->addWidget(noteToolsButton);

	addToolBarBreak();


	DropToolBar *zoomAndNotesToolBar = addDropToolBarToTop( tr( "Zoom and note controls" ) );

	auto zoom_lbl = new QLabel(m_toolBar);
	zoom_lbl->setPixmap( embed::getIconPixmap( "zoom_x" ) );

	m_zoomingComboBox = new ComboBox( m_toolBar );
	m_zoomingComboBox->setModel( &m_editor->m_zoomingModel );
	m_zoomingComboBox->setFixedSize( 64, ComboBox::DEFAULT_HEIGHT );
	m_zoomingComboBox->setToolTip( tr( "Horizontal zooming") );

	auto zoom_y_lbl = new QLabel(m_toolBar);
	zoom_y_lbl->setPixmap(embed::getIconPixmap("zoom_y"));

	m_zoomingYComboBox = new ComboBox(m_toolBar);
	m_zoomingYComboBox->setModel(&m_editor->m_zoomingYModel);
	m_zoomingYComboBox->setFixedSize(64, ComboBox::DEFAULT_HEIGHT);
	m_zoomingYComboBox->setToolTip(tr("Vertical zooming"));

	// setup quantize-stuff
	auto quantize_lbl = new QLabel(m_toolBar);
	quantize_lbl->setPixmap( embed::getIconPixmap( "quantize" ) );

	m_quantizeComboBox = new ComboBox( m_toolBar );
	m_quantizeComboBox->setModel( &m_editor->m_quantizeModel );
	m_quantizeComboBox->setFixedSize(85, ComboBox::DEFAULT_HEIGHT);
	m_quantizeComboBox->setToolTip( tr( "Quantization") );

	// setup note-len-stuff
	auto note_len_lbl = new QLabel(m_toolBar);
	note_len_lbl->setPixmap( embed::getIconPixmap( "note" ) );

	m_noteLenComboBox = new ComboBox( m_toolBar );
	m_noteLenComboBox->setModel( &m_editor->m_noteLenModel );
	m_noteLenComboBox->setFixedSize( 105, ComboBox::DEFAULT_HEIGHT );
	m_noteLenComboBox->setToolTip( tr( "Note length") );

	// setup key-stuff
	m_keyComboBox = new ComboBox(m_toolBar);
	m_keyComboBox->setModel(&m_editor->m_keyModel);
	m_keyComboBox->setFixedSize(72, ComboBox::DEFAULT_HEIGHT);
	m_keyComboBox->setToolTip(tr("Key"));

	// setup scale-stuff
	auto scale_lbl = new QLabel(m_toolBar);
	scale_lbl->setPixmap( embed::getIconPixmap( "scale" ) );

	m_scaleComboBox = new ComboBox( m_toolBar );
	m_scaleComboBox->setModel( &m_editor->m_scaleModel );
	m_scaleComboBox->setFixedSize(155, ComboBox::DEFAULT_HEIGHT);
	m_scaleComboBox->setToolTip( tr( "Scale") );

	// setup chord-stuff
	auto chord_lbl = new QLabel(m_toolBar);
	chord_lbl->setPixmap( embed::getIconPixmap( "chord" ) );

	m_chordComboBox = new ComboBox( m_toolBar );
	m_chordComboBox->setModel( &m_editor->m_chordModel );
	m_chordComboBox->setFixedSize(125, ComboBox::DEFAULT_HEIGHT);
	m_chordComboBox->setToolTip( tr( "Chord" ) );

	// setup snap-stuff
	auto snapLbl = new QLabel(m_toolBar);
	snapLbl->setPixmap(embed::getIconPixmap("gridmode"));

	m_snapComboBox = new ComboBox(m_toolBar);
	m_snapComboBox->setModel(&m_editor->m_snapModel);
	m_snapComboBox->setFixedSize(96, ComboBox::DEFAULT_HEIGHT);
	m_snapComboBox->setToolTip(tr("Snap mode"));

	// -- Clear ghost MIDI clip button
	m_clearGhostButton = new QPushButton( m_toolBar );
	m_clearGhostButton->setIcon( embed::getIconPixmap( "clear_ghost_note" ) );
	m_clearGhostButton->setToolTip( tr( "Clear ghost notes" ) );
	m_clearGhostButton->setEnabled( false );
	connect( m_clearGhostButton, SIGNAL(clicked()), m_editor, SLOT(clearGhostClip()));
	connect( m_editor, SIGNAL(ghostClipSet(bool)), this, SLOT(ghostClipSet(bool)));

	// Wrap label icons and comboboxes in a single widget so when
	// the window is resized smaller in width it hides both
	auto zoom_widget = new QWidget();
	auto zoom_hbox = new QHBoxLayout();
	zoom_hbox->setContentsMargins(0, 0, 0, 0);
	zoom_hbox->addWidget(zoom_lbl);
	zoom_hbox->addWidget(m_zoomingComboBox);
	zoom_widget->setLayout(zoom_hbox);
	zoomAndNotesToolBar->addWidget(zoom_widget);

	auto zoomY_widget = new QWidget();
	auto zoomY_hbox = new QHBoxLayout();
	zoomY_hbox->setContentsMargins(0, 0, 0, 0);
	zoomY_hbox->addWidget(zoom_y_lbl);
	zoomY_hbox->addWidget(m_zoomingYComboBox);
	zoomY_widget->setLayout(zoomY_hbox);
	zoomAndNotesToolBar->addWidget(zoomY_widget);

	auto quantize_widget = new QWidget();
	auto quantize_hbox = new QHBoxLayout();
	quantize_hbox->setContentsMargins(0, 0, 0, 0);
	quantize_hbox->addWidget(quantize_lbl);
	quantize_hbox->addWidget(m_quantizeComboBox);
	quantize_widget->setLayout(quantize_hbox);
	zoomAndNotesToolBar->addSeparator();
	zoomAndNotesToolBar->addWidget(quantize_widget);

	auto note_widget = new QWidget();
	auto note_hbox = new QHBoxLayout();
	note_hbox->setContentsMargins(0, 0, 0, 0);
	note_hbox->addWidget(note_len_lbl);
	note_hbox->addWidget(m_noteLenComboBox);
	note_widget->setLayout(note_hbox);
	zoomAndNotesToolBar->addSeparator();
	zoomAndNotesToolBar->addWidget(note_widget);

	auto scale_widget = new QWidget();
	auto scale_hbox = new QHBoxLayout();
	scale_hbox->setContentsMargins(0, 0, 0, 0);
	scale_hbox->addWidget(scale_lbl);
	// Add the key selection between scale label and key
	scale_hbox->addWidget(m_keyComboBox);
	scale_hbox->addWidget(m_scaleComboBox);
	scale_widget->setLayout(scale_hbox);
	zoomAndNotesToolBar->addSeparator();
	zoomAndNotesToolBar->addWidget(scale_widget);

	auto chord_widget = new QWidget();
	auto chord_hbox = new QHBoxLayout();
	chord_hbox->setContentsMargins(0, 0, 0, 0);
	chord_hbox->addWidget(chord_lbl);
	chord_hbox->addWidget(m_chordComboBox);
	chord_widget->setLayout(chord_hbox);
	zoomAndNotesToolBar->addSeparator();
	zoomAndNotesToolBar->addWidget(chord_widget);

	zoomAndNotesToolBar->addSeparator();
	zoomAndNotesToolBar->addWidget( m_clearGhostButton );

	auto snapWidget = new QWidget();
	auto snapHbox = new QHBoxLayout();
	snapHbox->setContentsMargins(0, 0, 0, 0);
	snapHbox->addWidget(snapLbl);
	snapHbox->addWidget(m_snapComboBox);
	snapWidget->setLayout(snapHbox);
	zoomAndNotesToolBar->addSeparator();
	zoomAndNotesToolBar->addWidget(snapWidget);

	// setup our actual window
	setFocusPolicy( Qt::StrongFocus );
	setFocus();
	setWindowIcon( embed::getIconPixmap( "piano" ) );
	setCurrentMidiClip( nullptr );

	// Connections
	connect( m_editor, SIGNAL(currentMidiClipChanged()), this, SIGNAL(currentMidiClipChanged()));
	connect( m_editor, SIGNAL(currentMidiClipChanged()), this, SLOT(updateAfterMidiClipChange()));
}




const MidiClip* PianoRollWindow::currentMidiClip() const
{
	return m_editor->currentMidiClip();
}




void PianoRollWindow::setGhostMidiClip( MidiClip* clip )
{
	m_editor->setGhostMidiClip( clip );
}




void PianoRollWindow::setCurrentMidiClip( MidiClip* clip )
{
	m_editor->setCurrentMidiClip( clip );

	if ( clip )
	{
		setWindowTitle( tr( "Piano-Roll - %1" ).arg( clip->name() ) );
		m_fileToolsButton->setEnabled(true);
		connect( clip->instrumentTrack(), SIGNAL(nameChanged()), this, SLOT(updateAfterMidiClipChange()));
		connect( clip, SIGNAL(dataChanged()), this, SLOT(updateAfterMidiClipChange()));
	}
	else
	{
		setWindowTitle( tr( "Piano-Roll - no clip" ) );
		m_fileToolsButton->setEnabled(false);
	}
}




bool PianoRollWindow::isRecording() const
{
	return m_editor->isRecording();
}




int PianoRollWindow::quantization() const
{
	return m_editor->quantization();
}




void PianoRollWindow::play()
{
	m_editor->play();
}




void PianoRollWindow::stop()
{
	m_editor->stop();
}




void PianoRollWindow::record()
{
	stopStepRecording(); //step recording mode is mutually exclusive with other record modes

	m_editor->record();
}




void PianoRollWindow::recordAccompany()
{
	stopStepRecording(); //step recording mode is mutually exclusive with other record modes

	m_editor->recordAccompany();
}


void PianoRollWindow::toggleStepRecording()
{
	if(isRecording())
	{
		// step recording mode is mutually exclusive with other record modes
		// stop them before starting step recording
		stop();
	}

	m_editor->toggleStepRecording();

	updateStepRecordingIcon();
}

void PianoRollWindow::stopRecording()
{
	m_editor->stopRecording();
}




void PianoRollWindow::reset()
{
	m_editor->reset();
}




void PianoRollWindow::saveSettings( QDomDocument & doc, QDomElement & de )
{
	if( !m_editor->ghostNotes().empty() )
	{
		QDomElement ghostNotesRoot = doc.createElement( "ghostnotes" );
		for( Note *note : m_editor->ghostNotes() )
		{
			QDomElement ghostNoteNode = doc.createElement( "ghostnote" );
			ghostNoteNode.setAttribute( "len", note->length() );
			ghostNoteNode.setAttribute( "key", note->key() );
			ghostNoteNode.setAttribute( "pos", note->pos() );

			ghostNotesRoot.appendChild(ghostNoteNode);
		}
		de.appendChild( ghostNotesRoot );
	}

	if (m_editor->m_markedSemiTones.length() > 0)
	{
		QDomElement markedSemiTonesRoot = doc.createElement("markedSemiTones");
		for (int markedSemiTone : m_editor->m_markedSemiTones)
		{
			QDomElement semiToneNode = doc.createElement("semiTone");
			semiToneNode.setAttribute("key", markedSemiTone);
			markedSemiTonesRoot.appendChild(semiToneNode);
		}
		de.appendChild(markedSemiTonesRoot);
	}

	de.setAttribute("stopbehaviour", static_cast<int>(
		Engine::getSong()->getTimeline(Song::PlayMode::MidiClip).stopBehaviour()));

	MainWindow::saveWidgetState( this, de );
}




void PianoRollWindow::loadSettings( const QDomElement & de )
{
	m_editor->loadGhostNotes( de.firstChildElement("ghostnotes") );
	m_editor->loadMarkedSemiTones(de.firstChildElement("markedSemiTones"));

	MainWindow::restoreWidgetState( this, de );

	Engine::getSong()->getTimeline(Song::PlayMode::MidiClip).setStopBehaviour(
		static_cast<Timeline::StopBehaviour>(de.attribute("stopbehaviour").toInt()));

	// update margins here because we're later in the startup process
	// We can't earlier because everything is still starting with the
	// WHITE_KEY_WIDTH default
	QMargins qm = m_editor->m_stepRecorderWidget.margins();
	qm.setLeft(m_editor->m_whiteKeyWidth);
	m_editor->m_stepRecorderWidget.setMargins(qm);
	m_editor->m_timeLine->setXOffset(m_editor->m_whiteKeyWidth);
}




QSize PianoRollWindow::sizeHint() const
{
	return { INITIAL_PIANOROLL_WIDTH, INITIAL_PIANOROLL_HEIGHT };
}



bool PianoRollWindow::hasFocus() const
{
	return m_editor->hasFocus();
}



void PianoRollWindow::updateAfterMidiClipChange()
{
	clipRenamed();
	updateStepRecordingIcon(); //MIDI clip change turn step recording OFF - update icon accordingly
}

void PianoRollWindow::clipRenamed()
{
	if ( currentMidiClip() )
	{
		setWindowTitle( tr( "Piano-Roll - %1" ).arg( currentMidiClip()->name() ) );
		m_fileToolsButton->setEnabled(true);
	}
	else
	{
		setWindowTitle( tr( "Piano-Roll - no clip" ) );
		m_fileToolsButton->setEnabled(false);
	}
}




void PianoRollWindow::ghostClipSet( bool state )
{
	m_clearGhostButton->setEnabled( state );
}




void PianoRollWindow::exportMidiClip()
{
	FileDialog exportDialog(this, tr("Export clip"), "",
		tr("XML clip file (*.xpt *.xptz)"));

	auto layout = dynamic_cast<QGridLayout*>(exportDialog.layout());
	QCheckBox* onlySelectedNotesCheckBox = nullptr;
	if (layout)
	{
		onlySelectedNotesCheckBox = new QCheckBox(tr("Export only selected notes"), &exportDialog);
		layout->addWidget(onlySelectedNotesCheckBox);
	}

	exportDialog.setAcceptMode(FileDialog::AcceptSave);

	if (exportDialog.exec() == QDialog::Accepted &&
		!exportDialog.selectedFiles().isEmpty() &&
		!exportDialog.selectedFiles().first().isEmpty())
	{
		QString suffix =
			ConfigManager::inst()->value("app", "nommpz").toInt() == 0
				? "xptz"
				: "xpt";
		exportDialog.setDefaultSuffix(suffix);

		const QString fullPath = exportDialog.selectedFiles()[0];

		// Check if only the selected notes should be exported
		auto* midiClip = m_editor->m_midiClip;

		const bool onlySelectedNotes = onlySelectedNotesCheckBox && onlySelectedNotesCheckBox->isChecked();

		DataFile dataFile(DataFile::Type::MidiClip);
		midiClip->exportToXML(dataFile, dataFile.content(), onlySelectedNotes);

		if (dataFile.writeFile(fullPath))
		{
			TextFloat::displayMessage(tr("Export clip success"),
				tr("Clip saved to %1").arg(fullPath),
				embed::getIconPixmap("project_export"), 4000);
		}
	}
}




void PianoRollWindow::importMidiClip()
{
	// Overwrite confirmation.
	if (!m_editor->m_midiClip->empty() &&
		QMessageBox::warning(
			nullptr,
			tr("Import clip."),
			tr("You are about to import a clip, this will "
				"overwrite your current clip. Do you want to "
				"continue?"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
		) != QMessageBox::Yes)
	{
		return;
	}

	FileDialog importDialog(this, tr("Open clip"), "",
		tr("XML clip file (*.xpt *.xptz)"));
	importDialog.setFileMode(FileDialog::ExistingFile);

	if (importDialog.exec() == QDialog::Accepted &&
		!importDialog.selectedFiles().isEmpty())
	{
		const QString fullPath = importDialog.selectedFiles()[0];
		DataFile dataFile(fullPath);

		if (dataFile.head().isNull())
		{
			return;
		}

		TimePos pos = m_editor->m_midiClip->startPosition(); // Backup position in timeline.

		m_editor->m_midiClip->loadSettings(dataFile.content());
		m_editor->m_midiClip->movePosition(pos);

		TextFloat::displayMessage(tr("Import clip success"),
			tr("Imported clip %1!").arg(fullPath),
			embed::getIconPixmap("project_import"), 4000);
	}
}




void PianoRollWindow::focusInEvent( QFocusEvent * event )
{
	// when the window is given focus, also give focus to the actual piano roll
	m_editor->setFocus( event->reason() );
}

void PianoRollWindow::stopStepRecording()
{
	if(m_editor->isStepRecording())
	{
		m_editor->toggleStepRecording();
		updateStepRecordingIcon();
	}
}

void PianoRollWindow::updateStepRecordingIcon()
{
	if(m_editor->isStepRecording())
	{
		m_toggleStepRecordingAction->setIcon(embed::getIconPixmap("record_step_on"));
	}
	else
	{
		m_toggleStepRecordingAction->setIcon(embed::getIconPixmap("record_step_off"));
	}
}


} // namespace gui

} // namespace lmms
