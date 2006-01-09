/*
 * piano_widget.h - declaration of class pianoWidget, a widget which provides
 *                  an interactive piano/keyboard-widget
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


#ifndef _PIANO_WIDGET_H
#define _PIANO_WIDGET_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>
#include <QPixmap>
#include <QScrollBar>

#else

#include <qwidget.h>
#include <qpixmap.h>
#include <qscrollbar.h>

#endif


#include "note.h"
#include "templates.h"


class channelTrack;
class notePlayHandle;


enum keyTypes
{
	WHITE_KEY,
	BLACK_KEY
} ;


class pianoWidget : public QWidget
{
	Q_OBJECT
public:
	pianoWidget( channelTrack * _channel_track );
	virtual ~pianoWidget();

	inline void setKeyState( int _key, bool _on = FALSE )
	{
		m_pressedKeys[tLimit( _key, 0, NOTES_PER_OCTAVE *
							OCTAVES -1 )] = _on;
		update();
	}


protected:
	virtual void paintEvent( QPaintEvent * );
	virtual void mousePressEvent( QMouseEvent * me );
	virtual void mouseReleaseEvent( QMouseEvent * me );
	virtual void mouseMoveEvent( QMouseEvent * me );
	virtual void keyPressEvent( QKeyEvent * ke );
	virtual void keyReleaseEvent( QKeyEvent * ke );
	virtual void focusOutEvent( QFocusEvent * _fe );


private:
	int FASTCALL getKeyFromMouse( const QPoint & _p );
	int FASTCALL getKeyFromKeyboard( int _k ) const;
	int FASTCALL getKeyX( int _key_num );

	static QPixmap * s_whiteKeyPm;
	static QPixmap * s_blackKeyPm;
	static QPixmap * s_whiteKeyPressedPm;
	static QPixmap * s_blackKeyPressedPm;
	
	bool m_pressedKeys[NOTES_PER_OCTAVE * OCTAVES];

	QScrollBar * m_pianoScroll;
	channelTrack * m_channelTrack;
	tones m_startTone;			// first key when drawing
	octaves m_startOctave;

	int m_lastKey;


private slots:
	void pianoScrolled( int _new_pos );

} ;


#endif

