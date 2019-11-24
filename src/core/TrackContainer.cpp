/*
 * TrackContainer.cpp - implementation of base class for all trackcontainers
 *                      like Song-Editor, BB-Editor...
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


#include <QApplication>
#include <QProgressDialog>
#include <QDomElement>
#include <QWriteLocker>

#include "AutomationPattern.h"
#include "AutomationTrack.h"
#include "BBTrack.h"
#include "BBTrackContainer.h"
#include "embed.h"
#include "TrackContainer.h"
#include "InstrumentTrack.h"
#include "Song.h"

#include "GuiApplication.h"
#include "MainWindow.h"
#include "TextFloat.h"

TrackContainer::TrackContainer() :
	Model( NULL ),
	JournallingObject(),
	m_tracksMutex(),
	m_tracks()
{
}




TrackContainer::~TrackContainer()
{
	clearAllTracks();
}




void TrackContainer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setTagName( classNodeName() );
	_this.setAttribute( "type", nodeName() );

	// save settings of each track
	m_tracksMutex.lockForRead();
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		m_tracks[i]->saveState( _doc, _this );
	}
	m_tracksMutex.unlock();
}




void TrackContainer::loadSettings( const QDomElement & _this )
{
	bool journalRestore = _this.parentNode().nodeName() == "journaldata";
	if( journalRestore )
	{
		clearAllTracks();
	}

	static QProgressDialog * pd = NULL;
	bool was_null = ( pd == NULL );
	if( !journalRestore && gui != nullptr )
	{
		if( pd == NULL )
		{
			pd = new QProgressDialog( tr( "Loading project..." ),
						tr( "Cancel" ), 0,
						Engine::getSong()->getLoadingTrackCount(),
						gui->mainWindow() );
			pd->setWindowModality( Qt::ApplicationModal );
			pd->setWindowTitle( tr( "Please wait..." ) );
			pd->show();
		}
	}

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( pd != NULL )
		{
			pd->setValue( pd->value() + 1 );
			QCoreApplication::instance()->processEvents(
						QEventLoop::AllEvents, 100 );
			if( pd->wasCanceled() )
			{
				if ( gui )
				{
					TextFloat::displayMessage( tr( "Loading cancelled" ),
					tr( "Project loading was cancelled." ),
					embed::getIconPixmap( "project_file", 24, 24 ),
					2000 );
				}
				Engine::getSong()->loadingCancelled();
				break;
			}
		}

		if( node.isElement() &&
			!node.toElement().attribute( "metadata" ).toInt() )
		{
			QString trackName = node.toElement().hasAttribute( "name" ) ?
						node.toElement().attribute( "name" ) :
						node.firstChild().toElement().attribute( "name" );
			if( pd != NULL )
			{
				pd->setLabelText( tr("Loading Track %1 (%2/Total %3)").arg( trackName ).
						  arg( pd->value() + 1 ).arg( Engine::getSong()->getLoadingTrackCount() ) );
			}
			Track::create( node.toElement(), this );
		}
		node = node.nextSibling();
	}

	if( pd != NULL )
	{
		if( was_null )
		{
			delete pd;
			pd = NULL;
		}
	}
}




int TrackContainer::countTracks( Track::TrackTypes _tt ) const
{
	int cnt = 0;
	m_tracksMutex.lockForRead();
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		if( m_tracks[i]->type() == _tt || _tt == Track::NumTrackTypes )
		{
			++cnt;
		}
	}
	m_tracksMutex.unlock();
	return( cnt );
}




void TrackContainer::addTrack( Track * _track )
{
	if( _track->type() != Track::HiddenAutomationTrack )
	{
		_track->lock();
		m_tracksMutex.lockForWrite();
		m_tracks.push_back( _track );
		m_tracksMutex.unlock();
		_track->unlock();
		emit trackAdded( _track );
	}
}




void TrackContainer::removeTrack( Track * _track )
{
	// need a read locker to ensure that m_tracks doesn't change after reading index.
	//   After checking that index != -1, we need to upgrade the lock to a write locker before changing m_tracks.
	//   But since Qt offers no function to promote a read lock to a write lock, we must start with the write locker.
	QWriteLocker lockTracksAccess(&m_tracksMutex);
	int index = m_tracks.indexOf( _track );
	if( index != -1 )
	{
		// If the track is solo, all other tracks are muted. Change this before removing the solo track:
		if (_track->isSolo()) {
			_track->setSolo(false);
		}
		m_tracks.remove( index );
		lockTracksAccess.unlock();

		if( Engine::getSong() )
		{
			Engine::getSong()->setModified();
		}
	}
}




void TrackContainer::updateAfterTrackAdd()
{
}




void TrackContainer::clearAllTracks()
{
	//m_tracksMutex.lockForWrite();
	while( !m_tracks.isEmpty() )
	{
		delete m_tracks.first();
	}
	//m_tracksMutex.unlock();
}




bool TrackContainer::isEmpty() const
{
	for( TrackList::const_iterator it = m_tracks.begin();
						it != m_tracks.end(); ++it )
	{
		if( !( *it )->getTCOs().isEmpty() )
		{
			return false;
		}
	}
	return true;
}



AutomatedValueMap TrackContainer::automatedValuesAt(MidiTime time, int tcoNum) const
{
	return automatedValuesFromTracks(tracks(), time, tcoNum);
}


AutomatedValueMap TrackContainer::automatedValuesFromTracks(const TrackList &tracks, MidiTime time, int tcoNum)
{
	Track::tcoVector tcos;

	for (Track* track: tracks)
	{
		if (track->isMuted()) {
			continue;
		}

		switch(track->type())
		{
		case Track::AutomationTrack:
		case Track::HiddenAutomationTrack:
		case Track::BBTrack:
			if (tcoNum < 0) {
				track->getTCOsInRange(tcos, 0, time);
			} else {
				Q_ASSERT(track->numOfTCOs() > tcoNum);
				tcos << track->getTCO(tcoNum);
			}
		default:
			break;
		}
	}

	AutomatedValueMap valueMap;

	Q_ASSERT(std::is_sorted(tcos.begin(), tcos.end(), TrackContentObject::comparePosition));

	for(TrackContentObject* tco : tcos)
	{
		if (tco->isMuted() || tco->startPosition() > time) {
			continue;
		}

		if (auto* p = dynamic_cast<AutomationPattern *>(tco))
		{
			if (! p->hasAutomation()) {
				continue;
			}
			MidiTime relTime = time - p->startPosition();
			if (! p->getAutoResize()) {
				relTime = qMin(relTime, p->length());
			}
			float value = p->valueAt(relTime);

			for (AutomatableModel* model : p->objects())
			{
				valueMap[model] = value;
			}
		}
		else if (auto* bb = dynamic_cast<BBTCO *>(tco))
		{
			auto bbIndex = dynamic_cast<class BBTrack*>(bb->getTrack())->index();
			auto bbContainer = Engine::getBBTrackContainer();

			MidiTime bbTime = time - tco->startPosition();
			bbTime = std::min(bbTime, tco->length());
			bbTime = bbTime % (bbContainer->lengthOfBB(bbIndex) * MidiTime::ticksPerBar());

			auto bbValues = bbContainer->automatedValuesAt(bbTime, bbIndex);
			for (auto it=bbValues.begin(); it != bbValues.end(); it++)
			{
				// override old values, bb track with the highest index takes precedence
				valueMap[it.key()] = it.value();
			}
		}
		else
		{
			continue;
		}
	}

	return valueMap;
};



DummyTrackContainer::DummyTrackContainer() :
	TrackContainer(),
	m_dummyInstrumentTrack( NULL )
{
	setJournalling( false );
	m_dummyInstrumentTrack = dynamic_cast<InstrumentTrack *>(
				Track::create( Track::InstrumentTrack,
							this ) );
	m_dummyInstrumentTrack->setJournalling( false );
}

