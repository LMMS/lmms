/*
 * PianoRoll.h - declaration of class PianoRoll which is a window where you
 *               can set and edit notes in an easy way
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008 Andrew Kelley <superjoe30/at/gmail/dot/com>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <QVector>
#include <QWidget>
#include <QInputDialog>

#include "Editor.h"
#include "ComboBoxModel.h"
#include "SerializingObject.h"
#include "Note.h"
#include "lmms_basics.h"
#include "Song.h"
#include "ToolTip.h"
#include "StepRecorder.h"
#include "StepRecorderWidget.h"

class QPainter;
class QPixmap;
class QScrollBar;
class QString;
class QMenu;

class ComboBox;
class NotePlayHandle;
class Pattern;
class TimeLineWidget;

class PianoRoll : public QWidget
{
	Q_OBJECT
	Q_PROPERTY( QColor barLineColor READ barLineColor WRITE setBarLineColor )
	Q_PROPERTY( QColor beatLineColor READ beatLineColor WRITE setBeatLineColor )
	Q_PROPERTY( QColor lineColor READ lineColor WRITE setLineColor )
	Q_PROPERTY( QColor noteModeColor READ noteModeColor WRITE setNoteModeColor )
	Q_PROPERTY( QColor noteColor READ noteColor WRITE setNoteColor )
	Q_PROPERTY( QColor ghostNoteColor READ ghostNoteColor WRITE setGhostNoteColor )
	Q_PROPERTY( QColor noteTextColor READ noteTextColor WRITE setNoteTextColor )
	Q_PROPERTY( QColor ghostNoteTextColor READ ghostNoteTextColor WRITE setGhostNoteTextColor )
	Q_PROPERTY( QColor barColor READ barColor WRITE setBarColor )
	Q_PROPERTY( QColor selectedNoteColor READ selectedNoteColor WRITE setSelectedNoteColor )
	Q_PROPERTY( QColor textColor READ textColor WRITE setTextColor )
	Q_PROPERTY( QColor textColorLight READ textColorLight WRITE setTextColorLight )
	Q_PROPERTY( QColor textShadow READ textShadow WRITE setTextShadow )
	Q_PROPERTY( QColor markedSemitoneColor READ markedSemitoneColor WRITE setMarkedSemitoneColor )
	Q_PROPERTY( int noteOpacity READ noteOpacity WRITE setNoteOpacity )
	Q_PROPERTY( bool noteBorders READ noteBorders WRITE setNoteBorders )
	Q_PROPERTY( int ghostNoteOpacity READ ghostNoteOpacity WRITE setGhostNoteOpacity )
	Q_PROPERTY( bool ghostNoteBorders READ ghostNoteBorders WRITE setGhostNoteBorders )
	Q_PROPERTY( QColor backgroundShade READ backgroundShade WRITE setBackgroundShade )
public:
	enum EditModes
	{
		ModeDraw,
		ModeErase,
		ModeSelect,
		ModeEditDetuning,
	};

	/*! \brief Resets settings to default when e.g. creating a new project */
	void reset();

	// functions to display the hover-text labeling a note's volume/panning
	void showTextFloat(const QString &text, const QPoint &pos, int timeout=-1);
	void showVolTextFloat(volume_t vol, const QPoint &pos, int timeout=-1);
	void showPanTextFloat(panning_t pan, const QPoint &pos, int timeout=-1);

	void setCurrentPattern( Pattern* newPattern );
	void setGhostPattern( Pattern* newPattern );
	void loadGhostNotes( const QDomElement & de );
	void loadMarkedSemiTones(const QDomElement & de);

	inline void stopRecording()
	{
		m_recording = false;
	}

	inline bool isRecording() const
	{
		return m_recording;
	}

	inline bool isStepRecording() const
	{
		return m_stepRecorder.isRecording();
	}

	const Pattern* currentPattern() const
	{
		return m_pattern;
	}

	bool hasValidPattern() const
	{
		return m_pattern != NULL;
	}

	Song::PlayModes desiredPlayModeForAccompany() const;

	int quantization() const;

	// qproperty access functions
	QColor barLineColor() const;
	void setBarLineColor( const QColor & c );
	QColor beatLineColor() const;
	void setBeatLineColor( const QColor & c );
	QColor lineColor() const;
	void setLineColor( const QColor & c );
	QColor noteModeColor() const;
	void setNoteModeColor( const QColor & c );
	QColor noteColor() const;
	void setNoteColor( const QColor & c );
	QColor noteTextColor() const;
	void setNoteTextColor( const QColor & c );
	QColor barColor() const;
	void setBarColor( const QColor & c );
	QColor selectedNoteColor() const;
	void setSelectedNoteColor( const QColor & c );
	QColor textColor() const;
	void setTextColor( const QColor & c );
	QColor textColorLight() const;
	void setTextColorLight( const QColor & c );
	QColor textShadow() const;
	void setTextShadow( const QColor & c );
	QColor markedSemitoneColor() const;
	void setMarkedSemitoneColor( const QColor & c );
	int noteOpacity() const;
	void setNoteOpacity( const int i );
	bool noteBorders() const;
	void setNoteBorders( const bool b );
	QColor ghostNoteColor() const;
	void setGhostNoteColor( const QColor & c );
	QColor ghostNoteTextColor() const;
	void setGhostNoteTextColor( const QColor & c );
	int ghostNoteOpacity() const;
	void setGhostNoteOpacity( const int i );
	bool ghostNoteBorders() const;
	void setGhostNoteBorders( const bool b );
	QColor backgroundShade() const;
	void setBackgroundShade( const QColor & c );


protected:
	void keyPressEvent( QKeyEvent * ke ) override;
	void keyReleaseEvent( QKeyEvent * ke ) override;
	void leaveEvent( QEvent * e ) override;
	void mousePressEvent( QMouseEvent * me ) override;
	void mouseDoubleClickEvent( QMouseEvent * me ) override;
	void mouseReleaseEvent( QMouseEvent * me ) override;
	void mouseMoveEvent( QMouseEvent * me ) override;
	void paintEvent( QPaintEvent * pe ) override;
	void resizeEvent( QResizeEvent * re ) override;
	void wheelEvent( QWheelEvent * we ) override;
	void focusOutEvent( QFocusEvent * ) override;

	int getKey( int y ) const;
	static void drawNoteRect( QPainter & p, int x, int y,
					int  width, const Note * n, const QColor & noteCol, const QColor & noteTextColor,
					const QColor & selCol, const int noteOpc, const bool borderless, bool drawNoteName );
	void removeSelection();
	void selectAll();
	NoteVector getSelectedNotes();
	void selectNotesOnKey();
	int xCoordOfTick( int tick );

	// for entering values with dblclick in the vol/pan bars
	void enterValue( NoteVector* nv );

protected slots:
	void play();
	void record();
	void recordAccompany();
	bool toggleStepRecording();
	void stop();

	void startRecordNote( const Note & n );
	void finishRecordNote( const Note & n );

	void horScrolled( int new_pos );
	void verScrolled( int new_pos );

	void setEditMode(int mode);

	void copySelectedNotes();
	void cutSelectedNotes();
	void pasteNotes();
	void deleteSelectedNotes();

	void updatePosition(const MidiTime & t );
	void updatePositionAccompany(const MidiTime & t );
	void updatePositionStepRecording(const MidiTime & t );

	void zoomingChanged();
	void quantizeChanged();
	void noteLengthChanged();
	void quantizeNotes();

	void updateSemiToneMarkerMenu();

	void changeNoteEditMode( int i );
	void markSemiTone( int i );

	void hidePattern( Pattern* pattern );

	void selectRegionFromPixels( int xStart, int xEnd );

	void clearGhostPattern();


signals:
	void currentPatternChanged();
	void ghostPatternSet(bool);
	void semiToneMarkerMenuScaleSetEnabled(bool);
	void semiToneMarkerMenuChordSetEnabled(bool);


private:
	enum Actions
	{
		ActionNone,
		ActionMoveNote,
		ActionResizeNote,
		ActionSelectNotes,
		ActionChangeNoteProperty,
		ActionResizeNoteEditArea
	};

	enum NoteEditMode
	{
		NoteEditVolume,
		NoteEditPanning,
		NoteEditCount // make sure this one is always last
	};

	enum SemiToneMarkerAction
	{
		stmaUnmarkAll,
		stmaMarkCurrentSemiTone,
		stmaMarkAllOctaveSemiTones,
		stmaMarkCurrentScale,
		stmaMarkCurrentChord,
		stmaCopyAllNotesOnKey
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

	void autoScroll(const MidiTime & t );

	MidiTime newNoteLen() const;

	void shiftPos(int amount);
	void shiftSemiTone(int amount);
	bool isSelection() const;
	int selectionCount() const;
	void testPlayNote( Note * n );
	void testPlayKey( int _key, int _vol, int _pan );
	void pauseTestNotes(bool pause = true );
	void playChordNotes(int key, int velocity=-1);
	void pauseChordNotes(int key);

	QList<int> getAllOctavesForKey( int keyToMirror ) const;

	int noteEditTop() const;
	int keyAreaBottom() const;
	int noteEditBottom() const;
	int keyAreaTop() const;
	int noteEditRight() const;
	int noteEditLeft() const;

	void dragNotes( int x, int y, bool alt, bool shift, bool ctrl );

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

	static TextFloat * s_textFloat;

	ComboBoxModel m_zoomingModel;
	ComboBoxModel m_quantizeModel;
	ComboBoxModel m_noteLenModel;
	ComboBoxModel m_scaleModel;
	ComboBoxModel m_chordModel;

	static const QVector<double> m_zoomLevels;

	Pattern* m_pattern;
	NoteVector m_ghostNotes;

	inline const NoteVector & ghostNotes() const
	{
		return m_ghostNotes;
	}

	QScrollBar * m_leftRightScroll;
	QScrollBar * m_topBottomScroll;

	MidiTime m_currentPosition;
	bool m_recording;
	QList<Note> m_recordingNotes;

	Note * m_currentNote;
	Actions m_action;
	NoteEditMode m_noteEditMode;

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
	int m_ppb;  // pixels per bar
	int m_totalKeysToScroll;

	// remember these values to use them
	// for the next note that is set
	MidiTime m_lenOfNewNotes;
	volume_t m_lastNoteVolume;
	panning_t m_lastNotePanning;

	int m_startKey; // first key when drawing
	int m_lastKey;

	EditModes m_editMode;
	EditModes m_ctrlMode; // mode they were in before they hit ctrl

	bool m_mouseDownRight; //true if right click is being held down

	TimeLineWidget * m_timeLine;
	bool m_scrollBack;

	void copyToClipboard(const NoteVector & notes ) const;

	void drawDetuningInfo( QPainter & _p, const Note * _n, int _x, int _y ) const;
	bool mouseOverNote();
	Note * noteUnderMouse();

	// turn a selection rectangle into selected notes
	void computeSelectedNotes( bool shift );
	void clearSelectedNotes();

	// did we start a mouseclick with shift pressed
	bool m_startedWithShift;

	friend class PianoRollWindow;

	StepRecorderWidget m_stepRecorderWidget;
	StepRecorder m_stepRecorder;

	// qproperty fields
	QColor m_barLineColor;
	QColor m_beatLineColor;
	QColor m_lineColor;
	QColor m_noteModeColor;
	QColor m_noteColor;
	QColor m_noteTextColor;
	QColor m_ghostNoteColor;
	QColor m_ghostNoteTextColor;
	QColor m_barColor;
	QColor m_selectedNoteColor;
	QColor m_textColor;
	QColor m_textColorLight;
	QColor m_textShadow;
	QColor m_markedSemitoneColor;
	int m_noteOpacity;
	int m_ghostNoteOpacity;
	bool m_noteBorders;
	bool m_ghostNoteBorders;
	QColor m_backgroundShade;

signals:
	void positionChanged( const MidiTime & );
} ;




class PianoRollWindow : public Editor, SerializingObject
{
	Q_OBJECT
public:
	PianoRollWindow();

	const Pattern* currentPattern() const;
	void setCurrentPattern( Pattern* pattern );
	void setGhostPattern( Pattern* pattern );

	int quantization() const;

	void play() override;
	void stop() override;
	void record() override;
	void recordAccompany() override;
	void toggleStepRecording() override;
	void stopRecording();

	bool isRecording() const;

	/*! \brief Resets settings to default when e.g. creating a new project */
	void reset();

	using SerializingObject::saveState;
	using SerializingObject::restoreState;
	void saveSettings(QDomDocument & doc, QDomElement & de ) override;
	void loadSettings( const QDomElement & de ) override;

	inline QString nodeName() const override
	{
		return "pianoroll";
	}

	QSize sizeHint() const override;

signals:
	void currentPatternChanged();


private slots:
	void updateAfterPatternChange();
	void ghostPatternSet( bool state );

private:
	void patternRenamed();
	void focusInEvent(QFocusEvent * event) override;
	void stopStepRecording();
	void updateStepRecordingIcon();

	PianoRoll* m_editor;

	ComboBox * m_zoomingComboBox;
	ComboBox * m_quantizeComboBox;
	ComboBox * m_noteLenComboBox;
	ComboBox * m_scaleComboBox;
	ComboBox * m_chordComboBox;
	QPushButton * m_clearGhostButton;

};


#endif
