/*
 * PatternClipView.h
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

#ifndef LMMS_GUI_PATTERN_CLIP_VIEW_H
#define LMMS_GUI_PATTERN_CLIP_VIEW_H

#include <QStaticText>

#include "ClipView.h"

namespace lmms
{

class PatternClip;

namespace gui
{

class PatternClipView : public ClipView
{
	Q_OBJECT
	Q_PROPERTY(float emptyTrackHeightRatio MEMBER m_emptyTrackHeightRatio)
	Q_PROPERTY(float verticalPadding MEMBER m_verticalPadding)
	Q_PROPERTY(float noteVerticalSpacing MEMBER m_noteVerticalSpacing)
	Q_PROPERTY(float noteHorizontalSpacing MEMBER m_noteHorizontalSpacing)
	Q_PROPERTY(QColor noteColor MEMBER m_noteColor)
public:
	PatternClipView(Clip* clip, TrackView* tv);
	~PatternClipView() override = default;


public slots:
	void update() override;

protected slots:
	void openInPatternEditor();
	void resetName();
	void changeName();


protected:
	void paintEvent( QPaintEvent * pe ) override;
	void mouseDoubleClickEvent( QMouseEvent * _me ) override;
	void constructContextMenu( QMenu * ) override;


private:
	PatternClip* m_patternClip;
	QPixmap m_paintPixmap;
	
	QStaticText m_staticTextName;

	float m_emptyTrackHeightRatio {0.5f};
	float m_verticalPadding {0.15f};
	float m_noteVerticalSpacing {0.2f};
	float m_noteHorizontalSpacing {0.2f};
	QColor m_noteColor {255, 255, 255};
} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PATTERN_CLIP_VIEW_H
