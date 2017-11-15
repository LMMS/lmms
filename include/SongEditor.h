/*
 * SongEditor.h - declaration of class SongEditor, a window where you can
 *                 setup your songs
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef SONG_EDITOR_H
#define SONG_EDITOR_H

#include <QVector>

#include "ActionGroup.h"
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
class TimeLineWidget;

class positionLine : public QWidget
{
public:
	positionLine( QWidget * parent );

private:
	virtual void paintEvent( QPaintEvent * pe );

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

	SongEditor( Song * song );
	~SongEditor();

	void saveSettings( QDomDocument& doc, QDomElement& element );
	void loadSettings( const QDomElement& element );

	ComboBoxModel *zoomingModel() const;

public slots:
	void scrolled( int new_pos );

	void setEditMode( EditMode mode );
	void setEditModeDraw();
	void setEditModeSelect();

	void updatePosition( const MidiTime & t );
	void updatePositionLine();
	void selectAllTcos( bool select );

protected:
	virtual void closeEvent( QCloseEvent * ce );

private slots:
	void setHighQuality( bool );

	void setMasterVolume( int new_val );
	void showMasterVolumeFloat();
	void updateMasterVolumeFloat( int new_val );
	void hideMasterVolumeFloat();

	void setMasterPitch( int new_val );
	void showMasterPitchFloat();
	void updateMasterPitchFloat( int new_val );
	void hideMasterPitchFloat();

	void updateScrollBar(int len);

	void zoomingChanged();

private:
	virtual void keyPressEvent( QKeyEvent * ke );
	virtual void wheelEvent( QWheelEvent * we );

	virtual bool allowRubberband() const;


	Song * m_song;

	QScrollBar * m_leftRightScroll;

	LcdSpinBox * m_tempoSpinBox;

	TimeLineWidget * m_timeLine;

	MeterDialog * m_timeSigDisplay;
	AutomatableSlider * m_masterVolumeSlider;
	AutomatableSlider * m_masterPitchSlider;

	TextFloat * m_mvsStatus;
	TextFloat * m_mpsStatus;

	positionLine * m_positionLine;

	ComboBoxModel* m_zoomingModel;

	static const QVector<double> m_zoomLevels;

	bool m_scrollBack;
	bool m_smoothScroll;

	EditMode m_mode;
	EditMode m_ctrlMode; // mode they were in before they hit ctrl

	friend class SongEditorWindow;

} ;




class SongEditorWindow : public Editor
{
	Q_OBJECT
public:
	SongEditorWindow( Song* song );

	QSize sizeHint() const;

	SongEditor* m_editor;

protected:
	virtual void resizeEvent( QResizeEvent * event );

protected slots:
	void play();
	void record();
	void recordAccompany();
	void stop();

	void lostFocus();
	void adjustUiAfterProjectLoad();

signals:
	void playTriggered();
	void resized();

private:
	virtual void keyPressEvent( QKeyEvent * ke );
	virtual void keyReleaseEvent( QKeyEvent * ke );

	QAction* m_addBBTrackAction;
	QAction* m_addSampleTrackAction;
	QAction* m_addAutomationTrackAction;

	ActionGroup * m_editModeGroup;
	QAction* m_drawModeAction;
	QAction* m_selectModeAction;
	QAction* m_crtlAction;

	ComboBox * m_zoomingComboBox;
};

#endif
