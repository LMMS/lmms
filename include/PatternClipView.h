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
public:
	PatternClipView(Clip* clip, TrackView* tv);
	~PatternClipView() override = default;


public slots:
	void update() override;

protected slots:
	void openInPatternEditor();
	void resetName();
	void changeName();

	// InteractiveModelView methods
	//const std::vector<ActionStruct>& getActions() override { return getActionsT(); }
	//const QString& getShortcutMessage() override { return getShortcutMessageT(); }
	//size_t getTypeId() override { return typeid(*this).hash_code(); }

protected:
	void paintEvent( QPaintEvent * pe ) override;
	void mouseDoubleClickEvent( QMouseEvent * _me ) override;
	void constructContextMenu( QMenu * ) override;


private:
	PatternClip* m_patternClip;
	QPixmap m_paintPixmap;
	
	QStaticText m_staticTextName;
} ;

/*
		s_actionArray = ClipView::getActions();
		if (s_actionArray.size() > 2)
		{
			s_actionArray[2].addAcceptedDataType(getClipStringPairType(getClip()->getTrack()));
		}
		m_shortcutMessage = buildShortcutMessage();
*/


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_PATTERN_CLIP_VIEW_H
