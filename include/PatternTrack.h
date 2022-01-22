/*
 * PatternTrack.h - class PatternTrack, a wrapper for using PatternEditor
 *              (which is a singleton-class) as track
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


#ifndef PATTERN_TRACK_H
#define PATTERN_TRACK_H


#include <QtCore/QMap>

#include "PatternClipView.h"
#include "Track.h"

class TrackLabelButton;
class TrackContainer;


class LMMS_EXPORT PatternTrack : public Track
{
	Q_OBJECT
public:
	PatternTrack(TrackContainer* tc);
	virtual ~PatternTrack();

	virtual bool play( const TimePos & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _clip_num = -1 ) override;
	TrackView * createView( TrackContainerView* tcv ) override;
	Clip* createClip(const TimePos & pos) override;

	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent ) override;
	void loadTrackSpecificSettings( const QDomElement & _this ) override;

	static PatternTrack* findPatternTrack(int pattern_num);
	static void swapPatternTracks(Track* track1, Track* track2);

	int index()
	{
		return s_infoMap[this];
	}

	bool automationDisabled( Track * _track )
	{
		return( m_disabledTracks.contains( _track ) );
	}
	void disableAutomation( Track * _track )
	{
		m_disabledTracks.append( _track );
	}
	void enableAutomation( Track * _track )
	{
		m_disabledTracks.removeAll( _track );
	}

protected:
	inline QString nodeName() const override
	{
		return( "bbtrack" ); //TODO rename to patterntrack
	}


private:
	QList<Track *> m_disabledTracks;

	typedef QMap<PatternTrack*, int> infoMap;
	static infoMap s_infoMap;

	friend class PatternTrackView;
} ;



#endif
