#ifndef SINGLE_SOURCE_COMPILE

/*
 * piano.cpp - implementation of piano-widget used in instrument-track-window
 *             for testing
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


#include "piano.h"


#include <QtGui/QCursor>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>


#include "caption_menu.h"
#include "automatable_model_templates.h"
#include "embed.h"
#include "gui_templates.h"
#include "instrument_track.h"
#include "knob.h"
#include "midi.h"
#include "templates.h"
#include "update_event.h"

#ifdef Q_WS_X11

#include <X11/Xlib.h>

#endif


const KeyTypes KEY_ORDER[] =
{
	//    C		CIS	    D	      DIS	E	  F
	WhiteKey, BlackKey, WhiteKey, BlackKey, WhiteKey, WhiteKey,
	//   FIS      G		GIS	    A	   	B	   H
	BlackKey, WhiteKey, BlackKey, WhiteKey,  BlackKey, WhiteKey
} ;


Keys WhiteKeys[] =
{
	Key_C, Key_D, Key_E, Key_F, Key_G, Key_A, Key_H
} ;


QPixmap * pianoView::s_whiteKeyPm = NULL;
QPixmap * pianoView::s_blackKeyPm = NULL;
QPixmap * pianoView::s_whiteKeyPressedPm = NULL;
QPixmap * pianoView::s_blackKeyPressedPm = NULL;


const int PIANO_BASE = 11;
const int PW_WHITE_KEY_WIDTH = 10;
const int PW_BLACK_KEY_WIDTH = 8;
const int PW_WHITE_KEY_HEIGHT = 57;
const int PW_BLACK_KEY_HEIGHT = 38;
const int LABEL_TEXT_SIZE = 7;




piano::piano( instrumentTrack * _it ) :
	model( _it ),
	m_instrumentTrack( _it )
{
	for( int i = 0; i < KeysPerOctave * NumOctaves; ++i )
	{
		m_pressedKeys[i] = FALSE;
	}

}




piano::~piano()
{
}




void piano::setKeyState( int _key, bool _on )
{
	m_pressedKeys[tLimit( _key, 0, KeysPerOctave * NumOctaves - 1 )] = _on;
	emit dataChanged();
}




void piano::handleKeyPress( int _key )
{
	m_instrumentTrack->processInEvent( midiEvent( NOTE_ON, 0, _key,
						DefaultVolume ), midiTime() );
	m_pressedKeys[_key] = TRUE;
}





void piano::handleKeyRelease( int _key )
{
	m_instrumentTrack->processInEvent( midiEvent( NOTE_OFF, 0, _key, 0 ),
								midiTime() );
	m_pressedKeys[_key] = FALSE;
}







pianoView::pianoView( QWidget * _parent ) :
	QWidget( _parent ),
	modelView( NULL ),
	m_piano( NULL ),
	m_startKey( Key_C + Octave_3*KeysPerOctave ),
	m_lastKey( -1 )
{
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

	setFocusPolicy( Qt::StrongFocus );

	m_pianoScroll = new QScrollBar( Qt::Horizontal, this );
	m_pianoScroll->setRange( 0, WhiteKeysPerOctave * ( NumOctaves - 3 ) -
									4 );
	m_pianoScroll->setSingleStep( 1 );
	m_pianoScroll->setPageStep( 20 );
	m_pianoScroll->setValue( Octave_3 * WhiteKeysPerOctave );
	m_pianoScroll->setGeometry( 0, PIANO_BASE + PW_WHITE_KEY_HEIGHT, 250,
									16 );
	// ...and connect it to this widget...
	connect( m_pianoScroll, SIGNAL( valueChanged( int ) ),
					this, SLOT( pianoScrolled( int ) ) );

}




pianoView::~pianoView()
{
}




int pianoView::getKeyFromScancode( int _k )
{
	switch( _k )
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




void pianoView::modelChanged( void )
{
	m_piano = castModel<piano>();
	if( m_piano != NULL )
	{
		connect( m_piano->m_instrumentTrack->baseNoteModel(),
			SIGNAL( dataChanged() ), this, SLOT( update() ) );
	}

}




// gets the key from the given mouse-position
int pianoView::getKeyFromMouse( const QPoint & _p ) const
{
	int key_num = (int)( (float) _p.x() / (float) PW_WHITE_KEY_WIDTH );

	for( int i = 0; i <= key_num; ++i )
	{
		if( KEY_ORDER[( m_startKey+i ) % KeysPerOctave] == BlackKey )
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
		if( key_num > 0 && KEY_ORDER[( key_num - 1 ) % KeysPerOctave] ==
								BlackKey &&
			_p.x() % PW_WHITE_KEY_WIDTH <=
					( PW_WHITE_KEY_WIDTH / 2 ) -
						( PW_BLACK_KEY_WIDTH / 2 ) )
		{
			--key_num;
		}
		if( key_num < KeysPerOctave * NumOctaves - 1 &&
			KEY_ORDER[( key_num + 1 ) % KeysPerOctave] ==
								BlackKey &&
			_p.x() % PW_WHITE_KEY_WIDTH >=
				( PW_WHITE_KEY_WIDTH -
				  		PW_BLACK_KEY_WIDTH / 2 ) )
		{
			++key_num;
		}
	}

	// some range-checking-stuff
	return( tLimit( key_num, 0, KeysPerOctave * NumOctaves - 1 ) );
}




// handler for scrolling-event
void pianoView::pianoScrolled( int _new_pos )
{
	m_startKey = WhiteKeys[_new_pos % WhiteKeysPerOctave]+
			( _new_pos / WhiteKeysPerOctave ) * KeysPerOctave;

	update();
}




void pianoView::contextMenuEvent( QContextMenuEvent * _me )
{
	if( _me->pos().y() > PIANO_BASE || m_piano == NULL )
	{
		QWidget::contextMenuEvent( _me );
		return;
	}

	captionMenu contextMenu( tr( "Base note" ) );
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "&Open in automation editor" ),
		m_piano->m_instrumentTrack->baseNoteModel()->
							getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.exec( QCursor::pos() );
}




// handler for mouse-click-event
void pianoView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton && m_piano != NULL )
	{
		// get pressed key
		Uint32 key_num = getKeyFromMouse( _me->pos() );
		if( _me->pos().y() > PIANO_BASE )
		{
			int y_diff = _me->pos().y() - PIANO_BASE;
			volume vol = (volume)( ( float ) y_diff /
				( ( KEY_ORDER[key_num % KeysPerOctave] ==
								WhiteKey ) ?
				PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) *
				(float) DefaultVolume );
			if( y_diff < 0 )
			{
				vol = 0;
			}
			else if( y_diff > ( ( KEY_ORDER[key_num %
							KeysPerOctave] ==
								WhiteKey ) ?
				PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) )
			{
				vol = DefaultVolume;
			}
			// set note on
			m_piano->m_instrumentTrack->processInEvent(
					midiEvent( NOTE_ON, 0, key_num,
							vol * 127 / 100 ),
								midiTime() );
			m_piano->m_pressedKeys[key_num] = TRUE;
			m_lastKey = key_num;
		}
		else
		{
			m_piano->m_instrumentTrack->baseNoteModel()->
							setInitValue( key_num );
		}

		// and let the user see that he pressed a key... :)
		update();
	}
}




// handler for mouse-release-event
void pianoView::mouseReleaseEvent( QMouseEvent * _me )
{
	if( m_lastKey != -1 )
	{
		if( m_piano != NULL )
		{
			m_piano->m_instrumentTrack->processInEvent(
				midiEvent( NOTE_OFF, 0, m_lastKey, 0 ),
								midiTime() );
			m_piano->m_pressedKeys[m_lastKey] = FALSE;
		}

		// and let the user see that he released a key... :)
		update();

		m_lastKey = -1;
	}
}




// handler for mouse-move-event
void pianoView::mouseMoveEvent( QMouseEvent * _me )
{
	if( m_piano == NULL )
	{
		return;
	}

	int key_num = getKeyFromMouse( _me->pos() );
	int y_diff = _me->pos().y() - PIANO_BASE;
	volume vol = (volume)( (float) y_diff /
		( ( KEY_ORDER[key_num % KeysPerOctave] == WhiteKey ) ?
			PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) *
						(float)DefaultVolume );
	// maybe the user moved the mouse-cursor above or under the
	// piano-widget while holding left button so check that and
	// correct volume if necessary
	if( y_diff < 0 )
	{
		vol = 0;
	}
	else if( y_diff >
		( ( KEY_ORDER[key_num % KeysPerOctave] == WhiteKey ) ?
				PW_WHITE_KEY_HEIGHT : PW_BLACK_KEY_HEIGHT ) )
	{
		vol = DefaultVolume;
	}

	// is the calculated key different from current key? (could be the
	// user just moved the cursor one pixel left but on the same key)
	if( key_num != m_lastKey )
	{
		if( m_lastKey != -1 )
		{
			m_piano->m_instrumentTrack->processInEvent(
				midiEvent( NOTE_OFF, 0, m_lastKey, 0 ),
								midiTime() );
			m_piano->m_pressedKeys[m_lastKey] = FALSE;
			m_lastKey = -1;
		}
		if( _me->buttons() & Qt::LeftButton )
		{
			if( _me->pos().y() > PIANO_BASE )
			{
				m_piano->m_instrumentTrack->processInEvent(
					midiEvent( NOTE_ON, 0, key_num, vol ),
								midiTime() );
				m_piano->m_pressedKeys[key_num] = TRUE;
				m_lastKey = key_num;
			}
			else
			{
				m_piano->m_instrumentTrack->baseNoteModel()->
							setInitValue( key_num );
			}
		}
		// and let the user see that he pressed a key... :)
		update();
	}
	else if( m_piano->m_pressedKeys[key_num] == TRUE )
	{
		m_piano->m_instrumentTrack->processInEvent(
				midiEvent( KEY_PRESSURE, 0, key_num, vol ),
								midiTime() );
	}

}




void pianoView::keyPressEvent( QKeyEvent * _ke )
{
	int key_num = getKeyFromScancode( _ke->nativeScanCode() ) +
			( DefaultOctave - 1 ) * KeysPerOctave;

	if( _ke->isAutoRepeat() == FALSE && key_num > -1 )
	{
		if( m_piano != NULL )
		{
			m_piano->handleKeyPress( key_num );
			update();
		}
	}
	else
	{
		_ke->ignore();
	}
}




void pianoView::keyReleaseEvent( QKeyEvent * _ke )
{
	int key_num = getKeyFromScancode( _ke->nativeScanCode() ) +
				( DefaultOctave - 1 ) * KeysPerOctave;
	if( _ke->isAutoRepeat() == FALSE && key_num > -1 )
	{
		if( m_piano != NULL )
		{
			m_piano->handleKeyRelease( key_num );
			update();
		}
	}
	else
	{
		_ke->ignore();
	}
}




void pianoView::focusOutEvent( QFocusEvent * )
{
	if( m_piano == NULL )
	{
		return;
	}
	// if we loose focus, we HAVE to note off all running notes because
	// we don't receive key-release-events anymore and so the notes would
	// hang otherwise
	for( int i = 0; i < KeysPerOctave * NumOctaves; ++i )
	{
		if( m_piano->m_pressedKeys[i] == TRUE )
		{
			m_piano->m_instrumentTrack->processInEvent(
						midiEvent( NOTE_OFF, 0, i, 0 ),
								midiTime() );
			m_piano->m_pressedKeys[i] = FALSE;
		}
	}
	update();
}




int pianoView::getKeyX( int _key_num ) const
{
	int k = m_startKey;
	if( _key_num < m_startKey )
	{
		return( ( _key_num - k ) * PW_WHITE_KEY_WIDTH / 2 );
	}

	int x = 0;
	int white_cnt = 0;

	while( k <= _key_num )
	{
		if( KEY_ORDER[k % KeysPerOctave] == WhiteKey )
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




void pianoView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

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

	const int base_key = ( m_piano != NULL ) ?
		m_piano->m_instrumentTrack->baseNoteModel()->value() : 0;
	if( KEY_ORDER[base_key % KeysPerOctave] == WhiteKey )
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


	int cur_key = m_startKey;

	// draw all white keys...
	for( int x = 0; x < width(); )
	{
		while( KEY_ORDER[cur_key%KeysPerOctave] != WhiteKey )
		{
			++cur_key;
		}

		// draw pressed or not pressed key, depending on state of
		// current key
		if( m_piano && m_piano->m_pressedKeys[cur_key] == TRUE )
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

	int s_key = m_startKey;
	if( s_key > 0 &&
		KEY_ORDER[(Keys)( --s_key ) % KeysPerOctave] == BlackKey )
	{
		if( m_piano && m_piano->m_pressedKeys[s_key] == TRUE )
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
		if( KEY_ORDER[cur_key%KeysPerOctave] == BlackKey )
		{
			// draw pressed or not pressed key, depending on
			// state of current key
			if( m_piano && m_piano->m_pressedKeys[cur_key] == TRUE )
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
}





#include "piano.moc"


#endif
