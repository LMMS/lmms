/*
 * BBTrackContainer.cpp - model-component of BB-Editor
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


#include "BBTrackContainer.h"
#include "BBTrack.h"
#include "Engine.h"
#include "Song.h"



BBTrackContainer::BBTrackContainer() :
	TrackContainer(),
	m_bbComboBoxModel( this )
{
	connect( &m_bbComboBoxModel, SIGNAL( dataChanged() ),
			this, SLOT( currentBBChanged() ) );
	// we *always* want to receive updates even in case BB actually did
	// not change upon setCurrentBB()-call
	connect( &m_bbComboBoxModel, SIGNAL( dataUnchanged() ),
			this, SLOT( currentBBChanged() ) );
	setType( BBContainer );
}




BBTrackContainer::~BBTrackContainer()
{
}




bool BBTrackContainer::play( MidiTime _start, fpp_t _frames,
								f_cnt_t _offset, int _tco_num )
{
	bool played_a_note = false;
	if( lengthOfBB( _tco_num ) <= 0 )
	{
		return false;
	}

	_start = _start % ( lengthOfBB( _tco_num ) * MidiTime::ticksPerBar() );

	TrackList tl = tracks();
	for( TrackList::iterator it = tl.begin(); it != tl.end(); ++it )
	{
		if( ( *it )->play( _start, _frames, _offset, _tco_num ) )
		{
			played_a_note = true;
		}
	}

	return played_a_note;
}




void BBTrackContainer::updateAfterTrackAdd()
{
	if( numOfBBs() == 0 && !Engine::getSong()->isLoadingProject() )
	{
		Engine::getSong()->addBBTrack();
	}
}




bar_t BBTrackContainer::lengthOfBB( int _bb ) const
{
	MidiTime max_length = MidiTime::ticksPerBar();

	const TrackList & tl = tracks();
	for (Track* t : tl)
	{
		// Don't create TCOs here if not exist
		if (_bb < t->numOfTCOs())
		{
			max_length = qMax(max_length, t->getTCO( _bb )->length());
		}
	}

	return max_length.nextFullBar();
}




int BBTrackContainer::numOfBBs() const
{
	return Engine::getSong()->countTracks( Track::BBTrack );
}




void BBTrackContainer::removeBB( int _bb )
{
	TrackList tl = tracks();
	for( TrackList::iterator it = tl.begin(); it != tl.end(); ++it )
	{
		delete ( *it )->getTCO( _bb );
		( *it )->removeBar( _bb * DefaultTicksPerBar );
	}
	if( _bb <= currentBB() )
	{
		setCurrentBB( qMax( currentBB() - 1, 0 ) );
	}
}




void BBTrackContainer::swapBB( int _bb1, int _bb2 )
{
	TrackList tl = tracks();
	for( TrackList::iterator it = tl.begin(); it != tl.end(); ++it )
	{
		( *it )->swapPositionOfTCOs( _bb1, _bb2 );
	}
	updateComboBox();
}




void BBTrackContainer::updateBBTrack( TrackContentObject * _tco )
{
	BBTrack * t = BBTrack::findBBTrack( _tco->startPosition() /
							DefaultTicksPerBar );
	if( t != NULL )
	{
		t->dataChanged();
	}
}




void BBTrackContainer::fixIncorrectPositions()
{
	TrackList tl = tracks();
	for( TrackList::iterator it = tl.begin(); it != tl.end(); ++it )
	{
		for( int i = 0; i < numOfBBs(); ++i )
		{
			( *it )->getTCO( i )->movePosition( MidiTime( i, 0 ) );
		}
	}
}




void BBTrackContainer::play()
{
	if( Engine::getSong()->playMode() != Song::Mode_PlayBB )
	{
		Engine::getSong()->playBB();
	}
	else
	{
		Engine::getSong()->togglePause();
	}
}




void BBTrackContainer::stop()
{
	Engine::getSong()->stop();
}




void BBTrackContainer::updateComboBox()
{
	const int cur_bb = currentBB();

	m_bbComboBoxModel.clear();

	for( int i = 0; i < numOfBBs(); ++i )
	{
		BBTrack * bbt = BBTrack::findBBTrack( i );
		m_bbComboBoxModel.addItem( bbt->name() );
	}
	setCurrentBB( cur_bb );
}




void BBTrackContainer::currentBBChanged()
{
	// now update all track-labels (the current one has to become white,
	// the others gray)
	TrackList tl = Engine::getSong()->tracks();
	for( TrackList::iterator it = tl.begin(); it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::BBTrack )
		{
			( *it )->dataChanged();
		}
	}
}




void BBTrackContainer::createTCOsForBB( int _bb )
{
	TrackList tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		tl[i]->createTCOsForBB( _bb );
	}
}

AutomatedValueMap BBTrackContainer::automatedValuesAt(MidiTime time, int tcoNum) const
{
	Q_ASSERT(tcoNum >= 0);
	Q_ASSERT(time.getTicks() >= 0);

	auto length_bars = lengthOfBB(tcoNum);
	auto length_ticks = length_bars * MidiTime::ticksPerBar();
	if (time > length_ticks)
	{
		time = length_ticks;
	}

	return TrackContainer::automatedValuesAt(time + (MidiTime::ticksPerBar() * tcoNum), tcoNum);
}

