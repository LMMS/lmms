/*
 * VideoTrack.cpp - Implementation of class VideoTrack, which provides a track
 * for VideoClips to be viewed in sync to the music.
 *
 * Copyright (c) 2024 regulus79
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "VideoTrackView.h"
#include "VideoTrack.h"
#include "VideoClip.h"

namespace lmms
{

VideoTrack::VideoTrack(TrackContainer* tc):
	Track(Track::Type::Video, tc)
{
	setName(tr("Video track"));
}

bool VideoTrack::play(const TimePos & start, const fpp_t frames, const f_cnt_t frameBase, int clipNum)
{
	return true;
}

gui::TrackView * VideoTrack::createView(gui::TrackContainerView * view)
{
	return new gui::VideoTrackView(this, view);
}

Clip * VideoTrack::createClip(const TimePos & pos)
{
	auto vClip = new VideoClip(this);
	vClip->movePosition(pos);
	return vClip;
}

void VideoTrack::saveTrackSpecificSettings(QDomDocument& doc, QDomElement& parent, bool presetMode)
{
}

void VideoTrack::loadTrackSpecificSettings(const QDomElement & element)
{
}

} // namespace lmms
