/*
 * bb_track_container.cpp - model-component of BB-Editor
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "bb_track_container.h"
#include "bb_track.h"
#include "combobox.h"
#include "engine.h"
#include "song.h"



bbTrackContainer::bbTrackContainer( void ) :
	trackContainer(),
	m_bbComboBoxModel( this )
{
	connect( &m_bbComboBoxModel, SIGNAL( dataChanged() ),
			this, SLOT( currentBBChanged() ),
			Qt::QueuedConnection );
	// we *always* want to receive updates even in case BB actually did
	// not change upon setCurrentBB()-call
	connect( &m_bbComboBoxModel, SIGNAL( dataUnchanged() ),
			this, SLOT( currentBBChanged() ),
			Qt::QueuedConnection );
}




bbTrackContainer::~bbTrackContainer()
{
}




bool bbTrackContainer::play( midiTime _start, fpp_t _frames,
							f_cnt_t _offset,
							Sint16 _tco_num )
{
	bool played_a_note = FALSE;
	if( lengthOfBB( _tco_num ) <= 0 )
	{
		return( played_a_note );
	}

	_start = ( _start.getTact() % lengthOfBB( _tco_num ) ) *
							DefaultTicksPerTact +
							_start.getTicks();
	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		if( tl[i]->play( _start, _frames, _offset, _tco_num ) == TRUE )
		{
			played_a_note = TRUE;
		}
	}

	return( played_a_note );
}




void bbTrackContainer::updateAfterTrackAdd( void )
{
	// make sure, new track(s) have TCOs for every beat/bassline
	for( int i = 0; i < tMax<int>( 1, numOfBBs() ); ++i )
	{
		createTCOsForBB( i );
	}
}




tact bbTrackContainer::lengthOfBB( int _bb )
{
	midiTime max_length;

	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		trackContentObject * tco = tl[i]->getTCO( _bb );
		max_length = tMax( max_length, tco->length() );
	}
	if( max_length.getTicks() == 0 )
	{
		return( max_length.getTact() );
	}

	return( max_length.getTact() + 1 );
}




int bbTrackContainer::numOfBBs( void ) const
{
	return( engine::getSong()->countTracks( track::BBTrack ) );
}




void bbTrackContainer::removeBB( int _bb )
{
	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		delete tl[i]->getTCO( _bb );
		tl[i]->removeTact( _bb * DefaultTicksPerTact );
	}
	if( _bb <= currentBB() )
	{
		setCurrentBB( tMax( currentBB() - 1, 0 ) );
	}
}




void bbTrackContainer::swapBB( int _bb1, int _bb2 )
{
	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		tl[i]->swapPositionOfTCOs( _bb1, _bb2 );
	}
	updateComboBox();
}




void bbTrackContainer::updateBBTrack( trackContentObject * _tco )
{
	bbTrack * t = bbTrack::findBBTrack( _tco->startPosition() /
							DefaultTicksPerTact );
	if( t != NULL )
	{
		t->dataChanged();
		//t->getTrackContentWidget()->update();
	}
}




void bbTrackContainer::play( void )
{
	if( engine::getSong()->isPlaying() )
	{
		if( engine::getSong()->playMode() != song::Mode_PlayBB )
		{
			engine::getSong()->stop();
			engine::getSong()->playBB();
		}
		else
		{
			engine::getSong()->pause();
		}
	}
	else if( engine::getSong()->isPaused() )
	{
		engine::getSong()->resumeFromPause();
	}
	else
	{
		engine::getSong()->playBB();
	}

}




void bbTrackContainer::stop( void )
{
	engine::getSong()->stop();
}




void bbTrackContainer::updateComboBox( void )
{
	const int cur_bb = currentBB();

	m_bbComboBoxModel.clear();

	for( int i = 0; i < numOfBBs(); ++i )
	{
		bbTrack * bbt = bbTrack::findBBTrack( i );
		m_bbComboBoxModel.addItem( bbt->name(),
				bbt->pixmap() ? new QPixmap( *bbt->pixmap() )
								: NULL );
	}
	setCurrentBB( cur_bb );
}




void bbTrackContainer::currentBBChanged( void )
{
	// first make sure, all channels have a TCO at current BB
	createTCOsForBB( currentBB() );

	// now update all track-labels (the current one has to become white,
	// the others green)
	for( int i = 0; i < numOfBBs(); ++i )
	{
		bbTrack::findBBTrack( i )->dataChanged();
//trackLabel()->update();
	}

	//emit dataChanged();
	//emit positionChanged( NULL );
}




void bbTrackContainer::createTCOsForBB( int _bb )
{
	if( numOfBBs() == 0 || engine::getSong()->isLoadingProject() )
	{
		return;
	}

	QList<track *> tl = tracks();
	for( int i = 0; i < tl.size(); ++i )
	{
		while( tl[i]->numOfTCOs() < _bb + 1 )
		{
			midiTime position = midiTime( tl[i]->numOfTCOs(), 0 );
			trackContentObject * tco = tl[i]->addTCO(
						tl[i]->createTCO( position ) );
			tco->movePosition( position );
			tco->changeLength( midiTime( 1, 0 ) );
		}
	}
}



#include "bb_track_container.moc"

