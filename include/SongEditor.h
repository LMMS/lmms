/*
 * SongEditor.h - declaration of class SongEditor, a window where you can
 *                 setup your songs
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef SONG_EDITOR_H
#define SONG_EDITOR_H

#include "Editor.h"
#include "TrackContainerView.h"

class QLabel;
class QScrollBar;

class AutomatableSlider;
class ComboBox;
class ComboBoxModel;
class LcdSpinBox;
class MeterDialog;
class Song;
class TextFloat;
class Timeline;

class positionLine : public QWidget
{
public:
	positionLine( QWidget * _parent );

private:
	virtual void paintEvent( QPaintEvent * _pe );

} ;


class SongEditor : public TrackContainerView
{
	Q_OBJECT
public:
	enum EditMode
	{
		DrawMode,
		SelectMode
	};

	SongEditor( Song * _song );
	~SongEditor();

public slots:
	void scrolled( int _new_pos );

	void setEditMode(EditMode mode);
	void setEditModeDraw();
	void setEditModeSelect();


private slots:
	void setHighQuality( bool );

	void setMasterVolume( int _new_val );
	void showMasterVolumeFloat();
	void updateMasterVolumeFloat( int _new_val );
	void hideMasterVolumeFloat();

	void setMasterPitch( int _new_val );
	void showMasterPitchFloat();
	void updateMasterPitchFloat( int _new_val );
	void hideMasterPitchFloat();

	void updateScrollBar( int );
	void updatePosition( const MidiTime & _t );

	void zoomingChanged();

	void adjustUiAfterProjectLoad();


private:
	virtual void keyPressEvent( QKeyEvent * _ke );
	virtual void wheelEvent( QWheelEvent * _we );

	virtual bool allowRubberband() const;


	Song * m_song;

	QScrollBar * m_leftRightScroll;

	LcdSpinBox * m_tempoSpinBox;

	Timeline * m_timeLine;

	MeterDialog * m_timeSigDisplay;
	AutomatableSlider * m_masterVolumeSlider;
	AutomatableSlider * m_masterPitchSlider;

	TextFloat * m_mvsStatus;
	TextFloat * m_mpsStatus;

	positionLine * m_positionLine;

	ComboBoxModel* m_zoomingModel;

	bool m_scrollBack;
	bool m_smoothScroll;

	EditMode m_mode;

	friend class SongEditorWindow;

} ;

class SongEditorWindow : public Editor
{
	Q_OBJECT
public:
	SongEditorWindow(Song* song);

	QSize sizeHint() const;

	SongEditor* m_editor;

protected slots:
	void play();
	void record();
	void recordAccompany();
	void stop();

private:
	QAction* m_addBBTrackAction;
	QAction* m_addSampleTrackAction;
	QAction* m_addAutomationTrackAction;

	QAction* m_drawModeAction;
	QAction* m_selectModeAction;

	ComboBox * m_zoomingComboBox;
};

#endif
