/*
 * AutomationTrack.h - declaration of class AutomationTrack, which handles
 *                     automation of objects without a track
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#ifndef AUTOMATION_TRACK_H
#define AUTOMATION_TRACK_H

#include "Track.h"


class AutomationTrack : public Track
{
	Q_OBJECT
public:
	AutomationTrack( TrackContainer* tc, bool _hidden = false );
	virtual ~AutomationTrack() = default;

	virtual bool play( const MidiTime & _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 ) override;

	QString nodeName() const override
	{
		return "automationtrack";
	}

	TrackView * createView( TrackContainerView* ) override;
	TrackContentObject * createTCO( const MidiTime & _pos ) override;

	virtual void saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _parent ) override;
	void loadTrackSpecificSettings( const QDomElement & _this ) override;

private:
	friend class AutomationTrackView;

} ;



class AutomationTrackView : public TrackView
{
public:
	AutomationTrackView( AutomationTrack* at, TrackContainerView* tcv );
	virtual ~AutomationTrackView() = default;

	void dragEnterEvent( QDragEnterEvent * _dee ) override;
	void dropEvent( QDropEvent * _de ) override;

} ;


#endif
