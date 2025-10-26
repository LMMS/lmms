/*
 * PatternTrack.h - a track representing a pattern in the PatternStore
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

#ifndef LMMS_PATTERN_TRACK_H
#define LMMS_PATTERN_TRACK_H

#include <QMap>

#include "Track.h"

namespace lmms
{

class TrackContainer;

namespace gui
{

class PatternTrackView;

} // namespace gui


/*! Track type used in the Song (Editor) to reference a pattern in the PatternStore */
class LMMS_EXPORT PatternTrack : public Track
{
	Q_OBJECT
public:
	PatternTrack(TrackContainer* tc);
	~PatternTrack() override;

	bool play( const TimePos & _start, const fpp_t _frames,

						const f_cnt_t _frame_base, int _clip_num = -1 ) override;
	gui::TrackView * createView( gui::TrackContainerView* tcv ) override;
	Clip* createClip(const TimePos & pos) override;

	void saveTrackSpecificSettings(QDomDocument& doc, QDomElement& parent, bool presetMode) override;
	void loadTrackSpecificSettings( const QDomElement & _this ) override;

	static PatternTrack* findPatternTrack(int pattern_num);
	static void swapPatternTracks(Track* track1, Track* track2);

	int patternIndex()
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
		return "patterntrack";
	}


private:
	QList<Track *> m_disabledTracks;

	using infoMap = QMap<PatternTrack*, int>;
	static infoMap s_infoMap;

	friend class gui::PatternTrackView;
} ;



} // namespace lmms

#endif // LMMS_PATTERN_TRACK_H
