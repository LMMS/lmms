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

class TrackOperationsWidget : public QWidget
{
	Q_OBJECT
public:
	TrackOperationsWidget( TrackView * parent );
	~TrackOperationsWidget() override = default;

	TrackGrip* getTrackGrip() const { return m_trackGrip; }

protected:
	void mousePressEvent( QMouseEvent * me ) override;
	void paintEvent( QPaintEvent * pe ) override;
	bool confirmRemoval();


private slots:
	void cloneTrack();
	void removeTrack();
	void updateMenu();
	void selectTrackColor();
	void randomizeTrackColor();
	void resetTrackColor();
	void resetClipColors();
	void toggleRecording(bool on);
	void recordingOn();
	void recordingOff();
	void clearTrack();

private:
	TrackView * m_trackView;

	TrackGrip* m_trackGrip;
	QPushButton * m_trackOps;
	AutomatableButton* m_muteBtn;
	AutomatableButton* m_soloBtn;


	friend class TrackView;

signals:
	void trackRemovalScheduled( lmms::gui::TrackView * t );

} ;


} // namespace lmms::gui

#endif // LMMS_GUI_TRACK_OPERATIONS_WIDGET_H
