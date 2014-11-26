/*
 * BBEditor.h - view-component of BB-Editor
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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


#ifndef BB_EDITOR_H
#define BB_EDITOR_H

#include "TrackContainerView.h"


class BBTrackContainer;
class ComboBox;
class ToolButton;


class bbEditor : public TrackContainerView
{
	Q_OBJECT
public:
	bbEditor( BBTrackContainer * _tc );
	virtual ~bbEditor();

	virtual inline bool fixedTCOs() const
	{
		return( true );
	}
	
	virtual void dropEvent( QDropEvent * _de );

	void removeBBView( int _bb );

	void setPauseIcon( bool pause );


public slots:
	void play();
	void stop();
	void updatePosition();
	void addAutomationTrack();
	void addSteps();
	void removeSteps();

private:
	virtual void keyPressEvent( QKeyEvent * _ke );

	BBTrackContainer * m_bbtc;
	QWidget * m_toolBar;

	ToolButton * m_playButton;
	ToolButton * m_stopButton;

	ComboBox * m_bbComboBox;

} ;


#endif
