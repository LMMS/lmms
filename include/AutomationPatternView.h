/*
 * AutomationPatternView.h - declaration of class AutomationPatternView
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef AUTOMATION_PATTERN_VIEW_H
#define AUTOMATION_PATTERN_VIEW_H

#include <QStaticText>

#include "Track.h"

class AutomationPattern;


class AutomationPatternView : public TrackContentObjectView
{
	Q_OBJECT


public:
	AutomationPatternView( AutomationPattern * _pat, TrackView * _parent );
	virtual ~AutomationPatternView();

public slots:
	/// Opens this view's pattern in the global automation editor
	void openInAutomationEditor();
	void update() override;


protected slots:
	void resetName();
	void changeName();
	void disconnectObject( QAction * _a );
	void toggleRecording();
	void flipY();
	void flipX();

protected:
	void constructContextMenu( QMenu * ) override;
	void mouseDoubleClickEvent(QMouseEvent * me ) override;
	void paintEvent( QPaintEvent * pe ) override;
	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;


private:
	AutomationPattern * m_pat;
	QPixmap m_paintPixmap;
	
	QStaticText m_staticTextName;
	
	static QPixmap * s_pat_rec;

	void scaleTimemapToFit( float oldMin, float oldMax );
} ;


#endif
