/*
 * VideoClip.cpp
 *
 * Copyright (c) 2024 regulus79
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
#include <QDebug>

#include "VideoClip.h"
#include "VideoClipView.h"
#include "VideoClipWindow.h"

namespace lmms
{

VideoClip::VideoClip(Track * track):
    Clip(track)
{
	qDebug() << "Normal Constructor!";
	saveJournallingState( false );
	setVideoFile( "" );
	changeLength(TimePos::ticksPerBar());
	restoreJournallingState();

	m_window = new gui::VideoClipWindow(this);
	m_window->toggleVisibility(false);

	setAutoResize(false);
}

VideoClip::VideoClip(VideoClip & orig):
    VideoClip(orig.getTrack())
{
	qDebug() << "Copy Constructor!";
	setVideoFile(orig.videoFile());
}

gui::ClipView * VideoClip::createView(gui::TrackView * tv)
{
    return new gui::VideoClipView(this, tv);
}

void VideoClip::changeLength(const TimePos & _length)
{
	Clip::changeLength(std::max(static_cast<int>(_length), 1));
}

void VideoClip::setVideoFile(const QString& vf)
{
    m_videoFile = vf;
	setStartTimeOffset(0);
	qDebug() << "setVideoFile!";
	if (!vf.isEmpty())
	{
		qDebug() << "Emitting videoChanged!";
		emit videoChanged();
	}
}

void VideoClip::saveSettings( QDomDocument & _doc, QDomElement & _parent )
{
}

void VideoClip::loadSettings( const QDomElement & _this )
{
}

} // namespace lmms