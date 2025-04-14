/*
 * VideoTrack.h - class VideoTrack, a track for VideoClips to be shown to sync music to
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

#ifndef LMMS_VIDEO_TRACK_H
#define LMMS_VIDEO_TRACK_H

#include "Track.h"

namespace lmms
{

namespace gui
{

//class VideoTrackView;

}

class VideoTrack : public Track
{
	Q_OBJECT
public:
	VideoTrack(TrackContainer* tc);

	bool play( const TimePos & start, const fpp_t frames,
						const f_cnt_t frameBase, int clipNum = -1 ) override;

	gui::TrackView * createView( gui::TrackContainerView * view ) override;

	Clip * createClip( const TimePos & pos ) override;

	void saveTrackSpecificSettings(QDomDocument& doc, QDomElement& parent, bool presetMode) override;
	void loadTrackSpecificSettings( const QDomElement & element ) override;

	QString nodeName() const override
	{
		return "videotrack";
	}
};


} // namespace lmms

#endif // LMMS_VIDEO_TRACK_H
