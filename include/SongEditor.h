/*
 * SongEditor.h - declaration of class SongEditor, a window where you can
 *                 setup your songs
 *
 * Copyright (c) 2004-2015 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "TrackContainerView.h"

class QLabel;
class QScrollBar;

class AutomatableSlider;
class ComboBox;
class LcdSpinBox;
class MeterDialog;
class Song;
class TextFloat;
class Timeline;
class ToolButton;

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
	SongEditor( Song * _song );
	virtual ~SongEditor();

	void setPauseIcon( bool pause );


public slots:
	void scrolled( int _new_pos );

protected:
	virtual void closeEvent( QCloseEvent * _ce );

private slots:
	void setHighQuality( bool );

	void play();
	void record();
	void recordAccompany();
	void stop();

	void masterVolumeChanged( int _new_val );
	void masterVolumePressed();
	void masterVolumeMoved( int _new_val );
	void masterVolumeReleased();
	void masterPitchChanged( int _new_val );
	void masterPitchPressed();
	void masterPitchMoved( int _new_val );
	void masterPitchReleased();

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

	QWidget * m_toolBar;

	ToolButton * m_playButton;
	ToolButton * m_recordButton;
	ToolButton * m_recordAccompanyButton;
	ToolButton * m_stopButton;
	LcdSpinBox * m_tempoSpinBox;

	Timeline * m_timeLine;

	MeterDialog * m_timeSigDisplay;
	AutomatableSlider * m_masterVolumeSlider;
	AutomatableSlider * m_masterPitchSlider;

	TextFloat * m_mvsStatus;
	TextFloat * m_mpsStatus;

	ToolButton * m_addBBTrackButton;
	ToolButton * m_addSampleTrackButton;
	ToolButton * m_addAutomationTrackButton;

	ToolButton * m_drawModeButton;
	ToolButton * m_editModeButton;

	ComboBox * m_zoomingComboBox;

	positionLine * m_positionLine;

	bool m_scrollBack;
	bool m_smoothScroll;

} ;



#endif
