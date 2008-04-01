/*
 * track_container.cpp - implementation of base-class for all track-containers
 *                       like Song-Editor, BB-Editor...
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


#include "track_container.h"


#include <QtGui/QApplication>
#include <QtGui/QProgressDialog>


#include "engine.h"
#include "song.h"


trackContainer::trackContainer( void ) :
	model( NULL ),
	journallingObject(),
	m_tracks()
{
}




trackContainer::~trackContainer()
{
	while( !m_tracks.empty() )
	{
		delete m_tracks.takeLast();
	}
}




void trackContainer::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setTagName( classNodeName() );
	_this.setAttribute( "type", nodeName() );
// ### TODO
	//mainWindow::saveWidgetState( this, _this );

	// save settings of each track
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		m_tracks[i]->saveState( _doc, _this );
	}
}




void trackContainer::loadSettings( const QDomElement & _this )
{
	static QProgressDialog * pd = NULL;
	bool was_null = ( pd == NULL );
	int start_val = 0;
	if( pd == NULL )
	{
		pd = new QProgressDialog( tr( "Loading project..." ),
						tr( "Cancel" ), 0,
						_this.childNodes().count() );
		pd->setWindowModality( Qt::ApplicationModal );
		pd->setWindowTitle( tr( "Please wait..." ) );
		pd->show();
	}
	else
	{
		start_val = pd->value();
		pd->setMaximum( pd->maximum() + _this.childNodes().count() );
	}

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		pd->setValue( pd->value() + 1 );
		qApp->processEvents( QEventLoop::AllEvents, 100 );

		if( pd->wasCanceled() )
		{
			break;
		}

		if( node.isElement() &&
			!node.toElement().attribute( "metadata" ).toInt() )
		{
			track::create( node.toElement(), this );
		}
		node = node.nextSibling();
	}

// ### TODO
//	mainWindow::restoreWidgetState( this, _this );


	pd->setValue( start_val + _this.childNodes().count() );

	if( was_null )
	{
		delete pd;
		pd = NULL;
	}
}




void trackContainer::addTrack( track * _track )
{
	if( _track->type() != track::AutomationTrack )
	{
		m_tracks.push_back( _track );
		emit trackAdded( _track );
	}
}




void trackContainer::removeTrack( track * _track )
{
	int index = m_tracks.indexOf( _track );
	if( index != -1 )
	{
		m_tracks.removeAt( index );

		if( engine::getSong() )
		{
			engine::getSong()->setModified();
		}
	}
}





void trackContainer::updateAfterTrackAdd( void )
{
}





void trackContainer::clearAllTracks( void )
{
	while( !m_tracks.empty() )
	{
		delete m_tracks.takeLast();
	}
}




int trackContainer::countTracks( track::TrackTypes _tt ) const
{
	int cnt = 0;
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		if( m_tracks[i]->type() == _tt || _tt == track::NumTrackTypes )
		{
			++cnt;
		}
	}
	return( cnt );
}




void trackContainer::setMutedOfAllTracks( bool _muted )
{
	for( int i = 0; i < m_tracks.size(); ++i )
	{
		m_tracks[i]->setMuted( _muted );
	}
}



#include "track_container.moc"

