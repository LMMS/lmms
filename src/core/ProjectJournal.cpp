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

//! Avoid clashes between loaded IDs (have the bit cleared)
//! and newly created IDs (have the bit set)
static const int EO_ID_MSB = 1 << 23;

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
	restoreCheckPoint(m_undoCheckPoints, m_redoCheckPoints);
}




void ProjectJournal::redo()
{
	restoreCheckPoint(m_redoCheckPoints, m_undoCheckPoints);
}




/*! \brief Take a backup of the current state before restoring the most recent checkpoint
 *
 *  \param restoreStack pop a checkpoint from this stack and restore it
 *  \param backupStack append a checkpoint of the current state to this stack
 */
void ProjectJournal::restoreCheckPoint(ProjectJournal::CheckPointStack& restoreStack,
										ProjectJournal::CheckPointStack& backupStack)
{
	while (!restoreStack.empty())
	{
		BatchCheckPoint backup;

		// For every checkpoint (journaled object) in the last group...
		for (CheckPoint& restorePoint: restoreStack.back())
		{
			JournallingObject* jo = journallingObject(restorePoint.joID);
			// TODO when can this be null?
			if (!jo) { continue; }

			// Create a backup of the object's current state
			DataFile curState(DataFile::JournalData);
			jo->saveState( curState, curState.content() );
			backup.push_back(CheckPoint(restorePoint.joID, curState));

			// Restore object to its previous state
			bool prev = isJournalling();
			setJournalling( false );
			jo->restoreState(restorePoint.data.content().firstChildElement());
			setJournalling( prev );
			Engine::getSong()->setModified();
		}
		restoreStack.pop_back();

		// Keep trying until something is restored
		if (backup.empty()) { continue; }

		backupStack.push_back(backup);
		return;
	}
}




bool ProjectJournal::canUndo() const
{
	return !m_undoCheckPoints.empty();
}




bool ProjectJournal::canRedo() const
{
	return !m_redoCheckPoints.empty();
}




void ProjectJournal::addJournalCheckPoint( JournallingObject *jo )
{
	if( isJournalling() )
	{
		m_redoCheckPoints.clear();

		// If we're not batching checkpoints, begin on a new one
		if (m_batchingCount == 0 || m_undoCheckPoints.empty())
		{
			m_undoCheckPoints.emplace_back();
		}
		BatchCheckPoint& batch = m_undoCheckPoints.back();

		// If this object already has a checkpoint in the batch, skip it
		for (const CheckPoint& checkpoint: batch)
		{
			if (checkpoint.joID == jo->id()) { return; }
		}

		// Create a checkpoint and save it to the batch
		DataFile dataFile( DataFile::JournalData );
		jo->saveState( dataFile, dataFile.content() );
		batch.push_back(CheckPoint(jo->id(), dataFile));

		// Remove excessive checkpoints from the stack
		if( m_undoCheckPoints.size() > MAX_UNDO_STATES )
		{
			m_undoCheckPoints.erase(m_undoCheckPoints.begin(), m_undoCheckPoints.end() - MAX_UNDO_STATES);
		}
	}
}




void ProjectJournal::beginBatchCheckPoint()
{
	if (!isJournalling()) { return; }
	// Only begin on a new batch if we are not already batching
	if (m_batchingCount == 0) { m_undoCheckPoints.emplace_back(); }
	++m_batchingCount;
}




void ProjectJournal::endBatchCheckPoint()
{
	if (!isJournalling()) { return; }
	// If no checkpoints were added to the batch, remove it
	if (m_undoCheckPoints.back().empty()) { m_undoCheckPoints.pop_back(); }
	--m_batchingCount;
	assert(m_batchStartCount >= 0);
}




jo_id_t ProjectJournal::allocID( JournallingObject * _obj )
{
	jo_id_t id;
	for( jo_id_t tid = rand(); m_joIDs.contains( id = tid % EO_ID_MSB
							| EO_ID_MSB ); tid++ )
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




jo_id_t ProjectJournal::idToSave( jo_id_t id )
{
	return id & ~EO_ID_MSB;
}

jo_id_t ProjectJournal::idFromSave( jo_id_t id )
{
	return id | EO_ID_MSB;
}




void ProjectJournal::clearJournal()
{
	m_undoCheckPoints.clear();
	m_redoCheckPoints.clear();

	for( JoIdMap::Iterator it = m_joIDs.begin(); it != m_joIDs.end(); )
	{
		if( it.value() == nullptr )
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
		if( it.value() != nullptr )
		{
			it.value()->setJournalling(false);
		}
	}
	setJournalling(false);
}



