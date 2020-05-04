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
	void paintEvent( QPaintEvent * pe ) override;

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

	void saveSettings( QDomDocument& doc, QDomElement& element ) override;
	void loadSettings( const QDomElement& element ) override;

	ComboBoxModel *zoomingModel() const;
	ComboBoxModel *snappingModel() const;
	float getSnapSize() const;
	QString getSnapSizeString() const;

public slots:
	void scrolled( int new_pos );
	void selectRegionFromPixels(int xStart, int xEnd);
	void stopSelectRegion();
	void updateRubberband();

	void setEditMode( EditMode mode );
	void setEditModeDraw();
	void setEditModeSelect();
	void toggleProportionalSnap();

	void updatePosition( const MidiTime & t );
	void updatePositionLine();
	void selectAllTcos( bool select );

protected:
	void closeEvent( QCloseEvent * ce ) override;
	void mousePressEvent(QMouseEvent * me) override;
	void mouseMoveEvent(QMouseEvent * me) override;
	void mouseReleaseEvent(QMouseEvent * me) override;

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
	void keyPressEvent( QKeyEvent * ke ) override;
	void wheelEvent( QWheelEvent * we ) override;

	bool allowRubberband() const override;

	int trackIndexFromSelectionPoint(int yPos);
	int indexOfTrackView(const TrackView* tv);


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
	ComboBoxModel* m_snappingModel;
	bool m_proportionalSnap;

	static const QVector<double> m_zoomLevels;

	bool m_scrollBack;
	bool m_smoothScroll;

	EditMode m_mode;
	EditMode m_ctrlMode; // mode they were in before they hit ctrl

	QPoint m_origin;
	QPoint m_scrollPos;
	QPoint m_mousePos;
	int m_rubberBandStartTrackview;
	MidiTime m_rubberbandStartMidipos;
	int m_currentZoomingValue;
	int m_trackHeadWidth;
	bool m_selectRegion;

	friend class SongEditorWindow;
} ;




class SongEditorWindow : public Editor
{
	Q_OBJECT
public:
	SongEditorWindow( Song* song );

	QSize sizeHint() const override;

	SongEditor* m_editor;

protected:
	void resizeEvent( QResizeEvent * event ) override;
	void changeEvent( QEvent * ) override;

protected slots:
	void play() override;
	void record() override;
	void recordAccompany() override;
	void stop() override;

	void lostFocus();
	void adjustUiAfterProjectLoad();

	void updateSnapLabel();

signals:
	void playTriggered();
	void resized();

private:
	void keyPressEvent( QKeyEvent * ke ) override;
	void keyReleaseEvent( QKeyEvent * ke ) override;

	QAction* m_addBBTrackAction;
	QAction* m_addSampleTrackAction;
	QAction* m_addAutomationTrackAction;
	QAction* m_setProportionalSnapAction;

	ActionGroup * m_editModeGroup;
	QAction* m_drawModeAction;
	QAction* m_selectModeAction;
	QAction* m_crtlAction;

	ComboBox * m_zoomingComboBox;
	ComboBox * m_snappingComboBox;
	QLabel* m_snapSizeLabel;
};

#endif
