/*
 * bb_track_container.h - model-component of BB-Editor
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _BB_TRACK_CONTAINER_H
#define _BB_TRACK_CONTAINER_H

#include "track_container.h"
#include "combobox.h"


class EXPORT bbTrackContainer : public trackContainer
{
	Q_OBJECT
	mapPropertyFromModel(int,currentBB,setCurrentBB,m_bbComboBoxModel);
public:
	bbTrackContainer( void );
	virtual ~bbTrackContainer();

	virtual bool play( midiTime _start, const fpp_t _frames,
						const f_cnt_t _frame_base,
							Sint16 _tco_num = -1 );

	virtual void updateAfterTrackAdd( void );

	inline virtual QString nodeName( void ) const
	{
		return "bbtrackcontainer";
	}

	tact_t lengthOfBB( int _bb );
	inline tact_t lengthOfCurrentBB( void )
	{
		return lengthOfBB( currentBB() );
	}
	int numOfBBs( void ) const;
	void removeBB( int _bb );

	void swapBB( int _bb1, int _bb2 );

	void updateBBTrack( trackContentObject * _tco );
	void fixIncorrectPositions( void );
	void createTCOsForBB( int _bb );


public slots:
	void play( void );
	void stop( void );
	void updateComboBox( void );
	void currentBBChanged( void );


private:
	ComboBoxModel m_bbComboBoxModel;


	friend class bbEditor;

} ;


#endif
