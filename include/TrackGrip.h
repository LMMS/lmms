/*
 * TrackGrip.h - Grip that can be used to move tracks
 *
 * Copyright (c) 2024- Michael Gregorius
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

#ifndef LMMS_GUI_TRACK_GRIP_H
#define LMMS_GUI_TRACK_GRIP_H

#include <QWidget>


class QPixmap;

namespace lmms
{

class Track;

namespace gui
{

class TrackGrip : public QWidget
{
	Q_OBJECT
public:
	TrackGrip(Track* track, QWidget* parent = 0);
	~TrackGrip() override = default;

signals:
	void grabbed();
	void released();

protected:
	void mousePressEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void paintEvent(QPaintEvent*) override;

private:
	Track* m_track = nullptr;
	bool m_isGrabbed = false;
	static QPixmap* s_grabbedPixmap;
	static QPixmap* s_releasedPixmap;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_TRACK_GRIP_H
