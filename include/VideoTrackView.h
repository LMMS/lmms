/*
 * VideoTrackView.h
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

#ifndef LMMS_VIDEO_TRACK_VIEW_H
#define LMMS_VIDEO_TRACK_VIEW_H

#include "TrackView.h"

namespace lmms
{

class VideoTrack;

namespace gui
{

class TrackLabelButton;

class VideoTrackView : public TrackView
{
	Q_OBJECT
public:
	VideoTrackView(VideoTrack * _track, TrackContainerView* tcv);

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	VideoTrack * model()
	{
		return castModel<VideoTrack>();
	}

	const VideoTrack * model() const
	{
		return castModel<VideoTrack>();
	}

protected:
	QString nodeName() const override
	{
		return "VideoTrackView";
	}

private:
	TrackLabelButton * m_trackLabel;

};

} // namespace gui

} // namespace lmms

#endif // LMMS_VIDEO_TRACK_VIEW_H
