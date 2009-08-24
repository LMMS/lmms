/*
 * ProjectJournal.cpp - implementation of ProjectJournal
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <cstdlib>

#include "ProjectJournal.h"
#include "engine.h"
#include "JournallingObject.h"
#include "song.h"


ProjectJournal::ProjectJournal() :
	m_joIDs(),
	m_journalEntries(),
	m_currentJournalEntry( m_journalEntries.end() ),
	m_journalling( false )
{
}




ProjectJournal::~ProjectJournal()
{
}




void ProjectJournal::undo()
{
	if( m_journalEntries.empty() == true )
	{
		return;
	}

	JournallingObject * jo;

	if( m_currentJournalEntry - 1 >= m_journalEntries.begin() &&
		( jo = m_joIDs[*--m_currentJournalEntry] ) != NULL )
	{
		jo->undo();
		engine::getSong()->setModified();
	}
}




void ProjectJournal::redo()
{
	if( m_journalEntries.empty() == true )
	{
		return;
	}

	JournallingObject * jo;

	//printf("%d\n", m_joIDs[*(m_currentJournalEntry+1)] );
	if( m_currentJournalEntry < m_journalEntries.end() &&
		( jo = m_joIDs[*m_currentJournalEntry++] ) != NULL )
	{
		jo->redo();
		engine::getSong()->setModified();
	}
}




void ProjectJournal::journalEntryAdded( const jo_id_t _id )
{
	m_journalEntries.erase( m_currentJournalEntry, m_journalEntries.end() );
	m_journalEntries.push_back( _id );
	m_currentJournalEntry = m_journalEntries.end();
	engine::getSong()->setModified();
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




void ProjectJournal::forgetAboutID( const jo_id_t _id )
{
	//printf("forget about %d\n", _id );
	JournalEntryVector::Iterator it;
	while( ( it = qFind( m_journalEntries.begin(), m_journalEntries.end(),
					_id ) ) != m_journalEntries.end() )
	{
		if( m_currentJournalEntry >= it )
		{
			--m_currentJournalEntry;
		}
		m_journalEntries.erase( it );
	}
	m_joIDs.remove( _id );
}




void ProjectJournal::clearJournal()
{
	m_journalEntries.clear();
	m_currentJournalEntry = m_journalEntries.end();
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



