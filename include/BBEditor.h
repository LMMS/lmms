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

#include "Editor.h"
#include "TrackContainerView.h"


class BBTrackContainer;
class ComboBox;

class BBTrackContainerView;

class BBEditor : public Editor
{
	Q_OBJECT
public:
	BBEditor( BBTrackContainer * _tc );
	~BBEditor();

	QSize sizeHint() const;

	const BBTrackContainerView* trackContainerView() const {
		return m_trackContainerView;
	}
	BBTrackContainerView* trackContainerView() {
		return m_trackContainerView;
	}

	void removeBBView( int bb );

public slots:
	void play();
	void stop();

private:
	BBTrackContainerView* m_trackContainerView;
	ComboBox * m_bbComboBox;
} ;



class BBTrackContainerView : public TrackContainerView
{
	Q_OBJECT
public:
	BBTrackContainerView(BBTrackContainer* tc);

	bool fixedTCOs() const
	{
		return true;
	}

	void removeBBView(int bb);

	void saveSettings(QDomDocument& doc, QDomElement& element);
	void loadSettings(const QDomElement& element);

public slots:
	void addSteps();
	void cloneSteps();
	void removeSteps();
	void addAutomationTrack();

protected slots:
	void dropEvent(QDropEvent * de );
	void updatePosition();
	void postTrackWasAdded(Track * track);

private:
	BBTrackContainer * m_bbtc;
	void makeSteps( bool clone );
	// Resizes a newly added track to match size of pre-existing tracks in the BB
	void resizeNewTrack(Track * track);
};


#endif
