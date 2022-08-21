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

#include "debug.h"
#include "ProjectJournal.h"
#include "Engine.h"
#include "JournallingObject.h"
#include "Song.h"

namespace lmms
{

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
		CheckPointBatch backup;

		// For every checkpoint (journaled object) in the last batch...
		for (CheckPoint& restorePoint: restoreStack.back())
		{
			JournallingObject* jo = journallingObject(restorePoint.joID);
			// If the object with this ID has been destroyed there's an implementation problem somewhere else
			if (!jo)
			{
				fprintf(stderr, "Cannot undo/redo changes on a deleted object\n");
				continue;
			}

			// Create a backup of the object's current state
			DataFile curState(DataFile::JournalData);
			jo->saveState( curState, curState.content() );
			backup.emplace_back(restorePoint.joID, curState);

			// Restore object to its previous state
			bool prev = isJournalling();
			setJournalling( false );
			jo->restoreState(restorePoint.data.content().firstChildElement());
			setJournalling( prev );
			Engine::getSong()->setModified();
		}
		restoreStack.pop_back();
		backupStack.push_back(std::move(backup));
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
		if (!isBatching() || m_undoCheckPoints.empty())
		{
			m_undoCheckPoints.emplace_back();
		}
		CheckPointBatch& batch = m_undoCheckPoints.back();

		// If this object already has a checkpoint in the batch, skip it
		for (const CheckPoint& checkpoint: batch)
		{
			if (checkpoint.joID == jo->id()) { return; }
		}

		// Create a checkpoint and save it to the batch
		DataFile dataFile( DataFile::JournalData );
		jo->saveState( dataFile, dataFile.content() );
		batch.emplace_back(jo->id(), dataFile);

		// Remove excessive checkpoints from the stack
		if( m_undoCheckPoints.size() > MAX_UNDO_STATES )
		{
			m_undoCheckPoints.erase(m_undoCheckPoints.begin(), m_undoCheckPoints.end() - MAX_UNDO_STATES);
		}
	}
}




/*! \brief Start grouping new checkpoints together in a batch
 *
 *  Returns a BatchActionGuard that must be kept for the whole scope of the batch action.
 *  If batching is already ongoing, checkpoints will be appended to the existing batch.
 *  Batching ends when all BatchActionGuards goes out of scope or are destroyed.
 *
 *  \param append - Don't begin on a new batch, append to the last one instead.
 *                  This is used as a workaround when we want add more checkpoints to a
 *                  checkpoint/batch that has already been created.
 */
ProjectJournal::BatchActionGuard ProjectJournal::beginBatchAction(bool append)
{
	if (!append && !isBatching()) { m_undoCheckPoints.emplace_back(); }

	++m_batchGuardCount;

	return BatchActionGuard(this);
}




/*! \brief Called by BatchActionGuard's destructor */
void ProjectJournal::batchGuardDestroyed()
{
	--m_batchGuardCount;

	// If the batch ends and no checkpoints were added, remove it from the undo vector
	if (m_batchGuardCount == 0 && m_undoCheckPoints.back().empty())
	{
		m_undoCheckPoints.pop_back();
	}
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



} // namespace lmms
