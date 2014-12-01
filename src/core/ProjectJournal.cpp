/*
 * ProjectJournal.cpp - implementation of ProjectJournal
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <cstdlib>

#include "ProjectJournal.h"
#include "engine.h"
#include "JournallingObject.h"
#include "song.h"

const int ProjectJournal::MAX_UNDO_STATES = 100; // TODO: make this configurable in settings

ProjectJournal::ProjectJournal() :
	m_joIDs(),
	m_undoCheckPoints(),
	m_redoCheckPoints(),
	m_journalling( false )
{
}




ProjectJournal::~ProjectJournal()
{
}




void ProjectJournal::undo()
{
	while( !m_undoCheckPoints.isEmpty() )
	{
		CheckPoint c = m_undoCheckPoints.pop();
		JournallingObject *jo = m_joIDs[c.joID];

		if( jo )
		{
			DataFile curState( DataFile::JournalData );
			jo->saveState( curState, curState.content() );
			m_redoCheckPoints.push( CheckPoint( c.joID, curState ) );

			bool prev = isJournalling();
			setJournalling( false );
			jo->restoreState( c.data.content().firstChildElement() );
			setJournalling( prev );
			engine::getSong()->setModified();
			break;
		}
	}
}



void ProjectJournal::redo()
{
	while( !m_redoCheckPoints.isEmpty() )
	{
		CheckPoint c = m_redoCheckPoints.pop();
		JournallingObject *jo = m_joIDs[c.joID];

		if( jo )
		{
			DataFile curState( DataFile::JournalData );
			jo->saveState( curState, curState.content() );
			m_undoCheckPoints.push( CheckPoint( c.joID, curState ) );

			bool prev = isJournalling();
			setJournalling( false );
			jo->restoreState( c.data.content().firstChildElement() );
			setJournalling( prev );
			engine::getSong()->setModified();
			break;
		}
	}
}




void ProjectJournal::addJournalCheckPoint( JournallingObject *jo )
{
	if( isJournalling() )
	{
		m_redoCheckPoints.clear();

		DataFile dataFile( DataFile::JournalData );
		jo->saveState( dataFile, dataFile.content() );

		m_undoCheckPoints.push( CheckPoint( jo->id(), dataFile ) );
		if( m_undoCheckPoints.size() > MAX_UNDO_STATES )
		{
			m_undoCheckPoints.remove( 0, m_undoCheckPoints.size() - MAX_UNDO_STATES );
		}
	}
}




jo_id_t ProjectJournal::allocID( JournallingObject * _obj )
{
	const jo_id_t EO_ID_MAX = (1 << 23)-1;
	jo_id_t id;
	while( m_joIDs.contains( id =
			static_cast<jo_id_t>( (jo_id_t)rand()*(jo_id_t)rand() %
								 EO_ID_MAX ) ) )
	{
	}

	m_joIDs[id] = _obj;
	//printf("new id: %d\n", id );
	return id;
}




void ProjectJournal::reallocID( const jo_id_t _id, JournallingObject * _obj )
{
	//printf("realloc %d %d\n", _id, _obj );
//	if( m_joIDs.contains( _id ) )
	{
		m_joIDs[_id] = _obj;
	}
}




void ProjectJournal::clearJournal()
{
	m_undoCheckPoints.clear();
	m_redoCheckPoints.clear();

	for( JoIdMap::Iterator it = m_joIDs.begin(); it != m_joIDs.end(); )
	{
		if( it.value() == NULL )
		{
			it = m_joIDs.erase( it );
		}
		else
		{
			++it;
		}
	}
}

void ProjectJournal::stopAllJournalling()
{
	for( JoIdMap::Iterator it = m_joIDs.begin(); it != m_joIDs.end(); ++it)
	{
		if( it.value() != NULL ) 
		{
			it.value()->setJournalling(false);
		}
	}
	setJournalling(false);
}



