/*
 * piano_roll.h - declaration of class pianoRoll which is a window where you
 *                can set and edit notes in an easy way
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifndef _PIANO_ROLL_H
#define _PIANO_ROLL_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>
#include <QVector>

#else

#include <qwidget.h>
#include <qvaluevector.h>

#endif

#include "types.h"
#include "note.h"


class QComboBox;
class QPainter;
class QPixmap;
class QScrollBar;

class crystalButton;
class pattern;
class notePlayHandle;
class pixmapButton;
class timeLine;
class lmmsMainWin;



class pianoRoll : public QWidget
{
	Q_OBJECT
public:
	static inline pianoRoll * inst( void )
	{
		if( s_instanceOfMe == NULL )
		{
			s_instanceOfMe = new pianoRoll();
		}
		return( s_instanceOfMe );
	}

	void FASTCALL setCurrentPattern( pattern * _new_pattern );
	inline const pattern * currentPattern( void ) const
	{
		return( m_pattern );
	}
	inline bool validPattern( void ) const
	{
		return( m_pattern != NULL );
	}


protected:
	void closeEvent( QCloseEvent * _ce );
	void paintEvent( QPaintEvent * _pe );
	void resizeEvent( QResizeEvent * _re );
	void mousePressEvent( QMouseEvent * _me );
	void mouseReleaseEvent( QMouseEvent * _me );
	void mouseMoveEvent( QMouseEvent * _me );
	void keyPressEvent( QKeyEvent * _ke );
	void keyReleaseEvent( QKeyEvent * _ke );
	void wheelEvent( QWheelEvent * _we );

	int FASTCALL getKey( int _y );
	inline void drawNoteRect( QPainter & _p, Uint16 _x, Uint16 _y,
					Sint16 _width, bool _is_selected );
	void removeSelection( void );
	void selectAll( void );
	void FASTCALL getSelectedNotes( noteVector & _selected_notes );


protected slots:
	void play( void );
	void record( void );
	void stop( void );

	void recordNote( const note & _n );

	void horScrolled( int _new_pos );
	void verScrolled( int _new_pos );

	void drawButtonToggled( bool = FALSE );
	void eraseButtonToggled( bool = FALSE );
	void selectButtonToggled( bool = FALSE );
	void moveButtonToggled( bool = FALSE );

	void copySelectedNotes( void );
	void cutSelectedNotes( void );
	void pasteNotes( void );
	void deleteSelectedNotes( void );

	void updatePosition( const midiTime & _t );

	void zoomingChanged( const QString & _zfac );


private:

	enum editModes
	{
		DRAW,
		ERASE,
		SELECT,
		MOVE
	} ;

	enum actions
	{
		NONE,
		MOVE_NOTE,
		RESIZE_NOTE,
		SELECT_NOTES,
		MOVE_SELECTION,
		CHANGE_NOTE_VOLUME,
		CHANGE_NOTE_PANNING
	} ;

	enum pianoRollKeyTypes
	{
		PR_WHITE_KEY_SMALL,
		PR_WHITE_KEY_BIG,
		PR_BLACK_KEY
	} ;


	static pianoRoll * s_instanceOfMe;

	static QPixmap * s_whiteKeyBigPm;
	static QPixmap * s_whiteKeySmallPm;
	static QPixmap * s_artwork1;
	static QPixmap * s_artwork2;
	static QPixmap * s_blackKeyPm;
	static QPixmap * s_toolDraw;
	static QPixmap * s_toolErase;
	static QPixmap * s_toolSelect;
	static QPixmap * s_toolMove;

	static pianoRollKeyTypes prKeyOrder[];


	pixmapButton * m_playButton;
	pixmapButton * m_recordButton;
	pixmapButton * m_stopButton;

	crystalButton * m_drawButton;
	crystalButton * m_eraseButton;
	crystalButton * m_selectButton;
	crystalButton * m_moveButton;

	crystalButton * m_cutButton;
	crystalButton * m_copyButton;
	crystalButton * m_pasteButton;

	QComboBox * m_zoomingComboBox;


	pattern * m_pattern;
	QScrollBar * m_leftRightScroll;
	QScrollBar * m_topBottomScroll;

	midiTime m_currentPosition;
	bool m_recording;

	note * m_currentNote;
	actions m_action;

	Uint32 m_selectStartTact64th;
	int m_selectedTact64th;
	int m_selectStartKey;
	int m_selectedKeys;

	int m_moveStartKey;
	int m_moveStartTact64th;
	int m_moveXOffset;

	int m_notesEditHeight;
	int m_ppt;
	int m_totalKeysToScroll;

	midiTime m_lenOfNewNotes;

	bool m_shiftPressed;
	bool m_controlPressed;

	int m_startKey;			// first key when drawing

	int m_lastKey;

	noteVector m_notesToCopy;
	noteVector m_selNotesForMove;


	editModes m_editMode;


	timeLine * m_timeLine;
	bool m_scrollBack;


	pianoRoll( void );
	pianoRoll( const pianoRoll & );
	~pianoRoll();


	friend class lmmsMainWin;


signals:
	void positionChanged( const midiTime & );

} ;


#endif

