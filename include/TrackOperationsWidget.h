/*
 * TrackOperationsWidget.h - declaration of TrackOperationsWidget class
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

#ifndef LMMS_GUI_TRACK_OPERATIONS_WIDGET_H
#define LMMS_GUI_TRACK_OPERATIONS_WIDGET_H

#include <QWidget>

class QPushButton;

namespace lmms::gui
{

class AutomatableButton;
class TrackGrip;
class TrackView;

//! @brief The grip and the mute/solo buttons of a track.
class TrackOperationsWidget : public QWidget
{
	Q_OBJECT
public:

	//! @brief Create a new TrackOperationsWidget
	//! @param parent The TrackView to contain this widget
	TrackOperationsWidget(TrackView* parent);

	~TrackOperationsWidget() override = default;

	TrackGrip* getTrackGrip() const { return m_trackGrip; }

protected:
	//! @brief Respond to trackOperationsWidget mouse events
	//!
	//! If it's the left mouse button, and Ctrl is held down, and we're not a Pattern Editor track, then start a new
	//! drag event to copy this track. Otherwise, ignore all other events.
	//!
	//! @param me The mouse event to respond to.
	void mousePressEvent( QMouseEvent * me ) override;

	//! @brief Repaint the trackOperationsWidget
	//!
	//! Only things that's done for now is to paint the background with the brush of the window from the palette.
	void paintEvent(QPaintEvent* pe) override;

	//! @brief Show a message box warning the user that this track is about to be closed
	bool confirmRemoval();


private slots:
	void cloneTrack(); //!< @brief Clone this track
	void removeTrack(); //!< @brief Remove this track from the track list

	//! @brief Update the trackOperationsWidget context menu
	//!
	//! For all track types, we have the Clone and Remove options.
	//! For instrument-tracks we also offer the MIDI-control-menu
	//! For automation tracks, extra options: turn on/off recording on all Clips (same should be added for sample tracks
	//! when sampletrack recording is implemented)
	void updateMenu();

	void selectTrackColor();
	void randomizeTrackColor();
	void resetTrackColor();
	void resetClipColors();
	void toggleRecording(bool on);
	void recordingOn();
	void recordingOff();
	void clearTrack(); //!< @brief Clears all Clips from the track

private:
	TrackView * m_trackView;

	TrackGrip* m_trackGrip;
	QPushButton * m_trackOps;
	AutomatableButton* m_muteBtn;
	AutomatableButton* m_soloBtn;


	friend class TrackView;

signals:
	void trackRemovalScheduled( lmms::gui::TrackView * t );
};


} // namespace lmms::gui

#endif // LMMS_GUI_TRACK_OPERATIONS_WIDGET_H
