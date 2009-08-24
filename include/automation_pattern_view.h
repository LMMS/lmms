/*
 * automation_pattern_view.h - declaration of class automationPatternView
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _AUTOMATION_PATTERN_VIEW_H
#define _AUTOMATION_PATTERN_VIEW_H

#include "track.h"

class automationPattern;


class automationPatternView : public trackContentObjectView
{
	Q_OBJECT
public:
	automationPatternView( automationPattern * _pat, trackView * _parent );
	virtual ~automationPatternView();


public slots:
	virtual void update();


protected slots:
	void resetName();
	void changeName();
	void disconnectObject( QAction * _a );


protected:
	virtual void constructContextMenu( QMenu * );
	virtual void mouseDoubleClickEvent( QMouseEvent * _me );
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void resizeEvent( QResizeEvent * _re )
	{
		m_needsUpdate = true;
		trackContentObjectView::resizeEvent( _re );
	}
	virtual void dragEnterEvent( QDragEnterEvent * _dee );
	virtual void dropEvent( QDropEvent * _de );


private:
	automationPattern * m_pat;
	QPixmap m_paintPixmap;
	bool m_needsUpdate;

} ;


#endif
