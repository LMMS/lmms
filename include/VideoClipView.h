/*
 * VideoClipView.h
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

#ifndef LMMS_VIDEO_CLIP_VIEW_H
#define LMMS_VIDEO_CLIP_VIEW_H

#include "ClipView.h"

namespace lmms
{

class VideoClip;

namespace gui
{

class VideoClipView : public ClipView
{
    Q_OBJECT
public:
    VideoClipView(VideoClip * clip, TrackView * tv);
    //~VideoClip() override;

protected:
	void paintEvent(QPaintEvent * pe) override;

private:
    VideoClip * m_clip;
	QPixmap m_paintPixmap;
    
    bool splitClip( const TimePos pos ) override;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_VIDEO_CLIP_VIEW_H