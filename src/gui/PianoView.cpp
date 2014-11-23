/*
 * Piano.cpp - implementation of piano-widget used in instrument-track-window
 *             for testing + according model class
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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


#include <QtGui/QCursor>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QVBoxLayout>


#include "PianoView.h"
#include "caption_menu.h"
#include "embed.h"
#include "engine.h"
#include "gui_templates.h"
#include "InstrumentTrack.h"
#include "knob.h"
#include "string_pair_drag.h"
#include "MainWindow.h"
#include "MidiEvent.h"
#include "templates.h"
#include "update_event.h"


/*! The black / white order of keys as they appear on the keyboard.
 */
const Piano::KeyTypes KEY_ORDER[] =
{
//	C                CIS              D                DIS
	Piano::WhiteKey, Piano::BlackKey, Piano::WhiteKey, Piano::BlackKey,
//	E                F                FIS              G
	Piano::WhiteKey, Piano::WhiteKey, Piano::BlackKey, Piano::WhiteKey,
//	GIS              A                B                H
	Piano::BlackKey, Piano::WhiteKey, Piano::BlackKey, Piano::WhiteKey
} ;


/*! The scale of C Major - white keys only.
 */
Keys WhiteKeys[] =
{
	Key_C, Key_D, Key_E, Key_F, Key_G, Key_A, Key_H
} ;


QPixmap * PianoView::s_whiteKeyPm = NULL;           /*!< A white key released */
QPixmap * PianoView::s_blackKeyPm = NULL;           /*!< A black key released */
QPixmap * PianoView::s_whiteKeyPressedPm = NULL;    /*!< A white key pressed */
QPixmap * PianoView::s_blackKeyPressedPm = NULL;    /*!< A black key pressed */


const int PIANO_BASE = 11;          /*!< The height of the root note display */
const int PW_WHITE_KEY_WIDTH = 10;  /*!< The width of a white key */
const int PW_BLACK_KEY_WIDTH = 8;   /*!< The width of a black key */
const int PW_WHITE_KEY_HEIGHT = 57; /*!< The height of a white key */
const int PW_BLACK_KEY_HEIGHT = 38; /*!< The height of a black key */
const int LABEL_TEXT_SIZE = 7;      /*!< The height of the key label text */




/*! \brief Create a new keyboard display view
 *
 *  \param _parent the parent instrument plugin window
 *  \todo are the descriptions of the m_startkey and m_lastkey properties correct?
 */
PianoView::PianoView( QWidget * _parent ) :
	QWidget( _parent ),             /*!< Our parent */
	ModelView( NULL, this ),        /*!< Our view Model */
	m_piano( NULL ),                /*!< Our piano Model */
	m_startKey( Key_C + Octave_3*KeysPerOctave ), /*!< The first key displayed? */
	m_lastKey( -1 )                 /*!< The last key displayed? */
{
	if( s_whiteKeyPm == NULL )
	{
		s_whiteKeyPm = new QPixmap( embed::getIconPixmap( "white_key" ) );
	}
	if( s_blackKeyPm == NULL )
	{
		s_blackKeyPm = new QPixmap( embed::getIconPixmap( "black_key" ) );
	}
	if( s_whiteKeyPressedPm == NULL )
	{
		s_whiteKeyPressedPm = new QPixmap( embed::getIconPixmap( "white_key_pressed" ) );
	}
	if ( s_blackKeyPressedPm == NULL )
	{
		s_blackKeyPressedPm = new QPixmap( embed::getIconPixmap( "black_key_pressed" ) );
	}

	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setFocusPolicy( Qt::StrongFocus );
	setMaximumWidth( WhiteKeysPerOctave * NumOctaves * PW_WHITE_KEY_WIDTH );

	// create scrollbar at the bottom
	m_pianoScroll = new QScrollBar( Qt::Horizontal, this );
	m_pianoScroll->setSingleStep( 1 );
	m_pianoScroll->setPageStep( 20 );
	m_pianoScroll->setValue( Octave_3 * WhiteKeysPerOctave );

	// and connect it to this widget
	connect( m_pianoScroll, SIGNAL( valueChanged( int ) ),
			this, SLOT( pianoScrolled( int ) ) );

	// create a layout for ourselves
	QVBoxLayout * layout = new QVBoxLayout( this );
	layout->setSpacing( 0 );
	layout->setMargin( 0 );
	layout->addSpacing( PIANO_BASE+PW_WHITE_KEY_HEIGHT );
	layout->addWidget( m_pianoScroll );

}




/*! \brief Destroy this piano display view
 *
 */
PianoView::~PianoView()
{
}




/*! \brief Map a keyboard key being pressed to a note in our keyboard view
 *
 *  \param _k The keyboard scan code of the key being pressed.
 *  \todo check the scan codes for ',' = c, 'L' = c#, '.' = d, ':' = d#,
 *     '/' = d, '[' = f', '=' = f'#, ']' = g' - Paul's additions
 */
int PianoView::getKeyFromKeyEvent( QKeyEvent * _ke )
{
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
#endif
#ifdef LMMS_BUILD_LINUX
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
#endif

	return -100;
}




/*! \brief Register a change to this piano display view
 *
 */
void PianoView::modelChanged()
{
	m_piano = castModel<Piano>();
	if( m_piano != NULL )
	{
		connect( m_piano->instrumentTrack()->baseNoteModel(), SIGNAL( dataChanged() ),
					this, SLOT( update() ) );
	}

}




// gets the key from the given mouse-position
/*! \brief Get the key from the mouse position in the piano display
 *
 *  First we determine it roughly by the position of the point given in
 *  white key widths from our start.  We then add in any black keys that
 *  might have been skipped over (they take a key number, but no 'white
 *  key' space).  We then add in our starting key number.
 *
 *  We then determine whether it was a black key that was pressed by
 *  checking whether it was within the vertical range of black keys.
 *  Black keys sit exactly between white keys on this keyboard, so
 *  we then shift the note down or up if we were in the left or right
 *  half of the white note.  We only do this, of course, if the white
 *  note has a black key on that side, so to speak.
 *
 *  This function returns const because there is a linear mapping from
 *  the point given to the key returned that never changes.
 *
 *  \param _p The point that the mouse was pressed.
 */
int PianoView::getKeyFromMouse( const QPoint & _p ) const
{
	int key_num = (int)( (float) _p.x() / (float) PW_WHITE_KEY_WIDTH );

	for( int i = 0; i <= key_num; ++i )
	{
		if( KEY_ORDER[( m_startKey+i ) % KeysPerOctave] ==
							Piano::BlackKey )
		{
			++key_num;
		}
	}

	key_num += m_startKey;

	// is it a black key?
	if( _p.y() < PIANO_BASE + PW_BLACK_KEY_HEIGHT )
	{
		// then do extra checking whether the mouse-cursor is over
		// a black key
		if( key_num > 0 && KEY_ORDER[(key_num-1 ) % KeysPerOctave] ==
							Piano::BlackKey &&
			_p.x() % PW_WHITE_KEY_WIDTH <=
					( PW_WHITE_KEY_WIDTH / 2 ) -
						( PW_BLACK_KEY_WIDTH / 2 ) )
		{
			--key_num;
		}
		if( key_num < NumKeys - 1 &&
			KEY_ORDER[( key_num + 1 ) % KeysPerOctave] ==
							Piano::BlackKey &&
			_p.x() % PW_WHITE_KEY_WIDTH >=
				( PW_WHITE_KEY_WIDTH -
				  		PW_BLACK_KEY_WIDTH / 2 ) )
		{
			++key_num;
		}
	}

	// some range-checking-stuff
	return tLimit( key_num, 0, NumKeys - 1 );
}




// handler for scrolling-event
/*! \brief Handle the scrolling on the piano display view
 *
 *  We need to update our start key position based on the new position.
 *
 *  \param _new_pos the new key position.
 */
void PianoView::pianoScrolled( int _new_pos )
{
	m_startKey = WhiteKeys[_new_pos % WhiteKeysPerOctave]+
			( _new_pos / WhiteKeysPerOctave ) * KeysPerOctave;

	update();
}




/*! \brief Handle a context menu selection on the piano display view
 *
 *  \param _me the ContextMenuEvent to handle.
 *  \todo Is this right, or does this create the context menu?
 */
void PianoView::contextMenuEvent( QContextMenuEvent * _me )
{
	if( _me->pos().y() > PIANO_BASE || m_piano == NULL )
	{
		QWidget::contextMenuEvent( _me );
		return;
	}

	captionMenu contextMenu( tr( "Base note" ) );
	AutomatableModelView amv( m_piano->instrumentTrack()->baseNoteModel(), &contextMenu );
	amv.addDefaultActions( &contextMenu );
	contextMenu.exec( QCursor::pos() );
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
 *  \param _me the mouse click to handle.
 */
void PianoView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton && m_piano != NULL )
	{
		// get pressed key
		int key_num = getKeyFromMouse( _me->pos() );
		if( _me->pos().y() > PIANO_BASE )
		{
			int y_diff = _me->pos().y() - PIANO_BASE;
			int velocity = (int)( ( float ) y_diff /
				( ( KEY_ORDER[key_num % KeysPerOctave] ==
							Piano::WhiteKey ) ?
				PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) *
						(float) m_piano->instrumentTrack()->midiPort()->baseVelocity() );
			if( y_diff < 0 )
			{
				velocity = 0;
			}
			else if( y_diff >
				( ( KEY_ORDER[key_num % KeysPerOctave] ==
							Piano::WhiteKey ) ?
				PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) )
			{
				velocity = m_piano->instrumentTrack()->midiPort()->baseVelocity();
			}
			// set note on
			m_piano->midiEventProcessor()->processInEvent( MidiEvent( MidiNoteOn, -1, key_num, velocity ) );
			m_piano->setKeyState( key_num, true );
			m_lastKey = key_num;

			emit keyPressed( key_num );
		}
		else
		{
			if( _me->modifiers() & Qt::ControlModifier )
			{
				new stringPairDrag( "automatable_model",
					QString::number( m_piano->instrumentTrack()->baseNoteModel()->id() ),
					QPixmap(), this );
				_me->accept();
			}
			else
			{
				m_piano->instrumentTrack()->baseNoteModel()->setInitValue( (float) key_num );

				emit baseNoteChanged();
			}
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
		if( m_piano != NULL )
		{
			m_piano->midiEventProcessor()->processInEvent( MidiEvent( MidiNoteOff, -1, m_lastKey, 0 ) );
			m_piano->setKeyState( m_lastKey, false );
		}

		// and let the user see that he released a key... :)
		update();

		m_lastKey = -1;
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
	if( m_piano == NULL )
	{
		return;
	}

	int key_num = getKeyFromMouse( _me->pos() );
	int y_diff = _me->pos().y() - PIANO_BASE;
	int velocity = (int)( (float) y_diff /
		( ( KEY_ORDER[key_num % KeysPerOctave] == Piano::WhiteKey ) ?
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
		( ( KEY_ORDER[key_num % KeysPerOctave] == Piano::WhiteKey ) ?
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
			else
			{
				m_piano->instrumentTrack()->baseNoteModel()->setInitValue( (float) key_num );
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
	const int key_num = getKeyFromKeyEvent( _ke ) +
				( DefaultOctave - 1 ) * KeysPerOctave;

	if( _ke->isAutoRepeat() == false && key_num > -1 )
	{
		if( m_piano != NULL )
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
	const int key_num = getKeyFromKeyEvent( _ke ) +
				( DefaultOctave - 1 ) * KeysPerOctave;
	if( _ke->isAutoRepeat() == false && key_num > -1 )
	{
		if( m_piano != NULL )
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
	if( m_piano == NULL )
	{
		return;
	}

	// focus just switched to another control inside the instrument track
	// window we live in?
	if( parentWidget()->parentWidget()->focusWidget() != this &&
		parentWidget()->parentWidget()->focusWidget() != NULL &&
		!parentWidget()->parentWidget()->
				focusWidget()->inherits( "QLineEdit" ) )
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




/*! \brief update scrollbar range after resize
 *
 *  After resizing we need to adjust range of scrollbar for not allowing
 *  to scroll too far to the right.
 *
 *  \param _event resize-event object (unused)
 */
void PianoView::resizeEvent( QResizeEvent * _event )
{
	QWidget::resizeEvent( _event );
	m_pianoScroll->setRange( 0, WhiteKeysPerOctave * NumOctaves -
					(int) ceil( (float) width() /
							PW_WHITE_KEY_WIDTH ) );
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
		if( KEY_ORDER[k % KeysPerOctave] == Piano::WhiteKey )
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
	p.setFont( pointSize<LABEL_TEXT_SIZE>( p.font() ) );


	// draw blue bar above the actual keyboard (there will be the labels
	// for all C's)
	QLinearGradient g( 0, 0, 0, PIANO_BASE-3 );
	g.setColorAt( 0, Qt::black );
	g.setColorAt( 0.1, QColor( 96, 96, 96 ) );
	g.setColorAt( 1, Qt::black );
	p.fillRect( QRect( 0, 1, width(), PIANO_BASE-2 ), g );

	// draw stuff above the actual keyboard
	p.setPen( Qt::black );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, PIANO_BASE-1, width(), PIANO_BASE-1 );

	p.setPen( Qt::white );

	const int base_key = ( m_piano != NULL ) ?
		m_piano->instrumentTrack()->baseNoteModel()->value() : 0;
	g.setColorAt( 0, QApplication::palette().color( QPalette::Active,
							QPalette::BrightText ).darker(220) );
	g.setColorAt( 0.1, QApplication::palette().color( QPalette::Active,
							QPalette::BrightText ) );
	g.setColorAt( 1, QApplication::palette().color( QPalette::Active,
							QPalette::BrightText ) );
	if( KEY_ORDER[base_key % KeysPerOctave] == Piano::WhiteKey )
	{
		p.fillRect( QRect( getKeyX( base_key ), 1, PW_WHITE_KEY_WIDTH-1,
							PIANO_BASE-2 ), g );
	}
	else
	{
		p.fillRect( QRect( getKeyX( base_key ) + 1, 1,
				PW_BLACK_KEY_WIDTH - 1, PIANO_BASE - 2 ), g );
	}


	int cur_key = m_startKey;

	// draw all white keys...
	for( int x = 0; x < width(); )
	{
		while( KEY_ORDER[cur_key%KeysPerOctave] != Piano::WhiteKey )
		{
			++cur_key;
		}

		// draw pressed or not pressed key, depending on state of
		// current key
		if( m_piano && m_piano->isKeyPressed( cur_key ) )
		{
			p.drawPixmap( x, PIANO_BASE, *s_whiteKeyPressedPm );
		}
		else
		{
			p.drawPixmap( x, PIANO_BASE, *s_whiteKeyPm );
		}

		x += PW_WHITE_KEY_WIDTH;

		if( (Keys) (cur_key%KeysPerOctave) == Key_C )
		{
			// label key of note C with "C" and number of current
			// octave
			p.drawText( x - PW_WHITE_KEY_WIDTH, LABEL_TEXT_SIZE + 2,
					QString( "C" ) + QString::number(
					cur_key / KeysPerOctave, 10 ) );
		}
		++cur_key;
	}


	// reset all values, because now we're going to draw all black keys
	cur_key = m_startKey;
	int white_cnt = 0;

	int startKey = m_startKey;
	if( startKey > 0 &&
		KEY_ORDER[(Keys)(--startKey) % KeysPerOctave] == Piano::BlackKey )
	{
		if( m_piano && m_piano->isKeyPressed( startKey ) )
		{
			p.drawPixmap( 0 - PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, *s_blackKeyPressedPm );
		}
		else
		{
			p.drawPixmap( 0 - PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, *s_blackKeyPm );
		}
	}

	// now draw all black keys...
	for( int x = 0; x < width(); )
	{
		if( KEY_ORDER[cur_key%KeysPerOctave] == Piano::BlackKey )
		{
			// draw pressed or not pressed key, depending on
			// state of current key
			if( m_piano && m_piano->isKeyPressed( cur_key ) )
			{
				p.drawPixmap( x + PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, *s_blackKeyPressedPm );
			}
			else
			{
				p.drawPixmap( x + PW_WHITE_KEY_WIDTH / 2, PIANO_BASE, *s_blackKeyPm );
			}
			x += PW_WHITE_KEY_WIDTH;
			white_cnt = 0;
		}
		else
		{
			// simple workaround for increasing x if there were two
			// white keys (e.g. between E and F)
			++white_cnt;
			if( white_cnt > 1 )
			{
				x += PW_WHITE_KEY_WIDTH;
			}
		}
		++cur_key;
	}
}




#include "moc_PianoView.cxx"

