/*
 * ProjectJournal.cpp - implementation of ProjectJournal
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <cstdlib>

#include "ProjectJournal.h"
#include "Engine.h"
#include "JournallingObject.h"
#include "Song.h"

namespace lmms
{

const int ProjectJournal::MAX_UNDO_STATES = 100; // TODO: make this configurable in settings

ProjectJournal::ProjectJournal() :
	m_joIDs(),
	m_undoCheckPoints(),
	m_redoCheckPoints(),
	m_journalling( false )
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
			Engine::getSong()->setModified();
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
			Engine::getSong()->setModified();
			break;
		}
	}
}

bool ProjectJournal::canUndo() const
{
	return !m_undoCheckPoints.isEmpty();
}

bool ProjectJournal::canRedo() const
{
	return !m_redoCheckPoints.isEmpty();
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
	jo_id_t id = Uuid::RandomUuid();
	m_joIDs[id] = _obj;
	return id;
}




void ProjectJournal::reallocID( const jo_id_t _id, JournallingObject * _obj )
{
	m_joIDs[_id] = _obj;
}


void ProjectJournal::clearJournal()
{
	m_undoCheckPoints.clear();
	m_redoCheckPoints.clear();

	for( auto it = m_joIDs.begin(); it != m_joIDs.end(); )
	{
		if( it->second == nullptr )
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
	for(auto& m_joID : m_joIDs)
	{
		if( m_joID.second != nullptr )
		{
			m_joID.second->setJournalling(false);
		}
	}
	setJournalling(false);
}



} // namespace lmms
