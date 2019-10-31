/*
 * BBTrackContainer.h - model-component of BB-Editor
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


#ifndef BB_TRACK_CONTAINER_H
#define BB_TRACK_CONTAINER_H

#include "TrackContainer.h"
#include "ComboBoxModel.h"


class LMMS_EXPORT BBTrackContainer : public TrackContainer
{
	Q_OBJECT
	mapPropertyFromModel(int,currentBB,setCurrentBB,m_bbComboBoxModel);
public:
	BBTrackContainer();
	virtual ~BBTrackContainer();

	virtual bool play( MidiTime _start, const fpp_t _frames,
						const f_cnt_t _frame_base, int _tco_num = -1 );

	void updateAfterTrackAdd() override;

	inline QString nodeName() const override
	{
		return "bbtrackcontainer";
	}

	bar_t lengthOfBB( int _bb ) const;
	inline bar_t lengthOfCurrentBB()
	{
		return lengthOfBB( currentBB() );
	}
	int numOfBBs() const;
	void removeBB( int _bb );

	void swapBB( int _bb1, int _bb2 );

	void updateBBTrack( TrackContentObject * _tco );
	void fixIncorrectPositions();
	void createTCOsForBB( int _bb );

	AutomatedValueMap automatedValuesAt(MidiTime time, int tcoNum) const override;

public slots:
	void play();
	void stop();
	void updateComboBox();
	void currentBBChanged();


private:
	ComboBoxModel m_bbComboBoxModel;


	friend class BBEditor;

} ;


#endif
