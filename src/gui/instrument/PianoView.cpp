/*
 * PianoView.cpp - implementation of piano-widget used in instrument-track-window
 *             for testing + according model class
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

/** \file piano.cpp
 *  \brief A piano keyboard to play notes on in the instrument plugin window.
 */

/*
 * \mainpage Instrument plugin keyboard display classes
 *
 * \section introduction Introduction
 *
 * \todo fill this out
 * \todo write isWhite inline function and replace throughout
 */

#include <cmath>

#include <QCursor>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>  // IWYU pragma: keep
#include <QVBoxLayout>

#include "AutomatableModelView.h"
#include "PianoView.h"
#include "Piano.h"
#include "CaptionMenu.h"
#include "Engine.h"
#include "FontHelper.h"
#include "InstrumentTrack.h"
#include "Song.h"
#include "StringPairDrag.h"


namespace lmms::gui
{


/*! The scale of C Major - white keys only.
 */
auto WhiteKeys = std::array
{
	Key::C, Key::D, Key::E, Key::F, Key::G, Key::A, Key::H
} ;

const int PIANO_BASE = 11;          /*!< The height of the root note display */
const int PW_WHITE_KEY_WIDTH = 10;  /*!< The width of a white key */
const int PW_BLACK_KEY_WIDTH = 8;   /*!< The width of a black key */
const int PW_WHITE_KEY_HEIGHT = 57; /*!< The height of a white key */
const int PW_BLACK_KEY_HEIGHT = 38; /*!< The height of a black key */
const int LABEL_TEXT_SIZE = 8;      /*!< The height of the key label text */




/*! \brief Create a new keyboard display view
 *
 *  \param _parent the parent instrument plugin window
 *  \todo are the descriptions of the m_startkey and m_lastkey properties correct?
 */
PianoView::PianoView(QWidget *parent) :
	QWidget(parent),                 /*!< Our parent */
	ModelView(nullptr, this),        /*!< Our view Model */
	m_piano(nullptr),                /*!< Our piano Model */
	m_startKey(Octave::Octave_3 + Key::C), /*!< The first key displayed? */
	m_lastKey(-1),                   /*!< The last key displayed? */
	m_movedNoteModel(nullptr)        /*!< Key marker which is being moved */
{
	setFocusPolicy(Qt::StrongFocus);

	// Black keys are drawn halfway between successive white keys, so they do not
	// contribute to the total width. Half of a black key is added in case the last
	// octave is incomplete and ends with a black key. Drawing always starts at
	// a white key, so no similar modification is needed at the beginning.
	setMaximumWidth(Piano::NumWhiteKeys * PW_WHITE_KEY_WIDTH +
		(Piano::isBlackKey(NumKeys-1) ? PW_BLACK_KEY_WIDTH / 2 : 0));

	// create scrollbar at the bottom
	m_pianoScroll = new QScrollBar( Qt::Horizontal, this );
	m_pianoScroll->setSingleStep( 1 );
	m_pianoScroll->setPageStep( 20 );
	m_pianoScroll->setValue(static_cast<int>(Octave::Octave_3) * Piano::WhiteKeysPerOctave);

	// and connect it to this widget
	connect( m_pianoScroll, SIGNAL(valueChanged(int)),
			this, SLOT(pianoScrolled(int)));

	// create a layout for ourselves
	auto layout = new QVBoxLayout(this);
	layout->setSpacing( 0 );
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addSpacing( PIANO_BASE+PW_WHITE_KEY_HEIGHT );
	layout->addWidget( m_pianoScroll );

	// trigger a redraw if keymap definitions change (different keys may become disabled)
	connect(Engine::getSong(), SIGNAL(keymapListChanged(int)), this, SLOT(update()));
}

static int getKeyOffsetFromKeyEvent( QKeyEvent * _ke )
{
	// TODO: check the scan codes for ',' = c, 'L' = c#, '.' = d, ':' = d#,
	// '/' = d, '[' = f', '=' = f'#, ']' = g' - Paul's additions
#ifdef LMMS_BUILD_APPLE
	const int k = _ke->nativeVirtualKey();
#else
	const int k = _ke->nativeScanCode();
#endif

#ifdef LMMS_BUILD_WIN32
	switch( k )
	{
		case 44: return 0; // Z  = C
		case 31: return 1; // S  = C#
		case 45: return 2; // X  = D
		case 32: return 3; // D  = D#
		case 46: return 4; // C  = E
		case 47: return 5; // V  = F
		case 34: return 6; // G  = F#
		case 48: return 7; // B  = G
		case 35: return 8; // H  = G#
		case 49: return 9; // N  = A
		case 36: return 10; // J = A#
		case 50: return 11; // M = B
		case 51: return 12; // , = c
		case 38: return 13; // L = c#
		case 52: return 14; // . = d
		case 39: return 15; // ; = d#
		//case 86: return 16; // / = e
		case 53: return 16; // / = e
		case 16: return 12; // Q = c
		case 3: return 13; // 2 = c#
		case 17: return 14; // W = d
		case 4: return 15; // 3 = d#
		case 18: return 16; // E = e
		case 19: return 17; // R = f
		case 6: return 18; // 5 = f#
		case 20: return 19; // T = g
		case 7: return 20; // 6 = g#
		case 21: return 21; // Y = a
		case 8: return 22; // 7 = a#
		case 22: return 23; // U = b
		case 23: return 24; // I = c'
		case 10: return 25; // 9 = c'#
		case 24: return 26; // O = d'
		case 11: return 27; // 0 = d'#
		case 25: return 28; // P = e'
		case 26: return 29; // [
		case 13: return 30; // =
		case 27: return 31; // ]
	}
#endif // LMMS_BUILD_WIN32
#if defined(LMMS_BUILD_LINUX) || defined(LMMS_BUILD_OPENBSD) || defined(LMMS_BUILD_FREEBSD)
	switch( k )
	{
		case 52: return 0; // Z  = C
		case 39: return 1; // S  = C#
		case 53: return 2; // X  = D
		case 40: return 3; // D  = D#
		case 54: return 4; // C  = E
		case 55: return 5; // V  = F
		case 42: return 6; // G  = F#
		case 56: return 7; // B  = G
		case 43: return 8; // H  = G#
		case 57: return 9; // N  = A
		case 44: return 10; // J = A#
		case 58: return 11; // M = B
		case 59: return 12; // , = c
		case 46: return 13; // L = c#
		case 60: return 14; // . = d
		case 47: return 15; // ; = d#
		case 61: return 16; // / = e
		case 24: return 12; // Q = c
		case 11: return 13; // 2 = c#
		case 25: return 14; // W = d
		case 12: return 15; // 3 = d#
		case 26: return 16; // E = e
		case 27: return 17; // R = f
		case 14: return 18; // 5 = f#
		case 28: return 19; // T = g
		case 15: return 20; // 6 = g#
		case 29: return 21; // Y = a
		case 16: return 22; // 7 = a#
		case 30: return 23; // U = b
		case 31: return 24; // I = c'
		case 18: return 25; // 9 = c'#
		case 32: return 26; // O = d'
		case 19: return 27; // 0 = d'#
		case 33: return 28; // P = e'
		case 34: return 29; // [
		case 21: return 30; // =
		case 35: return 31; // ]
	}
#endif
#ifdef LMMS_BUILD_APPLE
	switch( k )
	{
		case 6: return 0; // Z  = C
		case 1: return 1; // S  = C#
		case 7: return 2; // X  = D
		case 2: return 3; // D  = D#
		case 8: return 4; // C  = E
		case 9: return 5; // V  = F
		case 5: return 6; // G  = F#
		case 11: return 7; // B  = G
		case 4: return 8; // H  = G#
		case 45: return 9; // N  = A
		case 38: return 10; // J = A#
		case 46: return 11; // M = B
		case 43: return 12; // , = c
		case 37: return 13; // L = c#
		case 47: return 14; // . = d
		case 41: return 15; // ; = d#
		case 44: return 16; // / = e
		case 12: return 12; // Q = c
		case 19: return 13; // 2 = c#
		case 13: return 14; // W = d
		case 20: return 15; // 3 = d#
		case 14: return 16; // E = e
		case 15: return 17; // R = f
		case 23: return 18; // 5 = f#
		case 17: return 19; // T = g
		case 22: return 20; // 6 = g#
		case 16: return 21; // Y = a
		case 26: return 22; // 7 = a#
		case 32: return 23; // U = b
		case 34: return 24; // I = c'
		case 25: return 25; // 9 = c'#
		case 31: return 26; // O = d'
		case 29: return 27; // 0 = d'#
		case 35: return 28; // P = e'
	}
#endif // LMMS_BUILD_APPLE

	return -100;
}

/*! \brief Map a keyboard key being pressed to a note in our keyboard view
 *
 */
int PianoView::getKeyFromKeyEvent( QKeyEvent * ke )
{
	const auto key = static_cast<Key>(getKeyOffsetFromKeyEvent(ke));
	return DefaultOctave + key - KeysPerOctave;
}

/*! \brief Register a change to this piano display view
 *
 */
void PianoView::modelChanged()
{
	m_piano = castModel<Piano>();
	if (m_piano != nullptr)
	{
		connect(m_piano->instrumentTrack()->baseNoteModel(), SIGNAL(dataChanged()), this, SLOT(update()));
		connect(m_piano->instrumentTrack()->firstKeyModel(), SIGNAL(dataChanged()), this, SLOT(update()));
		connect(m_piano->instrumentTrack()->lastKeyModel(), SIGNAL(dataChanged()), this, SLOT(update()));
		connect(m_piano->instrumentTrack()->microtuner()->enabledModel(), SIGNAL(dataChanged()), this, SLOT(update()));
		connect(m_piano->instrumentTrack()->microtuner()->keymapModel(), SIGNAL(dataChanged()), this, SLOT(update()));
		connect(m_piano->instrumentTrack()->microtuner()->keyRangeImportModel(), SIGNAL(dataChanged()),
				this, SLOT(update()));
	}
}



// Gets the key from the given mouse position
/*! \brief Get the key from the mouse position in the piano display
 *
 *  \param p The point that the mouse was pressed.
 */
int PianoView::getKeyFromMouse(const QPoint& p) const
{
	// The left-most key visible in the piano display is always white
	const int startingWhiteKey = m_pianoScroll->value();

	// Adjust the mouse x position as if x == 0 was the left side of the lowest key
	const int adjX = p.x() + (startingWhiteKey * PW_WHITE_KEY_WIDTH);

	// Can early return for notes too low
	if (adjX <= 0) { return 0; }

	// Now we can calculate the key number (in only white keys) and the octave
	const int whiteKey = adjX / PW_WHITE_KEY_WIDTH;
	const int octave = whiteKey / Piano::WhiteKeysPerOctave;

	// Calculate for full octaves
	int key = octave * KeysPerOctave;

	// Adjust for white notes in the current octave
	// (WhiteKeys maps each white key to the number of notes to their left in the octave)
	key += static_cast<int>(WhiteKeys[whiteKey % Piano::WhiteKeysPerOctave]);

	// Might be a black key, which would require further adjustment
	if (p.y() < PIANO_BASE + PW_BLACK_KEY_HEIGHT)
	{
		// Maps white keys to neighboring black keys
		static constexpr std::array neighboringKeyMap {
			std::pair{ 0, 1 }, // C --> no B#; C#
			std::pair{ 1, 1 }, // D --> C#; D#
			std::pair{ 1, 0 }, // E --> D#; no E#
			std::pair{ 0, 1 }, // F --> no E#; F#
			std::pair{ 1, 1 }, // G --> F#; G#
			std::pair{ 1, 1 }, // A --> G#; A#
			std::pair{ 1, 0 }, // B --> A#; no B#
		};

		const auto neighboringBlackKeys = neighboringKeyMap[whiteKey % Piano::WhiteKeysPerOctave];
		const int offset = adjX - (whiteKey * PW_WHITE_KEY_WIDTH); // mouse X offset from white key

		if (offset < PW_BLACK_KEY_WIDTH / 2)
		{
			// At the location of a (possibly non-existent) black key on the left side
			key -= neighboringBlackKeys.first;
		}
		else if (offset > PW_WHITE_KEY_WIDTH - (PW_BLACK_KEY_WIDTH / 2))
		{
			// At the location of a (possibly non-existent) black key on the right side
			key += neighboringBlackKeys.second;
		}

		// For white keys in between black keys, no further adjustment is needed
	}

	return std::clamp(key, 0, NumKeys - 1);
}




// handler for scrolling-event
/*! \brief Handle the scrolling on the piano display view
 *
 *  We need to update our start key position based on the new position.
 *
 *  \param newPos the new key position, counting only white keys.
 */
void PianoView::pianoScrolled(int newPos)
{
	m_startKey = static_cast<Octave>(newPos / Piano::WhiteKeysPerOctave)
		+ WhiteKeys[newPos % Piano::WhiteKeysPerOctave];

	update();
}




/*! \brief Handle a context menu selection on the piano display view
 *
 *  \param me the ContextMenuEvent to handle.
 *  \todo Is this right, or does this create the context menu?
 */
void PianoView::contextMenuEvent(QContextMenuEvent *me)
{
	if (me->pos().y() > PIANO_BASE || m_piano == nullptr ||
		m_piano->instrumentTrack()->keyRangeImport())
	{
		QWidget::contextMenuEvent(me);
		return;
	}

	// check which control element is closest to the mouse and open the appropriate menu
	QString title;
	IntModel *noteModel = getNearestMarker(getKeyFromMouse(me->pos()), &title);

	CaptionMenu contextMenu(title);
	AutomatableModelView amv(noteModel, &contextMenu);
	amv.addDefaultActions(&contextMenu);
	contextMenu.exec(QCursor::pos());
}




// handler for mouse-click-event
/*! \brief Handle a mouse click on this piano display view
 *
 *  We first determine the key number using the getKeyFromMouse() method.
 *
 *  If we're below the 'root key selection' area,
 *  we set the volume of the note to be proportional to the vertical
 *  position on the keyboard - lower down the key is louder, within the
 *  boundaries of the (white or black) key pressed.  We then tell the
 *  instrument to play that note, scaling for MIDI max loudness = 127.
 *
 *  If we're in the 'root key selection' area, of course, we set the
 *  root key to be that key.
 *
 *  We finally update ourselves to show the key press
 *
 *  \param me the mouse click to handle.
 */
void PianoView::mousePressEvent(QMouseEvent *me)
{
	if (me->button() == Qt::LeftButton && m_piano != nullptr)
	{
		// get pressed key
		int key_num = getKeyFromMouse(me->pos());
		if (me->pos().y() > PIANO_BASE)
		{
			int y_diff = me->pos().y() - PIANO_BASE;
			int velocity = static_cast<int>(
				static_cast<float>(y_diff) / getKeyHeight(key_num) *
				m_piano->instrumentTrack()->midiPort()->baseVelocity());
			if (y_diff < 0)
			{
				velocity = 0;
			}
			else if (y_diff > getKeyHeight(key_num))
			{
				velocity = m_piano->instrumentTrack()->midiPort()->baseVelocity();
			}
			// set note on
			m_piano->midiEventProcessor()->processInEvent(MidiEvent(MidiNoteOn, -1, key_num, velocity));
			m_piano->setKeyState(key_num, true);
			m_lastKey = key_num;

			emit keyPressed(key_num);
		}
		else if (!m_piano->instrumentTrack()->keyRangeImport())
		{
			// upper section, select which marker (base / first / last note) will be moved
			m_movedNoteModel = getNearestMarker(key_num);

			if (me->modifiers() & Qt::ControlModifier)
			{
				new StringPairDrag("automatable_model",	QString::number(m_movedNoteModel->id()), QPixmap(), this);
				me->accept();
			}
			else
			{
				m_movedNoteModel->setInitValue(static_cast<float>(key_num));
				if (m_movedNoteModel == m_piano->instrumentTrack()->baseNoteModel()) { emit baseNoteChanged(); }	// TODO: not actually used by anything?
			}
		}
		else
		{
			m_movedNoteModel = nullptr;
		}

		// and let the user see that he pressed a key... :)
		update();
	}
}




// handler for mouse-release-event
/*! \brief Handle a mouse release event on the piano display view
 *
 *  If a key was pressed by the in the mousePressEvent() function, we
 *  turn the note off.
 *
 *  \param _me the mousePressEvent to handle.
 */
void PianoView::mouseReleaseEvent( QMouseEvent * )
{
	if( m_lastKey != -1 )
	{
		if( m_piano != nullptr )
		{
			m_piano->midiEventProcessor()->processInEvent( MidiEvent( MidiNoteOff, -1, m_lastKey, 0 ) );
			m_piano->setKeyState( m_lastKey, false );
		}

		// and let the user see that he released a key... :)
		update();

		m_lastKey = -1;
		m_movedNoteModel = nullptr;
	}
}




// handler for mouse-move-event
/*! \brief Handle a mouse move event on the piano display view
 *
 *  This handles the user dragging the mouse across the keys.  It uses
 *  code from mousePressEvent() and mouseReleaseEvent(), also correcting
 *  for if the mouse movement has stayed within one key and if the mouse
 *  has moved outside the vertical area of the keyboard (which is still
 *  allowed but won't make the volume go up to 11).
 *
 *  \param _me the ContextMenuEvent to handle.
 *  \todo Paul Wayper thinks that this code should be refactored to
 *  reduce or remove the duplication between this, the mousePressEvent()
 *  and mouseReleaseEvent() methods.
 */
void PianoView::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_piano == nullptr )
	{
		return;
	}

	int key_num = getKeyFromMouse( _me->pos() );
	int y_diff = _me->pos().y() - PIANO_BASE;
	int velocity = (int)( (float) y_diff /
		( Piano::isWhiteKey( key_num ) ?
			PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) *
						(float) m_piano->instrumentTrack()->midiPort()->baseVelocity() );
	// maybe the user moved the mouse-cursor above or under the
	// piano-widget while holding left button so check that and
	// correct volume if necessary
	if( y_diff < 0 )
	{
		velocity = 0;
	}
	else if( y_diff >
		( Piano::isWhiteKey( key_num ) ?
				PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) )
	{
		velocity = m_piano->instrumentTrack()->midiPort()->baseVelocity();
	}

	// is the calculated key different from current key? (could be the
	// user just moved the cursor one pixel left but on the same key)
	if( key_num != m_lastKey )
	{
		if( m_lastKey != -1 )
		{
			m_piano->midiEventProcessor()->processInEvent( MidiEvent( MidiNoteOff, -1, m_lastKey, 0 ) );
			m_piano->setKeyState( m_lastKey, false );
			m_lastKey = -1;
		}
		if( _me->buttons() & Qt::LeftButton )
		{
			if( _me->pos().y() > PIANO_BASE )
			{
				m_piano->midiEventProcessor()->processInEvent( MidiEvent( MidiNoteOn, -1, key_num, velocity ) );
				m_piano->setKeyState( key_num, true );
				m_lastKey = key_num;
			}
			else if (m_movedNoteModel != nullptr)
			{
				// upper section, move the base / first / last note marker
				m_movedNoteModel->setInitValue(static_cast<float>(key_num));
			}
		}
		// and let the user see that he pressed a key... :)
		update();
	}
	else if( m_piano->isKeyPressed( key_num ) )
	{
		m_piano->midiEventProcessor()->processInEvent( MidiEvent( MidiKeyPressure, -1, key_num, velocity ) );
	}

}




/*! \brief Handle a key press event on the piano display view
 *
 *  We determine our key number from the getKeyFromKeyEvent() method,
 *  and pass the event on to the piano's handleKeyPress() method if
 *  auto-repeat is off.
 *
 *  \param _ke the KeyEvent to handle.
 */
void PianoView::keyPressEvent( QKeyEvent * _ke )
{
	const int key_num = getKeyFromKeyEvent( _ke );

	if( _ke->isAutoRepeat() == false && key_num > -1 )
	{
		if( m_piano != nullptr )
		{
			m_piano->handleKeyPress( key_num );
			_ke->accept();
			update();
		}
	}
	else
	{
		_ke->ignore();
	}
}




/*! \brief Handle a key release event on the piano display view
 *
 *  The same logic as the keyPressEvent() method.
 *
 *  \param _ke the KeyEvent to handle.
 */
void PianoView::keyReleaseEvent( QKeyEvent * _ke )
{
	const int key_num = getKeyFromKeyEvent( _ke );
	if( _ke->isAutoRepeat() == false && key_num > -1 )
	{
		if( m_piano != nullptr )
		{
			m_piano->handleKeyRelease( key_num );
			_ke->accept();
			update();
		}
	}
	else
	{
		_ke->ignore();
	}
}




/*! \brief Handle the focus leaving the piano display view
 *
 *  Turn off all notes if we lose focus.
 *
 *  \todo Is there supposed to be a parameter given here?
 */
void PianoView::focusOutEvent( QFocusEvent * )
{
	if( m_piano == nullptr )
	{
		return;
	}

	// focus just switched to another control inside the instrument track
	// window we live in?
	if( parentWidget()->parentWidget()->focusWidget() != this &&
		parentWidget()->parentWidget()->focusWidget() != nullptr &&
		!(parentWidget()->parentWidget()->
				focusWidget()->inherits( "QLineEdit" ) ||
		parentWidget()->parentWidget()->
				focusWidget()->inherits( "QPlainTextEdit" ) ))
	{
		// then reclaim keyboard focus!
		setFocus();
		return;
	}

	// if we loose focus, we HAVE to note off all running notes because
	// we don't receive key-release-events anymore and so the notes would
	// hang otherwise
	for( int i = 0; i < NumKeys; ++i )
	{
		m_piano->midiEventProcessor()->processInEvent( MidiEvent( MidiNoteOff, -1, i, 0 ) );
		m_piano->setKeyState( i, false );
	}


	update();
}


void PianoView::focusInEvent( QFocusEvent * )
{
	m_piano->instrumentTrack()->autoAssignMidiDevice(true);
}



/*! \brief update scrollbar range after resize
 *
 *  After resizing we need to adjust range of scrollbar for not allowing
 *  to scroll too far to the right.
 *
 *  \param event resize-event object (unused)
 */
void PianoView::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	m_pianoScroll->setRange(0, Piano::NumWhiteKeys -
		static_cast<int>(floor(static_cast<float>(width()) / PW_WHITE_KEY_WIDTH)));
}




/*! \brief Convert a key number to an X coordinate in the piano display view
 *
 *  We can immediately discard the trivial case of when the key number is
 *  less than our starting key.  We then iterate through the keys from the
 *  start key to this key, adding the width of each key as we go.  For
 *  black keys, and the first white key if there is no black key between
 *  two white keys, we add half a white key width; for that second white
 *  key, we add a whole width.  That takes us to the boundary of a white
 *  key - subtract half a width to get to the middle.
 *
 *  \param _key_num the keyboard key to translate
 *  \todo is this description of what the method does correct?
 *  \todo replace the final subtract with initialising x to width/2.
 */
int PianoView::getKeyX( int _key_num ) const
{
	int k = m_startKey;
	if( _key_num < m_startKey )
	{
		return ( _key_num - k ) * PW_WHITE_KEY_WIDTH / 2;
	}

	int x = 0;
	int white_cnt = 0;

	while( k <= _key_num )
	{
		if( Piano::isWhiteKey( k ) )
		{
			++white_cnt;
			if( white_cnt > 1 )
			{
				x += PW_WHITE_KEY_WIDTH;
			}
			else
			{
				x += PW_WHITE_KEY_WIDTH/2;
			}
		}
		else
		{
			white_cnt = 0;
			x += PW_WHITE_KEY_WIDTH/2;
		}
		++k;
	}

	x -= PW_WHITE_KEY_WIDTH / 2;

	return x;

}


/*! \brief Return the width of a given key
 */
int PianoView::getKeyWidth(int key_num) const
{
	return Piano::isWhiteKey(key_num) ? PW_WHITE_KEY_WIDTH : PW_BLACK_KEY_WIDTH;
}

/*! \brief Return the height of a given key
 */
int PianoView::getKeyHeight(int key_num) const
{
	return Piano::isWhiteKey(key_num) ? PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT;
}


/*! \brief Return model and title of the marker closest to the given key
 */
IntModel* PianoView::getNearestMarker(int key, QString* title)
{
	const int base = m_piano->instrumentTrack()->baseNote();
	const int first = m_piano->instrumentTrack()->firstKey();
	const int last = m_piano->instrumentTrack()->lastKey();

	if (std::abs(key - base) < std::abs(key - first) && std::abs(key - base) < std::abs(key - last))
	{
		if (title) {*title = tr("Base note");}
		return m_piano->instrumentTrack()->baseNoteModel();
	}
	else if (std::abs(key - first) < std::abs(key - last))
	{
		if (title) {*title = tr("First note");}
		return m_piano->instrumentTrack()->firstKeyModel();
	}
	else
	{
		if (title) {*title = tr("Last note");}
		return m_piano->instrumentTrack()->lastKeyModel();
	}
}


/*! \brief Paint the piano display view in response to an event
 *
 *  This method draws the piano and the 'root note' base.  It draws
 *  the base first, then all the white keys, then all the black keys.
 *
 *  \todo Is there supposed to be a parameter given here?
 */
void PianoView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	// set smaller font for printing number of every octave
	p.setFont(adjustedToPixelSize(p.font(), LABEL_TEXT_SIZE));


	// draw bar above the keyboard (there will be the labels
	// for all C's)
	p.fillRect( QRect( 0, 1, width(), PIANO_BASE-2 ), p.background() );

	// draw the line above the keyboard
	p.setPen( Qt::black );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, PIANO_BASE-1, width(), PIANO_BASE-1 );

	p.setPen( Qt::white );

	// Controls for first / last / base key models are shown only if microtuner or its key range import are disabled
	if (m_piano != nullptr && !m_piano->instrumentTrack()->keyRangeImport())
	{
		// Draw the base note marker and first / last note boundary markers
		const int base_key = m_piano->instrumentTrack()->baseNoteModel()->value();
		const int first_key = m_piano->instrumentTrack()->firstKeyModel()->value();
		const int last_key = m_piano->instrumentTrack()->lastKeyModel()->value();
		QColor marker_color = QApplication::palette().color(QPalette::Active, QPalette::BrightText);

		// - prepare triangle shapes for start / end markers
		QPainterPath first_marker(QPoint(getKeyX(first_key) + 1, 1));
		first_marker.lineTo(getKeyX(first_key) + 1, PIANO_BASE);
		first_marker.lineTo(getKeyX(first_key) + PIANO_BASE / 2 + 1, PIANO_BASE / 2);

		QPainterPath last_marker(QPoint(getKeyX(last_key) + getKeyWidth(last_key), 1));
		last_marker.lineTo(getKeyX(last_key) + getKeyWidth(last_key), PIANO_BASE);
		last_marker.lineTo(getKeyX(last_key) + getKeyWidth(last_key) - PIANO_BASE / 2, PIANO_BASE / 2);

		// - fill all markers
		p.fillRect(QRect(getKeyX(base_key), 1, getKeyWidth(base_key) - 1, PIANO_BASE - 2), marker_color);
		p.fillPath(first_marker, marker_color);
		p.fillPath(last_marker, marker_color);
	}

	int cur_key = m_startKey;

	// draw all white keys...
	for (int x = 0; x < width();)
	{
		while (Piano::isBlackKey(cur_key))
		{
			++cur_key;
		}

		// draw normal, pressed or disabled key, depending on state and position of current key
		if (m_piano && m_piano->instrumentTrack()->isKeyMapped(cur_key))
		{
			if (m_piano && m_piano->isKeyPressed(cur_key))
			{
				p.drawPixmap(x, PIANO_BASE, m_whiteKeyPressedPm);
			}
			else
			{
				p.drawPixmap(x, PIANO_BASE, m_whiteKeyPm);
			}
		}
		else
		{
			p.drawPixmap(x, PIANO_BASE, m_whiteKeyDisabledPm);
		}

		x += PW_WHITE_KEY_WIDTH;

		if ((Key)(cur_key % KeysPerOctave) == Key::C)
		{
			// label key of note C with "C" and number of current octave
			p.drawText(x - PW_WHITE_KEY_WIDTH, LABEL_TEXT_SIZE + 2,
					   QString("C") + QString::number(FirstOctave + cur_key / KeysPerOctave));
		}
		++cur_key;
	}

	// reset all values, because now we're going to draw all black keys
	cur_key = m_startKey;
	int white_cnt = 0;

	int startKey = m_startKey;
	if (startKey > 0 && Piano::isBlackKey(--startKey))
	{
		if (m_piano && m_piano->instrumentTrack()->isKeyMapped(startKey))
		{
			if (m_piano && m_piano->isKeyPressed(startKey))
			{
				p.drawPixmap(0 - PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, m_blackKeyPressedPm);
			}
			else
			{
				p.drawPixmap(0 - PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, m_blackKeyPm);
			}
		}
		else
		{
			p.drawPixmap(0 - PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, m_blackKeyDisabledPm);
		}
	}

	// now draw all black keys...
	for (int x = 0; x < width();)
	{
		if (Piano::isBlackKey(cur_key))
		{
			// draw normal, pressed or disabled key, depending on state and position of current key
			if (m_piano && m_piano->instrumentTrack()->isKeyMapped(cur_key))
			{
				if (m_piano && m_piano->isKeyPressed(cur_key))
				{
					p.drawPixmap(x + PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, m_blackKeyPressedPm);
				}
				else
				{
					p.drawPixmap(x + PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, m_blackKeyPm);
				}
			}
			else
			{
				p.drawPixmap(x + PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, m_blackKeyDisabledPm);
			}
			x += PW_WHITE_KEY_WIDTH;
			white_cnt = 0;
		}
		else
		{
			// simple workaround for increasing x if there were two
			// white keys (e.g. between E and F)
			++white_cnt;
			if (white_cnt > 1)
			{
				x += PW_WHITE_KEY_WIDTH;
			}
		}
		// stop drawing when all keys are drawn, even if an extra black key could fit
		if (++cur_key == NumKeys) {break;}
	}
}


} // namespace lmms::gui

