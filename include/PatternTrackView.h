/*
 * PatternTrackView.h
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


#ifndef PATTERN_TRACK_VIEW_H
#define PATTERN_TRACK_VIEW_H


#include "TrackView.h"

class PatternTrack;
class TrackLabelButton;


class PatternTrackView : public TrackView
{
	Q_OBJECT
public:
	PatternTrackView(PatternTrack* pt, TrackContainerView* tcv);
	virtual ~PatternTrackView();

	bool close() override;

	const PatternTrack* getPatternTrack() const
	{
		return (m_patternTrack);
	}


public slots:
	void clickedTrackLabel();


private:
	PatternTrack* m_patternTrack;
	TrackLabelButton * m_trackLabel;
} ;



#endif
