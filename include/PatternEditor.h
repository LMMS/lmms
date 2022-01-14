/*
 * PatternEditor.h - view-component of Pattern Editor
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


#ifndef PATTERN_EDITOR_H
#define PATTERN_EDITOR_H


#include "Editor.h"
#include "TrackContainerView.h"


class PatternTrackContainer;
class ComboBox;

class PatternTrackContainerView;

class PatternEditor : public Editor
{
	Q_OBJECT
public:
	PatternEditor( PatternTrackContainer * _tc );
	~PatternEditor();

	QSize sizeHint() const override;

	const PatternTrackContainerView* trackContainerView() const {
		return m_trackContainerView;
	}
	PatternTrackContainerView* trackContainerView() {
		return m_trackContainerView;
	}

	void removePatternView(int pattern);

public slots:
	void play() override;
	void stop() override;

private:
	PatternTrackContainerView* m_trackContainerView;
	ComboBox * m_patternComboBox;
} ;



class PatternTrackContainerView : public TrackContainerView
{
	Q_OBJECT
public:
	PatternTrackContainerView(PatternTrackContainer* tc);

	bool fixedClips() const override
	{
		return true;
	}

	void removePatternView(int pattern);

	void saveSettings(QDomDocument& doc, QDomElement& element) override;
	void loadSettings(const QDomElement& element) override;

public slots:
	void addSteps();
	void cloneSteps();
	void removeSteps();
	void addSampleTrack();
	void addAutomationTrack();
	void cloneClip();

protected slots:
	void dropEvent(QDropEvent * de ) override;
	void updatePosition();

private:
	PatternTrackContainer * m_ptc;
	void makeSteps( bool clone );
};


#endif
