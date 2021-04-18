/*
 * BBTrack.cpp - implementation of class BBTrack
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
#include "BBTrack.h"

#include <QDomElement>
#include <QMenu>
#include <QPainter>

#include "BBTrackContainer.h"
#include "BBTrackView.h"
#include "Song.h"



BBTrack::infoMap BBTrack::s_infoMap;


BBTrack::BBTrack( TrackContainer* tc ) :
	Track( Track::BBTrack, tc )
{
	int bbNum = s_infoMap.size();
	s_infoMap[this] = bbNum;

	setName( tr( "Beat/Bassline %1" ).arg( bbNum ) );
	Engine::getBBTrackContainer()->createTCOsForBB( bbNum );
	Engine::getBBTrackContainer()->setCurrentBB( bbNum );
	Engine::getBBTrackContainer()->updateComboBox();

	connect( this, SIGNAL( nameChanged() ),
		Engine::getBBTrackContainer(), SLOT( updateComboBox() ) );
}




BBTrack::~BBTrack()
{
	Engine::mixer()->removePlayHandlesOfTypes( this,
					PlayHandle::TypeNotePlayHandle
					| PlayHandle::TypeInstrumentPlayHandle
					| PlayHandle::TypeSamplePlayHandle );

	const int bb = s_infoMap[this];
	Engine::getBBTrackContainer()->removeBB( bb );
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() > bb )
		{
			--it.value();
		}
	}
	s_infoMap.remove( this );

	// remove us from TC so bbTrackContainer::numOfBBs() returns a smaller
	// value and thus combobox-updating in bbTrackContainer works well
	trackContainer()->removeTrack( this );
	Engine::getBBTrackContainer()->updateComboBox();
}




// play _frames frames of given TCO within starting with _start
bool BBTrack::play( const TimePos & _start, const fpp_t _frames,
					const f_cnt_t _offset, int _tco_num )
{
	if( isMuted() )
	{
		return false;
	}

	if( _tco_num >= 0 )
	{
		return Engine::getBBTrackContainer()->play( _start, _frames, _offset, s_infoMap[this] );
	}

	tcoVector tcos;
	getTCOsInRange( tcos, _start, _start + static_cast<int>( _frames / Engine::framesPerTick() ) );

	if( tcos.size() == 0 )
	{
		return false;
	}

	TimePos lastPosition;
	TimePos lastLen;
	for( tcoVector::iterator it = tcos.begin(); it != tcos.end(); ++it )
	{
		if( !( *it )->isMuted() &&
				( *it )->startPosition() >= lastPosition )
		{
			lastPosition = ( *it )->startPosition();
			lastLen = ( *it )->length();
		}
	}

	if( _start - lastPosition < lastLen )
	{
		return Engine::getBBTrackContainer()->play( _start - lastPosition, _frames, _offset, s_infoMap[this] );
	}
	return false;
}




TrackView * BBTrack::createView( TrackContainerView* tcv )
{
	return new BBTrackView( this, tcv );
}




TrackContentObject* BBTrack::createTCO(const TimePos & pos)
{
	BBTCO* bbtco = new BBTCO(this);
	bbtco->movePosition(pos);
	return bbtco;
}




void BBTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
//	_this.setAttribute( "icon", m_trackLabel->pixmapFile() );
/*	_this.setAttribute( "current", s_infoMap[this] ==
					engine::getBBEditor()->currentBB() );*/
	if( s_infoMap[this] == 0 &&
			_this.parentNode().parentNode().nodeName() != "clone" &&
			_this.parentNode().parentNode().nodeName() != "journaldata" )
	{
		( (JournallingObject *)( Engine::getBBTrackContainer() ) )->
						saveState( _doc, _this );
	}
	if( _this.parentNode().parentNode().nodeName() == "clone" )
	{
		_this.setAttribute( "clonebbt", s_infoMap[this] );
	}
}




void BBTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
/*	if( _this.attribute( "icon" ) != "" )
	{
		m_trackLabel->setPixmapFile( _this.attribute( "icon" ) );
	}*/

	if( _this.hasAttribute( "clonebbt" ) )
	{
		const int src = _this.attribute( "clonebbt" ).toInt();
		const int dst = s_infoMap[this];
		TrackContainer::TrackList tl =
					Engine::getBBTrackContainer()->tracks();
		// copy TCOs of all tracks from source BB (at bar "src") to destination
		// TCOs (which are created if they do not exist yet)
		for( TrackContainer::TrackList::iterator it = tl.begin();
							it != tl.end(); ++it )
		{
			TrackContentObject::copyStateTo( ( *it )->getTCO( src ),
				( *it )->getTCO( dst ) );
		}
		setName( tr( "Clone of %1" ).arg(
					_this.parentNode().toElement().attribute( "name" ) ) );
	}
	else
	{
		QDomNode node = _this.namedItem(
					TrackContainer::classNodeName() );
		if( node.isElement() )
		{
			( (JournallingObject *)Engine::getBBTrackContainer() )->
					restoreState( node.toElement() );
		}
	}
/*	doesn't work yet because BBTrack-ctor also sets current bb so if
	bb-tracks are created after this function is called, this doesn't
	help at all....
	if( _this.attribute( "current" ).toInt() )
	{
		engine::getBBEditor()->setCurrentBB( s_infoMap[this] );
	}*/
}




// return pointer to BBTrack specified by _bb_num
BBTrack * BBTrack::findBBTrack( int _bb_num )
{
	for( infoMap::iterator it = s_infoMap.begin(); it != s_infoMap.end();
									++it )
	{
		if( it.value() == _bb_num )
		{
			return it.key();
		}
	}
	return NULL;
}




void BBTrack::swapBBTracks( Track * _track1, Track * _track2 )
{
	BBTrack * t1 = dynamic_cast<BBTrack *>( _track1 );
	BBTrack * t2 = dynamic_cast<BBTrack *>( _track2 );
	if( t1 != NULL && t2 != NULL )
	{
		qSwap( s_infoMap[t1], s_infoMap[t2] );
		Engine::getBBTrackContainer()->swapBB( s_infoMap[t1],
								s_infoMap[t2] );
		Engine::getBBTrackContainer()->setCurrentBB( s_infoMap[t1] );
	}
}
