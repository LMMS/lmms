/*
 * piano_widget.h - declaration of class pianoView, a widget which provides
 *                  an interactive piano/keyboard-widget
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


#ifndef _PIANO_WIDGET_H
#define _PIANO_WIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QPixmap>
#include <QtGui/QScrollBar>


#include "note.h"
#include "automatable_model.h"


class instrumentTrack;
class instrumentTrackView;
class notePlayHandle;


enum KeyTypes
{
	WhiteKey,
	BlackKey
} ;


class piano : public model
{
public:
	piano( instrumentTrack * _it );
	virtual ~piano();

	void setKeyState( int _key, bool _on = FALSE );
	static int getKeyFromKeycode( int _kc );


private:
	instrumentTrack * m_instrumentTrack;
	bool m_pressedKeys[NOTES_PER_OCTAVE * OCTAVES];


	friend class pianoView;

} ;



class pianoView : public QWidget, public modelView
{
	Q_OBJECT
public:
	pianoView( QWidget * _parent );
	virtual ~pianoView();


protected:
	virtual void modelChanged( void );
	virtual void keyPressEvent( QKeyEvent * ke );
	virtual void keyReleaseEvent( QKeyEvent * ke );
#ifndef BUILD_WIN32
	virtual bool x11Event( XEvent * _xe );
#endif
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * );
	virtual void mousePressEvent( QMouseEvent * me );
	virtual void mouseReleaseEvent( QMouseEvent * me );
	virtual void mouseMoveEvent( QMouseEvent * me );
	virtual void focusOutEvent( QFocusEvent * _fe );


private:
	int getKeyFromMouse( const QPoint & _p ) const;
	int getKeyX( int _key_num ) const;

	static QPixmap * s_whiteKeyPm;
	static QPixmap * s_blackKeyPm;
	static QPixmap * s_whiteKeyPressedPm;
	static QPixmap * s_blackKeyPressedPm;

	piano * m_piano;

	QScrollBar * m_pianoScroll;
	tones m_startTone;			// first key when drawing
	octaves m_startOctave;

	int m_lastKey;
	unsigned int m_keyCode;


private slots:
	void pianoScrolled( int _new_pos );

} ;


#endif

