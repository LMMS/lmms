/*
 * PianoView.h - declaration of PianoView, an interactive piano/keyboard-widget
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _PIANO_VIEW_H
#define _PIANO_VIEW_H

#include <QtGui/QPixmap>
#include <QtGui/QScrollBar>

#include "ModelView.h"

class Piano;


class PianoView : public QWidget, public ModelView
{
	Q_OBJECT
public:
	PianoView( QWidget * _parent );
	virtual ~PianoView();

	static int getKeyFromKeyEvent( QKeyEvent * _ke );


public:
	virtual void keyPressEvent( QKeyEvent * ke );
	virtual void keyReleaseEvent( QKeyEvent * ke );


protected:
	virtual void modelChanged();
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void paintEvent( QPaintEvent * );
	virtual void mousePressEvent( QMouseEvent * me );
	virtual void mouseReleaseEvent( QMouseEvent * me );
	virtual void mouseMoveEvent( QMouseEvent * me );
	virtual void focusOutEvent( QFocusEvent * _fe );
	virtual void resizeEvent( QResizeEvent * _event );


private:
	int getKeyFromMouse( const QPoint & _p ) const;
	int getKeyX( int _key_num ) const;

	static QPixmap * s_whiteKeyPm;
	static QPixmap * s_blackKeyPm;
	static QPixmap * s_whiteKeyPressedPm;
	static QPixmap * s_blackKeyPressedPm;

	Piano * m_piano;

	QScrollBar * m_pianoScroll;
	int m_startKey;			// first key when drawing
	int m_lastKey;


private slots:
	void pianoScrolled( int _new_pos );

signals:
	void keyPressed( int );
	void baseNoteChanged();

} ;

// Start of PianoKeyboard.h
class PianoKeyboard
{
public:

	static int getLinuxKey(int key);
	static int getWindowsKey(int key);
	static int getAppleKey(int key);

	static int getScaledNoteByIndex(int index);

	static QString getNoteByNumber(int index)	   { return m_notes[index]; }

	static int	   getOctaveNumber()			   { return m_octave; }
	static void	   setOctaveNumber(int octave)	   { m_octave = octave - 3; }

	static QString getScaleNote()				   { return m_notes[m_scaleNoteIndex]; }
	static int	   getScaleNoteIndex()			   { return m_scaleNoteIndex; }
	static void	   setScaleNote(QString note);

	static QString getScaleType()				   { return m_scaleType; }
	static void	   setScaleType(QString scale)	   { m_scaleType = scale; }

private:
	static int		m_octave;
	static int		m_scaleNoteIndex;
	static QString	m_scaleType;

	static const int majorNotes[];		// W W H W W W H
	static const int minorNotes[];		// W H W W H W W
	static const int lydianNotes[];		// W W W H W W H

	static QString m_notes[];

};
// End of PianoKeyboard.h

#endif

