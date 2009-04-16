/*
 * piano.h - piano and pianoView, an interactive piano/keyboard-widget
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _PIANO_H
#define _PIANO_H

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

	void handleKeyPress( int _key );
	void handleKeyRelease( int _key );


private:
	instrumentTrack * m_instrumentTrack;
	bool m_pressedKeys[NumKeys];


	friend class pianoView;

} ;



class pianoView : public QWidget, public modelView
{
	Q_OBJECT
public:
	pianoView( QWidget * _parent );
	virtual ~pianoView();

	static int getKeyFromKeyEvent( QKeyEvent * _ke );


protected:
	virtual void modelChanged( void );
	virtual void keyPressEvent( QKeyEvent * ke );
	virtual void keyReleaseEvent( QKeyEvent * ke );
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
	int m_startKey;			// first key when drawing
	int m_lastKey;


private slots:
	void pianoScrolled( int _new_pos );

} ;


#endif

