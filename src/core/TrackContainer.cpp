/*
 * TrackContainer.cpp - implementation of base class for all trackcontainers
 *                      like Song-Editor, BB-Editor...
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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


#include <QtGui/QApplication>
#include <QtGui/QProgressDialog>
#include <QtXml/QDomElement>

#include "TrackContainer.h"
#include "InstrumentTrack.h"
#include "engine.h"
#include "MainWindow.h"
#include "song.h"


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
	int start_val = 0;
	if( !journalRestore && engine::hasGUI() )
	{
		if( pd == NULL )
		{
			pd = new QProgressDialog( tr( "Loading project..." ),
						tr( "Cancel" ), 0,
						_this.childNodes().count(),
						engine::mainWindow() );
			pd->setWindowModality( Qt::ApplicationModal );
			pd->setWindowTitle( tr( "Please wait..." ) );
			pd->show();
		}
		else
		{
			start_val = pd->value();
			pd->setMaximum( pd->maximum() +
						_this.childNodes().count() );
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
				break;
			}
		}

		if( node.isElement() &&
			!node.toElement().attribute( "metadata" ).toInt() )
		{
			track::create( node.toElement(), this );
		}
		node = node.nextSibling();
	}

	if( pd != NULL )
	{
		pd->setValue( start_val + _this.childNodes().count() );
		if( was_null )
		{
			delete pd;
			pd = NULL;
		}
	}
}




int TrackContainer::countTracks( track::TrackTypes _tt ) const
{
	int cnt = 0;
	m_tracksMutex.lockForRead();
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		if( m_tracks[i]->type() == _tt || _tt == track::NumTrackTypes )
		{
			++cnt;
		}
	}
	m_tracksMutex.unlock();
	return( cnt );
}




void TrackContainer::addTrack( track * _track )
{
	if( _track->type() != track::HiddenAutomationTrack )
	{
		m_tracksMutex.lockForWrite();
		m_tracks.push_back( _track );
		m_tracksMutex.unlock();
		emit trackAdded( _track );
	}
}




void TrackContainer::removeTrack( track * _track )
{
	int index = m_tracks.indexOf( _track );
	if( index != -1 )
	{
		// If the track is solo, all other tracks are muted. Change this before removing the solo track:
		if (_track->isSolo()) {
			_track->setSolo(false);
		}
		m_tracksMutex.lockForWrite();
		m_tracks.remove( index );
		m_tracksMutex.unlock();

		if( engine::getSong() )
		{
			engine::getSong()->setModified();
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






DummyTrackContainer::DummyTrackContainer() :
	TrackContainer(),
	m_dummyInstrumentTrack( NULL )
{
	setJournalling( false );
	m_dummyInstrumentTrack = dynamic_cast<InstrumentTrack *>(
				track::create( track::InstrumentTrack,
							this ) );
	m_dummyInstrumentTrack->setJournalling( false );
}




#include "moc_TrackContainer.cxx"

