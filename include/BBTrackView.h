/*
 * BBTrackView.h
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


#ifndef BB_TRACK_VIEW_H
#define BB_TRACK_VIEW_H

#include <QtCore/QObject>

#include "BBTrack.h"
#include "TrackView.h"


namespace lmms::gui
{

class BBTrackView : public TrackView
{
	Q_OBJECT
public:
	BBTrackView( BBTrack* bbt, TrackContainerView* tcv );
	virtual ~BBTrackView();

	bool close() override;

	const BBTrack * getBBTrack() const
	{
		return( m_bbTrack );
	}


public slots:
	void clickedTrackLabel();


private:
	BBTrack * m_bbTrack;
	TrackLabelButton * m_trackLabel;
} ;



} // namespace lmms::gui

#endif