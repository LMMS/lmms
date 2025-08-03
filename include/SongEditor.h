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

#ifndef LMMS_GUI_SONG_EDITOR_H
#define LMMS_GUI_SONG_EDITOR_H

#include "Editor.h"
#include "TrackContainerView.h"

class QLabel;
class QScrollBar;

namespace lmms
{

class IntModel;
class Song;
class ComboBoxModel;

namespace gui
{


class ActionGroup;
class AutomatableSlider;
class ComboBox;
class LcdSpinBox;
class MeterDialog;
class PositionLine;
class TextFloat;
class TimeLineWidget;


class SongEditor : public TrackContainerView
{
	Q_OBJECT
public:
	enum class EditMode
	{
		Draw,
		Knife,
		Select
	};

	SongEditor( Song * song );
	~SongEditor() override = default;

	void saveSettings( QDomDocument& doc, QDomElement& element ) override;
	void loadSettings( const QDomElement& element ) override;

	ComboBoxModel* snappingModel() const;
	float getSnapSize() const;
	QString getSnapSizeString() const;

	TimeLineWidget* timeLine;
	PositionLine* positionLine;

public slots:
	void scrolled( int new_pos );
	void selectRegionFromPixels(int xStart, int xEnd);
	void stopSelectRegion();
	void updateRubberband();

	void setEditMode( lmms::gui::SongEditor::EditMode mode );
	void setEditModeDraw();
	void setEditModeKnife();
	void setEditModeSelect();
	void toggleProportionalSnap();

	void updatePosition( const lmms::TimePos & t );
	void updatePositionLine();
	void selectAllClips( bool select );

protected:
	void closeEvent( QCloseEvent * ce ) override;
	void mousePressEvent(QMouseEvent * me) override;
	void mouseMoveEvent(QMouseEvent * me) override;
	void mouseReleaseEvent(QMouseEvent * me) override;

private slots:
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
	bool knifeMode() const override;

	int calculatePixelsPerBar() const;
	int calculateZoomSliderValue(int pixelsPerBar) const;

	int trackIndexFromSelectionPoint(int yPos);
	int indexOfTrackView(const TrackView* tv);

	Song * m_song;

	QScrollBar * m_leftRightScroll;

	void adjustLeftRightScoll(int value);

	LcdSpinBox * m_tempoSpinBox;


	MeterDialog * m_timeSigDisplay;
	AutomatableSlider * m_masterVolumeSlider;
	AutomatableSlider * m_masterPitchSlider;

	TextFloat * m_mvsStatus;
	TextFloat * m_mpsStatus;


	IntModel* m_zoomingModel;
	ComboBoxModel* m_snappingModel;
	bool m_proportionalSnap;

	bool m_scrollBack;
	bool m_smoothScroll;

	EditMode m_mode;
	EditMode m_ctrlMode; // mode they were in before they hit ctrl

	QPoint m_origin;
	QPoint m_scrollPos;
	QPoint m_mousePos;
	int m_rubberBandStartTrackview;
	TimePos m_rubberbandStartTimePos;
	int m_rubberbandPixelsPerBar; //!< pixels per bar when selection starts
	int m_trackHeadWidth;
	bool m_selectRegion;

	friend class SongEditorWindow;

signals:
	void pixelsPerBarChanged(float);
	void proportionalSnapChanged();
} ;




class SongEditorWindow : public Editor
{
	Q_OBJECT
public:
	SongEditorWindow( Song* song );

	QSize sizeHint() const override;

	SongEditor* m_editor;
	void syncEditMode();

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
	QAction* m_addPatternTrackAction;
	QAction* m_addSampleTrackAction;
	QAction* m_addAutomationTrackAction;
	QAction* m_setProportionalSnapAction;

	ActionGroup * m_editModeGroup;
	QAction* m_drawModeAction;
	QAction* m_knifeModeAction;
	QAction* m_selectModeAction;
	QAction* m_crtlAction;

	AutomatableSlider * m_zoomingSlider;
	ComboBox * m_snappingComboBox;
	QLabel* m_snapSizeLabel;

	QAction* m_insertBarAction;
	QAction* m_removeBarAction;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_SONG_EDITOR_H
