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

#ifndef TRACK_OPERATIONS_WIDGET_H
#define TRACK_OPERATIONS_WIDGET_H

#include <QWidget>

class QPushButton;

class PixmapButton;
class TrackView;

class TrackOperationsWidget : public QWidget
{
	Q_OBJECT
public:
	TrackOperationsWidget( TrackView * parent );
	~TrackOperationsWidget();


protected:
	void mousePressEvent( QMouseEvent * me ) override;
	void paintEvent( QPaintEvent * pe ) override;


private slots:
	void cloneTrack();
	void removeTrack();
	void updateMenu();
	void changeTrackColor();
	void randomTrackColor();
	void resetTrackColor();
	void useTrackColor();
	void toggleRecording(bool on);
	void recordingOn();
	void recordingOff();
	void clearTrack();

private:
	TrackView * m_trackView;

	QPushButton * m_trackOps;
	PixmapButton * m_muteBtn;
	PixmapButton * m_soloBtn;


	friend class TrackView;

signals:
	void trackRemovalScheduled( TrackView * t );
	void colorChanged( QColor & c );
	void colorParented();
	void colorReset();

} ;

#endif
