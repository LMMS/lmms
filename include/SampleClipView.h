/*
 * SampleClipView.h
 *
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

#ifndef SAMPLE_CLIP_VIEW_H
#define SAMPLE_CLIP_VIEW_H

#include "SampleClipView.h"

#include "SampleClip.h"
#include "ClipView.h"

class SampleClipView : public ClipView
{
	Q_OBJECT

public:
	SampleClipView( SampleClip * _clip, TrackView * _tv );
	virtual ~SampleClipView() = default;

public slots:
	void updateSample();
	void reverseSample();



protected:
	void constructContextMenu(QMenu* cm) override;
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;
	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;
	void mouseDoubleClickEvent( QMouseEvent * ) override;
	void paintEvent( QPaintEvent * ) override;


private:
	SampleClip * m_clip;
	QPixmap m_paintPixmap;
	bool splitClip( const TimePos pos ) override;
} ;



#endif