/*
 * piano_roll.h - declaration of class pianoRoll which is a window where you
 *                can set and edit notes in an easy way
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


#ifndef _PIANO_ROLL_H
#define _PIANO_ROLL_H

#include <QtGui/QWidget>

#include "combobox_model.h"
#include "serializing_object.h"
#include "note.h"
#include "lmms_basics.h"


class QPainter;
class QPixmap;
class QScrollBar;

class comboBox;
class notePlayHandle;
class pattern;
class timeLine;
class toolButton;


class pianoRoll : public QWidget, public serializingObject
{
	Q_OBJECT
public:
	void setCurrentPattern( pattern * _new_pattern );

	inline bool isRecording( void ) const
	{
		return( m_recording );
	}

	inline const pattern * currentPattern( void ) const
	{
		return( m_pattern );
	}

	inline bool validPattern( void ) const
	{
		return( m_pattern != NULL );
	}

	int quantization( void ) const;


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName( void ) const
	{
		return( "pianoroll" );
	}


protected:
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void keyReleaseEvent( QKeyEvent * _ke );
	virtual void leaveEvent( QEvent * _e );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );
	virtual void wheelEvent( QWheelEvent * _we );
#ifdef LMMS_BUILD_LINUX
	virtual bool x11Event( XEvent * _xe );
#endif

	int getKey( int _y ) const;
	static inline void drawNoteRect( QPainter & _p, int _x, int _y,
					int  _width,
					const bool _is_selected,
					const bool _is_step_note );
	void removeSelection( void );
	void selectAll( void );
	void getSelectedNotes( noteVector & _selected_notes );


protected slots:
	void play( void );
	void record( void );
	void recordAccompany( void );
	void stop( void );

	void recordNote( const note & _n );

	void horScrolled( int _new_pos );
	void verScrolled( int _new_pos );

	void drawButtonToggled( void );
	void eraseButtonToggled( void );
	void selectButtonToggled( void );
	void moveButtonToggled( void );

	void copySelectedNotes( void );
	void cutSelectedNotes( void );
	void pasteNotes( void );
	void deleteSelectedNotes( void );

	void updatePosition( const midiTime & _t );

	void zoomingChanged( void );
	void quantizeChanged( void );


private:

	enum editModes
	{
		ModeDraw,
		ModeErase,
		ModeSelect,
		ModeMove,
		ModeOpen
	} ;

	enum actions
	{
		ActionNone,
		ActionMoveNote,
		ActionResizeNote,
		ActionSelectNotes,
		ActionMoveSelection,
		ActionChangeNoteVolume,
		ActionChangeNotePanning
	} ;

	enum pianoRollKeyTypes
	{
		PR_WHITE_KEY_SMALL,
		PR_WHITE_KEY_BIG,
		PR_BLACK_KEY
	} ;


	pianoRoll( void );
	pianoRoll( const pianoRoll & );
	virtual ~pianoRoll();

	midiTime newNoteLen( void ) const;
	
	void shiftPos(int amount);
	void shiftSemiTone(int amount);
	bool isSelection() const;
		
	static const int cm_scrollAmtHoriz = 10;
	static const int cm_scrollAmtVert = 1;
			
	static QPixmap * s_whiteKeyBigPm;
	static QPixmap * s_whiteKeySmallPm;
	static QPixmap * s_blackKeyPm;
	static QPixmap * s_toolDraw;
	static QPixmap * s_toolErase;
	static QPixmap * s_toolSelect;
	static QPixmap * s_toolMove;
	static QPixmap * s_toolOpen;

	static pianoRollKeyTypes prKeyOrder[];


	QWidget * m_toolBar;

	toolButton * m_playButton;
	toolButton * m_recordButton;
	toolButton * m_recordAccompanyButton;
	toolButton * m_stopButton;

	toolButton * m_drawButton;
	toolButton * m_eraseButton;
	toolButton * m_selectButton;

	toolButton * m_cutButton;
	toolButton * m_copyButton;
	toolButton * m_pasteButton;

	comboBox * m_zoomingComboBox;
	comboBox * m_quantizeComboBox;
	comboBox * m_noteLenComboBox;

	comboBoxModel m_zoomingModel;
	comboBoxModel m_quantizeModel;
	comboBoxModel m_noteLenModel;



	pattern * m_pattern;
	QScrollBar * m_leftRightScroll;
	QScrollBar * m_topBottomScroll;

	midiTime m_currentPosition;
	bool m_recording;

	note * m_currentNote;
	actions m_action;

	Uint32 m_selectStartTick;
	int m_selectedTick;
	int m_selectStartKey;
	int m_selectedKeys;

	int m_moveStartKey;
	int m_moveStartTick;
	int m_moveXOffset;

	int m_notesEditHeight;
	int m_ppt;
	int m_totalKeysToScroll;

	midiTime m_lenOfNewNotes;

	int m_startKey;			// first key when drawing
	int m_lastKey;

	noteVector m_selNotesForMove;


	editModes m_editMode;
	editModes m_ctrlMode; // mode they were in before they hit ctrl
		
	bool m_mouseDownLeft; //true if left click is being held down
	bool m_mouseDownRight; //true if right click is being held down

	timeLine * m_timeLine;
	bool m_scrollBack;

	void copy_to_clipboard( const noteVector & _notes ) const;

	void drawDetuningInfo( QPainter & _p, note * _n, int _x, int _y );
	bool mouseOverNote( void );
	note * noteUnderMouse( void );
	noteVector::const_iterator noteIteratorUnderMouse( void );
	
	// turn a selection rectangle into selected notes
	void computeSelectedNotes( bool shift ); 
	void clearSelectedNotes( void );

	friend class engine;


signals:
	void positionChanged( const midiTime & );

} ;


#endif

