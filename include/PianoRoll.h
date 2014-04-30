/*
 * PianoRoll.h - declaration of class PianoRoll which is a window where you
 *               can set and edit notes in an easy way
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008 Andrew Kelley <superjoe30/at/gmail/dot/com>
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

#ifndef PIANO_ROLL_H
#define PIANO_ROLL_H

#include <QtGui/QWidget>

#include "ComboBoxModel.h"
#include "SerializingObject.h"
#include "note.h"
#include "lmms_basics.h"
#include "song.h"


class QPainter;
class QPixmap;
class QScrollBar;
class QString;
class QMenu;
class QSignalMapper;

class comboBox;
class NotePlayHandle;
class pattern;
class timeLine;
class toolButton;

class PianoRoll : public QWidget, public SerializingObject
{
	Q_OBJECT
public:
	/*! \brief Resets settings to default when e.g. creating a new project */
	void reset();

	void setCurrentPattern( pattern * _new_pattern );

	inline void stopRecording()
	{
		m_recording = false;
	}

	inline bool isRecording() const
	{
		return m_recording;
	}

	inline const pattern * currentPattern() const
	{
		return m_pattern;
	}

	inline bool validPattern() const
	{
		return m_pattern != NULL;
	}

	song::PlayModes desiredPlayModeForAccompany() const;

	int quantization() const;


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );

	inline virtual QString nodeName() const
	{
		return "pianoroll";
	}

	void setPauseIcon( bool pause );


protected:
	virtual void closeEvent( QCloseEvent * _ce );
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void keyReleaseEvent( QKeyEvent * _ke );
	virtual void leaveEvent( QEvent * _e );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re );
	virtual void wheelEvent( QWheelEvent * _we );

	int getKey( int _y ) const;
	static inline void drawNoteRect( QPainter & _p, int _x, int _y,
					int  _width, note * _n );
	void removeSelection();
	void selectAll();
	void getSelectedNotes( NoteVector & _selected_notes );


protected slots:
	void play();
	void record();
	void recordAccompany();
	void stop();

	void startRecordNote( const note & _n );
	void finishRecordNote( const note & _n );

	void horScrolled( int _new_pos );
	void verScrolled( int _new_pos );

	void drawButtonToggled();
	void eraseButtonToggled();
	void selectButtonToggled();
	void detuneButtonToggled();

	void copySelectedNotes();
	void cutSelectedNotes();
	void pasteNotes();
	void deleteSelectedNotes();

	void updatePosition( const MidiTime & _t );
	void updatePositionAccompany( const MidiTime & _t );

	void zoomingChanged();
	void quantizeChanged();

	void updateSemiToneMarkerMenu();

	void changeNoteEditMode( int i );
	void markSemiTone( int i );


signals:
	void currentPatternChanged();
	void semiToneMarkerMenuScaleSetEnabled(bool);
	void semiToneMarkerMenuChordSetEnabled(bool);


private:
	enum editModes
	{
		ModeDraw,
		ModeErase,
		ModeSelect,
		ModeEditDetuning,
	};

	enum actions
	{
		ActionNone,
		ActionMoveNote,
		ActionResizeNote,
		ActionSelectNotes,
		ActionChangeNoteProperty,
		ActionResizeNoteEditArea
	};

	enum noteEditMode
	{
		NoteEditVolume,
		NoteEditPanning,
		NoteEditCount // make sure this one is always last
	};

	enum semiToneMarkerAction
	{
		stmaUnmarkAll,
		stmaMarkCurrentSemiTone,
		stmaMarkCurrentScale,
		stmaMarkCurrentChord,
	};

	enum PianoRollKeyTypes
	{
		PR_WHITE_KEY_SMALL,
		PR_WHITE_KEY_BIG,
		PR_BLACK_KEY
	};

	QVector<QString> m_nemStr; // gui names of each edit mode
	QMenu * m_noteEditMenu; // when you right click below the key area

	QList<int> m_markedSemiTones;
	QMenu * m_semiToneMarkerMenu; // when you right click on the key area

	PianoRoll();
	PianoRoll( const PianoRoll & );
	virtual ~PianoRoll();

	void autoScroll( const MidiTime & _t );

	MidiTime newNoteLen() const;

	void shiftPos(int amount);
	void shiftSemiTone(int amount);
	bool isSelection() const;
	int selectionCount() const;
	void testPlayNote( note * n );
	void testPlayKey( int _key, int _vol, int _pan );
	void pauseTestNotes( bool _pause = true );

	inline int noteEditTop() const;
	inline int keyAreaBottom() const;
	inline int noteEditBottom() const;
	inline int keyAreaTop() const;
	inline int noteEditRight() const;
	inline int noteEditLeft() const;

	void dragNotes( int x, int y, bool alt, bool shift );

	static const int cm_scrollAmtHoriz = 10;
	static const int cm_scrollAmtVert = 1;

	static QPixmap * s_whiteKeyBigPm;
	static QPixmap * s_whiteKeyBigPressedPm;
	static QPixmap * s_whiteKeySmallPm;
	static QPixmap * s_whiteKeySmallPressedPm;
	static QPixmap * s_blackKeyPm;
	static QPixmap * s_blackKeyPressedPm;
	static QPixmap * s_toolDraw;
	static QPixmap * s_toolErase;
	static QPixmap * s_toolSelect;
	static QPixmap * s_toolMove;
	static QPixmap * s_toolOpen;

	static PianoRollKeyTypes prKeyOrder[];


	QWidget * m_toolBar;

	toolButton * m_playButton;
	toolButton * m_recordButton;
	toolButton * m_recordAccompanyButton;
	toolButton * m_stopButton;

	toolButton * m_drawButton;
	toolButton * m_eraseButton;
	toolButton * m_selectButton;
	toolButton * m_detuneButton;

	toolButton * m_cutButton;
	toolButton * m_copyButton;
	toolButton * m_pasteButton;

	comboBox * m_zoomingComboBox;
	comboBox * m_quantizeComboBox;
	comboBox * m_noteLenComboBox;
	comboBox * m_scaleComboBox;
	comboBox * m_chordComboBox;

	ComboBoxModel m_zoomingModel;
	ComboBoxModel m_quantizeModel;
	ComboBoxModel m_noteLenModel;
	ComboBoxModel m_scaleModel;
	ComboBoxModel m_chordModel;



	pattern * m_pattern;
	QScrollBar * m_leftRightScroll;
	QScrollBar * m_topBottomScroll;

	MidiTime m_currentPosition;
	bool m_recording;
	QList<note> m_recordingNotes;

	note * m_currentNote;
	actions m_action;
	noteEditMode m_noteEditMode;

	int m_selectStartTick;
	int m_selectedTick;
	int m_selectStartKey;
	int m_selectedKeys;

	// boundary box around all selected notes when dragging
	int m_moveBoundaryLeft;
	int m_moveBoundaryTop;
	int m_moveBoundaryRight;
	int m_moveBoundaryBottom;

	// remember where the scrolling started when dragging so that
	// we can handle dragging while scrolling with arrow keys
	int m_mouseDownKey;
	int m_mouseDownTick;

	// remember the last x and y of a mouse movement
	int m_lastMouseX;
	int m_lastMouseY;

	// x,y of when the user starts a drag
	int m_moveStartX;
	int m_moveStartY;

	int m_oldNotesEditHeight;
	int m_notesEditHeight;
	int m_ppt;
	int m_totalKeysToScroll;

	// remember these values to use them
	// for the next note that is set
	MidiTime m_lenOfNewNotes;
	volume_t m_lastNoteVolume;
	panning_t m_lastNotePanning;

	int m_startKey;			// first key when drawing
	int m_lastKey;

	editModes m_editMode;
	editModes m_ctrlMode; // mode they were in before they hit ctrl

	bool m_mouseDownLeft; //true if left click is being held down
	bool m_mouseDownRight; //true if right click is being held down

	timeLine * m_timeLine;
	bool m_scrollBack;

	void copy_to_clipboard( const NoteVector & _notes ) const;

	void drawDetuningInfo( QPainter & _p, note * _n, int _x, int _y );
	bool mouseOverNote();
	note * noteUnderMouse();

	// turn a selection rectangle into selected notes
	void computeSelectedNotes( bool shift );
	void clearSelectedNotes();

	friend class engine;


signals:
	void positionChanged( const MidiTime & );

} ;


#endif

