#ifndef SINGLE_SOURCE_COMPILE

/*
 * piano_widget.cpp - implementation of piano-widget used in channel-window
 *                    for testing channel
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QCursor>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#else

#include <qcursor.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpopupmenu.h>

#endif


#include "piano_widget.h"
#include "instrument_track.h"
#include "midi.h"
#include "templates.h"
#include "embed.h"

#ifdef Q_WS_X11

#include <X11/Xlib.h>

#endif


const keyTypes KEY_ORDER[] =
{
	//    C		CIS	    D	      DIS	E	  F
	WHITE_KEY, BLACK_KEY, WHITE_KEY, BLACK_KEY, WHITE_KEY, WHITE_KEY,
	//   FIS      G		GIS	    A	   	B	   H
	BLACK_KEY, WHITE_KEY, BLACK_KEY, WHITE_KEY,  BLACK_KEY, WHITE_KEY
} ;


tones WHITE_KEYS[] =
{
	C, D, E, F, G, A, H
} ;


QPixmap * pianoWidget::s_whiteKeyPm = NULL;
QPixmap * pianoWidget::s_blackKeyPm = NULL;
QPixmap * pianoWidget::s_whiteKeyPressedPm = NULL;
QPixmap * pianoWidget::s_blackKeyPressedPm = NULL;


const int PIANO_BASE = 11;
const int PW_WHITE_KEY_WIDTH = 10;
const int PW_BLACK_KEY_WIDTH = 8;
const int PW_WHITE_KEY_HEIGHT = 57;
const int PW_BLACK_KEY_HEIGHT = 38;
const int LABEL_TEXT_SIZE = 7;


pianoWidget::pianoWidget( instrumentTrack * _parent ) :
	QWidget( _parent ),
	m_instrumentTrack( _parent ),
	m_startTone( C ),
	m_startOctave( OCTAVE_3 ),
	m_lastKey( -1 )
{
#ifdef QT4
	setFocusPolicy( Qt::StrongFocus );
#else
	setFocusPolicy( StrongFocus );
#endif

	if( s_whiteKeyPm == NULL )
	{
		s_whiteKeyPm = new QPixmap( embed::getIconPixmap(
								"white_key" ) );
	}
	if( s_blackKeyPm == NULL )
	{
		s_blackKeyPm = new QPixmap( embed::getIconPixmap(
								"black_key" ) );
	}
	if( s_whiteKeyPressedPm == NULL )
	{
		s_whiteKeyPressedPm = new QPixmap( embed::getIconPixmap(
							"white_key_pressed" ) );
	}
	if ( s_blackKeyPressedPm == NULL )
	{
		s_blackKeyPressedPm = new QPixmap( embed::getIconPixmap(
							"black_key_pressed" ) );
	}

	for( int i = 0; i < NOTES_PER_OCTAVE * OCTAVES; ++i )
	{
		m_pressedKeys[i] = FALSE;
	}

#ifdef QT4
	m_pianoScroll = new QScrollBar( Qt::Horizontal, this );
	m_pianoScroll->setRange( 0, WHITE_KEYS_PER_OCTAVE * ( OCTAVES - 3 ) -
									4 );
	m_pianoScroll->setSingleStep( 1 );
	m_pianoScroll->setPageStep( 20 );
	m_pianoScroll->setValue( OCTAVE_3 * WHITE_KEYS_PER_OCTAVE );
#else
	// create scrollbar below piano-widget...
	m_pianoScroll = new QScrollBar( 0, WHITE_KEYS_PER_OCTAVE *
					( OCTAVES - 3 ) - 4, 1, 20,
					OCTAVE_3 * WHITE_KEYS_PER_OCTAVE,
							Qt::Horizontal, this );
#endif
	m_pianoScroll->setGeometry( 0, PIANO_BASE + PW_WHITE_KEY_HEIGHT, 250,
									16 );
	// ...and connect it to this widget...
	connect( m_pianoScroll, SIGNAL( valueChanged( int ) ), this,
						SLOT( pianoScrolled( int ) ) );

#ifndef QT4
	// set background-mode for flicker-free redraw
	setBackgroundMode( Qt::NoBackground );
#endif

	m_noteKnob = new knob( knobDark_28, NULL, tr ( "Base note" ),
						_parent->eng(), _parent );
	m_noteKnob->setRange( 0, NOTES_PER_OCTAVE * OCTAVES - 1, 1.0f );
	m_noteKnob->setInitValue( DEFAULT_OCTAVE * NOTES_PER_OCTAVE + A );

	connect( m_noteKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( updateBaseNote( void ) ) );
}




pianoWidget::~pianoWidget()
{
	delete m_noteKnob;
}




// gets the key from the given mouse-position
int pianoWidget::getKeyFromMouse( const QPoint & _p )
{

	int key_num = (int)( (float) _p.x() / (float) PW_WHITE_KEY_WIDTH );

	for( int i = 0; i <= key_num; ++i )
	{
		if( KEY_ORDER[( m_startOctave * NOTES_PER_OCTAVE +
					m_startTone +i ) % NOTES_PER_OCTAVE] ==
			BLACK_KEY )
		{
			++key_num;
		}
	}

	key_num += m_startOctave * NOTES_PER_OCTAVE + m_startTone;

	// is it a black key?
	if( _p.y() < PIANO_BASE + PW_BLACK_KEY_HEIGHT )
	{
		// then do extra checking whether the mouse-cursor is over
		// a black key
		if( key_num > 0 &&
			KEY_ORDER[( key_num - 1 ) % NOTES_PER_OCTAVE] ==
								BLACK_KEY &&
			_p.x() % PW_WHITE_KEY_WIDTH <=
					( PW_WHITE_KEY_WIDTH / 2 ) -
						( PW_BLACK_KEY_WIDTH / 2 ) )
		{
			--key_num;
		}
		if( key_num < NOTES_PER_OCTAVE * OCTAVES - 1 &&
			KEY_ORDER[( key_num + 1 ) % NOTES_PER_OCTAVE] ==
								BLACK_KEY &&
			_p.x() % PW_WHITE_KEY_WIDTH >=
				( PW_WHITE_KEY_WIDTH -
				  		PW_BLACK_KEY_WIDTH / 2 ) )
		{
			++key_num;
		}
	}

	// some range-checking-stuff
	key_num = tLimit( key_num, 0, NOTES_PER_OCTAVE * OCTAVES - 1 );

	return( m_lastKey = key_num );
}




// handler for scrolling-event
void pianoWidget::pianoScrolled( int _new_pos )
{
	m_startTone = WHITE_KEYS[_new_pos % WHITE_KEYS_PER_OCTAVE];
	m_startOctave = (octaves)( _new_pos / WHITE_KEYS_PER_OCTAVE );

	update();
}




void pianoWidget::contextMenuEvent( QContextMenuEvent * _me )
{
	if( _me->pos().y() > PIANO_BASE )
	{
		QWidget::contextMenuEvent( _me );
		return;
	}

	QMenu contextMenu( this );
#ifdef QT4
	contextMenu.setTitle( m_noteKnob->accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( m_noteKnob->accessibleName() ) + "</b></font>",
			this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "&Open in automation editor" ),
					m_noteKnob->getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.exec( QCursor::pos() );
}




// handler for mouse-click-event
void pianoWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		// get pressed key
		Uint32 key_num = getKeyFromMouse( _me->pos() );
		if( _me->pos().y() > PIANO_BASE )
		{
			int y_diff = _me->pos().y() - PIANO_BASE;
			volume vol = (volume)( ( float ) y_diff /
				( ( KEY_ORDER[key_num % NOTES_PER_OCTAVE] ==
								WHITE_KEY ) ?
				PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) *
				(float) DEFAULT_VOLUME);
			if( y_diff < 0 )
			{
				vol = 0;
			}
			else if( y_diff > ( ( KEY_ORDER[key_num %
							NOTES_PER_OCTAVE] ==
								WHITE_KEY ) ?
				PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) )
			{
				vol = DEFAULT_VOLUME;
			}
			// set note on
			m_instrumentTrack->processInEvent(
					midiEvent( NOTE_ON, 0, key_num,
							vol * 127 / 100 ),
								midiTime() );
			m_pressedKeys[key_num] = TRUE;
		}
		else
		{
			m_noteKnob->setInitValue( key_num );
			m_instrumentTrack->setBaseNote( key_num );
		}

		// and let the user see that he pressed a key... :)
		update();
	}
}




// handler for mouse-release-event
void pianoWidget::mouseReleaseEvent( QMouseEvent * _me )
{
	int released_key = getKeyFromMouse( _me->pos() );

	m_instrumentTrack->processInEvent(
			midiEvent( NOTE_OFF, 0, released_key, 0 ), midiTime() );
	m_pressedKeys[released_key] = FALSE;

	// and let the user see that he released a key... :)
	update();
}




// handler for mouse-move-event
void pianoWidget::mouseMoveEvent( QMouseEvent * _me )
{
	// save current last-key-var
	int released_key = m_lastKey;
	int key_num = getKeyFromMouse( _me->pos() );
	int y_diff = _me->pos().y() - PIANO_BASE;
	volume vol = (volume)( (float) y_diff /
		( ( KEY_ORDER[key_num % NOTES_PER_OCTAVE] == WHITE_KEY ) ?
			PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) *
						(float)DEFAULT_VOLUME );
	// maybe the user moved the mouse-cursor above or under the
	// piano-widget while holding left button so check that and
	// correct volume if necessary
	if( y_diff < 0 )
	{
		vol = 0;
	}
	else if( y_diff >
		( ( KEY_ORDER[key_num % NOTES_PER_OCTAVE] == WHITE_KEY ) ?
				PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) )
	{
		vol = DEFAULT_VOLUME;
	}

	// is the calculated key different from current key? (could be the
	// user just moved the cursor one pixel left but on the same key)
	if( key_num != released_key )
	{
		m_instrumentTrack->processInEvent(
				midiEvent( NOTE_OFF, 0, released_key, 0 ),
								midiTime() );
		if( released_key >= 0 )
		{
			m_pressedKeys[released_key] = FALSE;
		}
#ifdef QT4
		if( _me->buttons() & Qt::LeftButton )
#else
		if( _me->state() == Qt::LeftButton )
#endif
		{
			if( _me->pos().y() > PIANO_BASE )
			{
				m_instrumentTrack->processInEvent(
					midiEvent( NOTE_ON, 0, key_num, vol ),
								midiTime() );
				m_pressedKeys[key_num] = TRUE;
			}
			else
			{
				m_noteKnob->setInitValue( key_num );
				m_instrumentTrack->setBaseNote( key_num );
			}
		}
		// and let the user see that he pressed a key... :)
		update();
	}
	else if( m_pressedKeys[key_num] == TRUE )
	{
		m_instrumentTrack->processInEvent(
				midiEvent( KEY_PRESSURE, 0, key_num, vol ),
								midiTime() );
	}

}




int pianoWidget::getKeyFromKeyboard( int _k ) const
{
	switch( m_keycode )
	{
		case 52: return( 0 ); // Y
		case 39: return( 1 ); // S
		case 53: return( 2 ); // X
		case 40: return( 3 ); // D
		case 54: return( 4 ); // C
		case 55: return( 5 ); // V
		case 42: return( 6 ); // G
		case 56: return( 7 ); // B
		case 43: return( 8 ); // H
		case 57: return( 9 ); // N
		case 44: return( 10 ); // J
		case 58: return( 11 ); // M
		case 24: return( 12 ); // Q
		case 11: return( 13 ); // 2
		case 25: return( 14 ); // W
		case 12: return( 15 ); // 3
		case 26: return( 16 ); // E
		case 27: return( 17 ); // R
		case 14: return( 18 ); // 5
		case 28: return( 19 ); // T
		case 15: return( 20 ); // 6
		case 29: return( 21 ); // Z
		case 16: return( 22 ); // 7
		case 30: return( 23 ); // U
		case 31: return( 24 ); // I
		case 18: return( 25 ); // 9
		case 32: return( 26 ); // O
		case 19: return( 27 ); // 0
		case 33: return( 28 ); // P
	}

	return( -100 );
}




void pianoWidget::keyPressEvent( QKeyEvent * _ke )
{
	int key_num = getKeyFromKeyboard( _ke->key() ) +
			( DEFAULT_OCTAVE - 1 ) * NOTES_PER_OCTAVE;

	if( _ke->isAutoRepeat() == FALSE && key_num > -1 )
	{
		m_instrumentTrack->processInEvent(
			midiEvent( NOTE_ON, 0, key_num, DEFAULT_VOLUME ),
								midiTime() );
		m_pressedKeys[key_num] = TRUE;
		update();
	}
	else
	{
		_ke->ignore();
	}
}




void pianoWidget::keyReleaseEvent( QKeyEvent * _ke )
{
	int key_num = getKeyFromKeyboard( _ke->key() ) +
				( DEFAULT_OCTAVE - 1 ) * NOTES_PER_OCTAVE;
	if( _ke->isAutoRepeat() == FALSE && key_num > -1 )
	{
		m_instrumentTrack->processInEvent(
					midiEvent( NOTE_OFF, 0, key_num, 0 ),
								midiTime() );
		m_pressedKeys[key_num] = FALSE;
		update();
	}
	else
	{
		_ke->ignore();
	}
}




void pianoWidget::focusOutEvent( QFocusEvent * )
{
	// if we loose focus, we HAVE to note off all running notes because
	// we don't receive key-release-events anymore and so the notes would
	// hang otherwise
	for( int i = 0; i < NOTES_PER_OCTAVE * OCTAVES; ++i )
	{
		if( m_pressedKeys[i] == TRUE )
		{
			m_instrumentTrack->processInEvent(
						midiEvent( NOTE_OFF, 0, i, 0 ),
								midiTime() );
			m_pressedKeys[i] = FALSE;
		}
	}
	update();
}




int pianoWidget::getKeyX( int _key_num )
{
	int k = m_startOctave*NOTES_PER_OCTAVE + m_startTone;
	if( _key_num < k )
	{
		return( ( _key_num - k ) * PW_WHITE_KEY_WIDTH / 2 );
	}

	int x = 0;
	int white_cnt = 0;

	while( k <= _key_num )
	{
		if( KEY_ORDER[k % NOTES_PER_OCTAVE] == WHITE_KEY )
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

	return( x );

}




void pianoWidget::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	// create pixmap for whole widget
	QPixmap pm( rect().size() );//width(), PIANO_BASE+PW_WHITE_KEY_HEIGHT);
	// and a painter for it
	QPainter p( &pm, this );
#endif

	// set smaller font for printing number of every octave
	p.setFont( pointSize<LABEL_TEXT_SIZE>( p.font() ) );


	// draw blue bar above the actual keyboard (there will be the labels
	// for all C's)
	p.fillRect( QRect( 0, 1, width(), PIANO_BASE-2 ),
						QColor( 0x00, 0x00, 0xFF ) );

	// draw stuff above the actual keyboard
	p.setPen( QColor( 0x00, 0x00, 0x00 ) );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, PIANO_BASE-1, width(), PIANO_BASE-1 );

	p.setPen( QColor ( 0xFF, 0xFF, 0xFF ) );

	int base_key = m_instrumentTrack->baseTone() +
			m_instrumentTrack->baseOctave() * NOTES_PER_OCTAVE;
	if( KEY_ORDER[base_key % NOTES_PER_OCTAVE] == WHITE_KEY )
	{
		p.fillRect( QRect( getKeyX( base_key ), 1, PW_WHITE_KEY_WIDTH-1,
								PIANO_BASE-2 ),
				QColor( 0xFF, 0xBB, 0x00 ) );
	}
	else
	{
		p.fillRect( QRect( getKeyX( base_key ) + 1, 1,
				PW_BLACK_KEY_WIDTH - 1, PIANO_BASE - 2 ),
				QColor( 0xFF, 0xBB, 0x00 ) );
	}


	int cur_key = m_startOctave*NOTES_PER_OCTAVE + m_startTone;

	// draw all white keys...
	for( int x = 0; x < width(); )
	{
		while( KEY_ORDER[cur_key%NOTES_PER_OCTAVE] != WHITE_KEY )
		{
			++cur_key;
		}

		// draw pressed or not pressed key, depending on state of
		// current key
		if( m_pressedKeys[cur_key] == TRUE )
		{
			p.drawPixmap( x, PIANO_BASE, *s_whiteKeyPressedPm );
		}
		else
		{
			p.drawPixmap( x, PIANO_BASE, *s_whiteKeyPm );
		}

		x += PW_WHITE_KEY_WIDTH;

		if( (tones) (cur_key%NOTES_PER_OCTAVE) == C )
		{
			// label key of note C with "C" and number of current
			// octave
			p.drawText( x - PW_WHITE_KEY_WIDTH, LABEL_TEXT_SIZE + 2,
					QString( "C" ) + QString::number(
					cur_key / NOTES_PER_OCTAVE, 10 ) );
		}
		++cur_key;
	}


	// reset all values, because now we're going to draw all black keys
	cur_key = m_startOctave*NOTES_PER_OCTAVE + m_startTone;
	int white_cnt = 0;

	int s_key = m_startOctave*NOTES_PER_OCTAVE+m_startTone;
	if( s_key > 0 &&
		KEY_ORDER[(tones)( --s_key ) % NOTES_PER_OCTAVE] == BLACK_KEY )
	{
		if( m_pressedKeys[s_key] == TRUE )
		{
			p.drawPixmap( 0 - PW_WHITE_KEY_WIDTH / 2, PIANO_BASE,
							*s_blackKeyPressedPm );
		}
		else
		{
			p.drawPixmap( 0 - PW_WHITE_KEY_WIDTH / 2, PIANO_BASE,
								*s_blackKeyPm );
		}
	}

	// now draw all black keys...
	for( int x = 0; x < width(); )
	{
		if( KEY_ORDER[cur_key%NOTES_PER_OCTAVE] == BLACK_KEY )
		{
			// draw pressed or not pressed key, depending on
			// state of current key
			if( m_pressedKeys[cur_key] == TRUE )
			{
				p.drawPixmap( x + PW_WHITE_KEY_WIDTH / 2,
								PIANO_BASE,
							*s_blackKeyPressedPm );
			}
			else
			{
				p.drawPixmap( x + PW_WHITE_KEY_WIDTH / 2,
						PIANO_BASE, *s_blackKeyPm );
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
#ifndef QT4
	// blit drawn pixmap to actual widget
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void pianoWidget::updateBaseNote( void )
{
	m_instrumentTrack->setBaseNote( (int)roundf( m_noteKnob->value() ),
									FALSE );
	update();
}




void pianoWidget::saveSettings( QDomDocument & _doc, QDomElement & _this,
							const QString & _name )
{
	m_noteKnob->saveSettings( _doc, _this, _name );
}




void pianoWidget::loadSettings( const QDomElement & _this,
							const QString & _name )
{
	if( _this.hasAttribute( "baseoct" ) )
	{
		m_noteKnob->setInitValue( _this.attribute( "baseoct" ).toInt()
				* NOTES_PER_OCTAVE
				+ _this.attribute( "basetone" ).toInt() );
	}
	else
	{
		m_noteKnob->loadSettings( _this, _name );
	}
	updateBaseNote();
}




bool pianoWidget::x11Event( XEvent * _xe )
{
	switch( _xe->type )
	{
		case KeyPress:
		case KeyRelease:
			m_keycode = _xe->xkey.keycode;
	}
	return( FALSE );
}




#include "piano_widget.moc"


#endif
