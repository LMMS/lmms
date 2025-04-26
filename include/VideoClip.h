/*
 * VideoClip.h
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

#ifndef LMMS_VIDEO_CLIP_H
#define LMMS_VIDEO_CLIP_H

#include "Clip.h"

namespace lmms
{

namespace gui
{

class VideoClipView;
class VideoClipWindow;

}

class VideoClip : public Clip
{
    Q_OBJECT
public:
    VideoClip(Track * track);
    VideoClip(VideoClip & orig);
    ~VideoClip() override;

    gui::ClipView * createView( gui::TrackView * tv ) override;

	QString nodeName() const override
	{
		return "videoclip";
	}

    QString& videoFile()
    {
        return m_videoFile;
    }

	void changeLength( const TimePos & _length ) override;
	void setVideoFile(const QString& vf);

    void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
    void loadSettings( const QDomElement & _this ) override;

private:
    QString m_videoFile;

    gui::VideoClipWindow * m_window;

	friend class gui::VideoClipView;

signals:
    void videoChanged();

};

} // namespace lmms

#endif // LMMS_VIDEO_CLIP_H
