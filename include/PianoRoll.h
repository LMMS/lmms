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

#ifndef LMMS_GUI_PIANO_ROLL_H
#define LMMS_GUI_PIANO_ROLL_H

#include <QWidget>

#include <vector>

#include "Editor.h"
#include "ComboBoxModel.h"
#include "SerializingObject.h"
#include "Note.h"
#include "LmmsTypes.h"
#include "Song.h"
#include "StepRecorder.h"
#include "StepRecorderWidget.h"

class QPushButton;
class QScrollBar;
class QMenu;
class QToolButton;

namespace lmms
{


class MidiClip;


namespace gui
{

class ComboBox;
class PositionLine;
class SimpleTextFloat;
class TimeLineWidget;


class PianoRoll : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor barLineColor MEMBER m_barLineColor)
	Q_PROPERTY(QColor beatLineColor MEMBER m_beatLineColor)
	Q_PROPERTY(QColor lineColor MEMBER m_lineColor)
	Q_PROPERTY(QColor noteModeColor MEMBER m_noteModeColor)
	Q_PROPERTY(QColor noteColor MEMBER m_noteColor)
	Q_PROPERTY(QColor stepNoteColor MEMBER m_stepNoteColor)
	Q_PROPERTY(QColor currentStepNoteColor MEMBER m_currentStepNoteColor)
	Q_PROPERTY(QColor ghostNoteColor MEMBER m_ghostNoteColor)
	Q_PROPERTY(QColor noteTextColor MEMBER m_noteTextColor)
	Q_PROPERTY(QColor ghostNoteTextColor MEMBER m_ghostNoteTextColor)
	Q_PROPERTY(QColor barColor MEMBER m_barColor)
	Q_PROPERTY(QColor selectedNoteColor MEMBER m_selectedNoteColor)
	Q_PROPERTY(QColor textColor MEMBER m_textColor)
	Q_PROPERTY(QColor textColorLight MEMBER m_textColorLight)
	Q_PROPERTY(QColor textShadow MEMBER m_textShadow)
	Q_PROPERTY(QColor markedSemitoneColor MEMBER m_markedSemitoneColor)
	Q_PROPERTY(QColor knifeCutLine MEMBER m_knifeCutLineColor)
	Q_PROPERTY(int noteOpacity MEMBER m_noteOpacity)
	Q_PROPERTY(bool noteBorders MEMBER m_noteBorders)
	Q_PROPERTY(int ghostNoteOpacity MEMBER m_ghostNoteOpacity)
	Q_PROPERTY(bool ghostNoteBorders MEMBER m_ghostNoteBorders)
	Q_PROPERTY(QColor backgroundShade MEMBER m_backgroundShade)
	Q_PROPERTY(QColor outOfBoundsShade MEMBER m_outOfBoundsShade)

	/* white key properties */
	Q_PROPERTY(int whiteKeyWidth MEMBER m_whiteKeyWidth)
	Q_PROPERTY(QColor whiteKeyInactiveTextColor MEMBER m_whiteKeyInactiveTextColor)
	Q_PROPERTY(QColor whiteKeyInactiveTextShadow MEMBER m_whiteKeyInactiveTextShadow)
	Q_PROPERTY(QBrush whiteKeyInactiveBackground MEMBER m_whiteKeyInactiveBackground)
	Q_PROPERTY(QColor whiteKeyActiveTextColor MEMBER m_whiteKeyActiveTextColor)
	Q_PROPERTY(QColor whiteKeyActiveTextShadow MEMBER m_whiteKeyActiveTextShadow)
	Q_PROPERTY(QBrush whiteKeyActiveBackground MEMBER m_whiteKeyActiveBackground)
	Q_PROPERTY(QBrush whiteKeyDisabledBackground MEMBER m_whiteKeyDisabledBackground)
	/* black key properties */
	Q_PROPERTY(int blackKeyWidth MEMBER m_blackKeyWidth)
	Q_PROPERTY(QBrush blackKeyInactiveBackground MEMBER m_blackKeyInactiveBackground)
	Q_PROPERTY(QBrush blackKeyActiveBackground MEMBER m_blackKeyActiveBackground)
	Q_PROPERTY(QBrush blackKeyDisabledBackground MEMBER m_blackKeyDisabledBackground)
public:
	enum class EditMode
	{
		Draw,
		Erase,
		Select,
		Detuning,
		Knife,
		Strum
	};

	/*! \brief Resets settings to default when e.g. creating a new project */
	void reset();

	// functions to display the hover-text labeling a note's volume/panning
	void showTextFloat(const QString &text, const QPoint &pos, int timeout=-1);
	void showVolTextFloat(volume_t vol, const QPoint &pos, int timeout=-1);
	void showPanTextFloat(panning_t pan, const QPoint &pos, int timeout=-1);

	void setCurrentMidiClip( MidiClip* newMidiClip );
	void setGhostMidiClip( MidiClip* newMidiClip );
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

	const MidiClip* currentMidiClip() const
	{
		return m_midiClip;
	}

	bool hasValidMidiClip() const
	{
		return m_midiClip != nullptr;
	}
	
	int trackOctaveSize() const;

	Song::PlayMode desiredPlayModeForAccompany() const;

	int quantization() const;

protected:
	enum class QuantizeAction
	{
		Both,
		Pos,
		Length
	};

	enum class SemiToneMarkerAction
	{
		UnmarkAll,
		MarkCurrentSemiTone,
		MarkAllOctaveSemiTones,
		MarkCurrentScale,
		MarkCurrentChord,
		CopyAllNotesOnKey
	};

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
	void focusInEvent( QFocusEvent * ) override;

	void drawNoteRect( QPainter & p, int x, int y,
					int  width, const Note * n, const QColor & noteCol, const QColor & noteTextColor,
					const QColor & selCol, const int noteOpc, const bool borderless, bool drawNoteName );
	void removeSelection();
	void selectAll();
	NoteVector getSelectedNotes() const;
	void selectNotesOnKey();

	// for entering values with dblclick in the vol/pan bars
	void enterValue( NoteVector* nv );

	void updateYScroll();

protected slots:
	void play();
	void record();
	void recordAccompany();
	bool toggleStepRecording();
	void stop();

	void startRecordNote( const lmms::Note & n );
	void finishRecordNote( const lmms::Note & n );

	void horScrolled( int new_pos );
	void verScrolled( int new_pos );

	void setEditMode(int mode);

	void copySelectedNotes();
	void cutSelectedNotes();
	void pasteNotes();
	bool deleteSelectedNotes();

	void updatePosition(const lmms::TimePos & t );
	void updatePositionAccompany(const lmms::TimePos & t );
	void updatePositionStepRecording(const lmms::TimePos & t );

	void zoomingChanged();
	void zoomingYChanged();
	void quantizeChanged();
	void noteLengthChanged();
	void keyChanged();
	void quantizeNotes(QuantizeAction mode = QuantizeAction::Both);

	void updateSemiToneMarkerMenu();

	void changeNoteEditMode( int i );
	void markSemiTone(SemiToneMarkerAction i, bool fromMenu = true);

	void hideMidiClip( lmms::MidiClip* clip );

	void selectRegionFromPixels( int xStart, int xEnd );

	void clearGhostClip();
	void glueNotes();
	void fitNoteLengths(bool fill);
	void reverseNotes();
	void constrainNoteLengths(bool constrainMax);

	void changeSnapMode();


signals:
	void currentMidiClipChanged();
	void ghostClipSet(bool);
	void semiToneMarkerMenuScaleSetEnabled(bool);
	void semiToneMarkerMenuChordSetEnabled(bool);


private:
	enum class Action
	{
		None,
		MoveNote,
		ResizeNote,
		SelectNotes,
		ChangeNoteProperty,
		ResizeNoteEditArea,
		Knife,
		Strum
	};

	enum class NoteEditMode
	{
		Volume,
		Panning,
		Count // make sure this one is always last
	};

	enum class KeyType
	{
		WhiteSmall,
		WhiteBig,
		Black
	};

	enum class GridMode
	{
		Nudge,
		Snap
	//	Free
	};

	PositionLine * m_positionLine;

	std::vector<QString> m_nemStr; // gui names of each edit mode
	QMenu * m_noteEditMenu; // when you right click below the key area

	QList<int> m_markedSemiTones;
	QMenu * m_semiToneMarkerMenu; // when you right click on the key area
	int m_pianoKeySelected;

	PianoRoll();
	PianoRoll( const PianoRoll & );
	~PianoRoll() override = default;

	void autoScroll(const TimePos & t );

	TimePos newNoteLen() const;

	void shiftPos(int amount);
	void shiftPos(const NoteVector& notes, int amount);
	void shiftSemiTone(int amount);
	void shiftSemiTone(const NoteVector& notes, int amount);
	bool isSelection() const;
	int selectionCount() const;
	void testPlayNote( Note * n );
	void testPlayKey( int _key, int _vol, int _pan );
	void pauseTestNotes(bool pause = true );
	void playChordNotes(int key, int velocity=-1);
	void pauseChordNotes(int key);

	void setKnifeAction();
	void cancelKnifeAction();

	void setStrumAction();
	void cancelStrumAction();

	void updateScrollbars();
	void updatePositionLineHeight();

	QList<int> getAllOctavesForKey( int keyToMirror ) const;

	int noteEditTop() const;
	int keyAreaBottom() const;
	int noteEditBottom() const;
	int keyAreaTop() const;
	int noteEditRight() const;
	int noteEditLeft() const;

	int getKey(int y) const;
	int yCoordOfKey(int key) const;

	void dragNotes(int x, int y, bool alt, bool shift, bool ctrl);

	static const int cm_scrollAmtHoriz = 10;
	static const int cm_scrollAmtVert = 1;

	QPixmap m_toolDraw = embed::getIconPixmap("edit_draw");
	QPixmap m_toolErase = embed::getIconPixmap("edit_erase");
	QPixmap m_toolSelect = embed::getIconPixmap("edit_select");
	QPixmap m_toolMove = embed::getIconPixmap("edit_move");
	QPixmap m_toolOpen = embed::getIconPixmap("automation");
	QPixmap m_toolKnife = embed::getIconPixmap("edit_knife");
	QPixmap m_toolStrum = embed::getIconPixmap("arp_free");

	static std::array<KeyType, 12> prKeyOrder;

	static SimpleTextFloat * s_textFloat;

	ComboBoxModel m_zoomingModel;
	ComboBoxModel m_zoomingYModel;
	ComboBoxModel m_quantizeModel;
	ComboBoxModel m_noteLenModel;
	ComboBoxModel m_keyModel;
	ComboBoxModel m_scaleModel;
	ComboBoxModel m_chordModel;
	ComboBoxModel m_snapModel;

	static const std::vector<float> m_zoomLevels;
	static const std::vector<float> m_zoomYLevels;

	MidiClip* m_midiClip;
	NoteVector m_ghostNotes;

	inline const NoteVector & ghostNotes() const
	{
		return m_ghostNotes;
	}

	QScrollBar * m_leftRightScroll;
	QScrollBar * m_topBottomScroll;

	void adjustLeftRightScoll(int value);

	TimePos m_currentPosition;
	bool m_recording;
	bool m_doAutoQuantization{false};
	QList<Note> m_recordingNotes;

	Note * m_currentNote;
	Action m_action;
	NoteEditMode m_noteEditMode;
	GridMode m_gridMode;

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

	int m_notesEditHeight;
	int m_userSetNotesEditHeight;
	int m_ppb;  // pixels per bar
	int m_totalKeysToScroll;
	int m_pianoKeysVisible;

	int m_keyLineHeight;
	int m_whiteKeySmallHeight;
	int m_whiteKeyBigHeight;
	int m_blackKeyHeight;

	// remember these values to use them
	// for the next note that is set
	TimePos m_lenOfNewNotes;
	volume_t m_lastNoteVolume;
	panning_t m_lastNotePanning;

	//When resizing several notes, we want to calculate a common minimum length
	TimePos m_minResizeLen;

	int m_startKey; // first key when drawing
	int m_lastKey;

	EditMode m_editMode;
	EditMode m_ctrlMode; // mode they were in before they hit ctrl
	EditMode m_knifeMode; // mode they where in before entering knife mode
	EditMode m_strumMode; //< mode they where in before entering strum mode

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

	// Variables that hold the start and end position for the knife line
	TimePos m_knifeStartTickPos;
	int m_knifeStartKey;
	TimePos m_knifeEndTickPos;
	int m_knifeEndKey;
	bool m_knifeDown;

	void updateKnifePos(QMouseEvent* me, bool initial);

	//! Stores the chords for the strum tool
	std::vector<NoteVector> m_selectedChords;
	//! Computes which notes belong to which chords from the selection
	void setupSelectedChords();

	TimePos m_strumStartTime;
	TimePos m_strumCurrentTime;
	int m_strumStartVertical = 0;
	int m_strumCurrentVertical = 0;
	float m_strumHeightRatio = 0.0f;
	bool m_strumEnabled = false;
	//! Handles updating all of the note positions when performing a strum
	void updateStrumPos(QMouseEvent* me, bool initial, bool warp);


	friend class PianoRollWindow;

	StepRecorderWidget m_stepRecorderWidget;
	StepRecorder m_stepRecorder;

	// qproperty fields
	QColor m_barLineColor;
	QColor m_beatLineColor;
	QColor m_lineColor;
	QColor m_noteModeColor;
	QColor m_noteColor;
	QColor m_stepNoteColor;
	QColor m_currentStepNoteColor;
	QColor m_noteTextColor;
	QColor m_ghostNoteColor;
	QColor m_ghostNoteTextColor;
	QColor m_barColor;
	QColor m_selectedNoteColor;
	QColor m_textColor;
	QColor m_textColorLight;
	QColor m_textShadow;
	QColor m_markedSemitoneColor;
	QColor m_knifeCutLineColor;
	int m_noteOpacity;
	int m_ghostNoteOpacity;
	bool m_noteBorders;
	bool m_ghostNoteBorders;
	QColor m_backgroundShade;
	QColor m_outOfBoundsShade;
	/* white key properties */
	int m_whiteKeyWidth;
	QColor m_whiteKeyActiveTextColor;
	QColor m_whiteKeyActiveTextShadow;
	QBrush m_whiteKeyActiveBackground;
	QColor m_whiteKeyInactiveTextColor;
	QColor m_whiteKeyInactiveTextShadow;
	QBrush m_whiteKeyInactiveBackground;
	QBrush m_whiteKeyDisabledBackground;
	/* black key properties */
	int m_blackKeyWidth;
	QBrush m_blackKeyActiveBackground;
	QBrush m_blackKeyInactiveBackground;
	QBrush m_blackKeyDisabledBackground;

signals:
	void positionChanged( const lmms::TimePos & );
} ;




class PianoRollWindow : public Editor, SerializingObject
{
	Q_OBJECT
public:
	PianoRollWindow();

	const MidiClip* currentMidiClip() const;
	void setCurrentMidiClip( MidiClip* clip );
	void setGhostMidiClip( MidiClip* clip );

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
	bool hasFocus() const;

signals:
	void currentMidiClipChanged();


private slots:
	void updateAfterMidiClipChange();
	void ghostClipSet( bool state );
	void exportMidiClip();
	void importMidiClip();

private:
	void clipRenamed();
	void focusInEvent(QFocusEvent * event) override;
	void stopStepRecording();
	void updateStepRecordingIcon();

	PianoRoll* m_editor;

	QToolButton* m_fileToolsButton;
	ComboBox * m_zoomingComboBox;
	ComboBox * m_zoomingYComboBox;
	ComboBox * m_quantizeComboBox;
	ComboBox * m_noteLenComboBox;
	ComboBox * m_keyComboBox;
	ComboBox * m_scaleComboBox;
	ComboBox * m_chordComboBox;
	ComboBox* m_snapComboBox;
	QPushButton * m_clearGhostButton;

};


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PIANO_ROLL_H
