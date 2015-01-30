/*
 * ProjectJournal.h - declaration of class ProjectJournal
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef PROJECT_JOURNAL_H
#define PROJECT_JOURNAL_H

#include <QtCore/QHash>
#include <QtCore/QStack>

#include "lmms_basics.h"
#include "DataFile.h"

class JournallingObject;


class ProjectJournal
{
public:
	static const int MAX_UNDO_STATES;

	ProjectJournal();
	virtual ~ProjectJournal();

	void undo();
	void redo();

	void addJournalCheckPoint( JournallingObject *jo );

	bool isJournalling() const
	{
		return m_journalling;
	}

	void setJournalling( const bool _on )
	{
		m_journalling = _on;
	}

	// alloc new ID and register object _obj to it
	jo_id_t allocID( JournallingObject * _obj );

	// if there's already something known about ID _id, but it is currently
	// unused (e.g. after jouralling object was deleted), register object
	// _obj to this id
	void reallocID( const jo_id_t _id, JournallingObject * _obj );

	// make ID _id unused, but keep all global journalling information
	// (order of journalling entries etc.) referring to _id - needed for
	// restoring a journalling object later
	void freeID( const jo_id_t _id )
	{
		reallocID( _id, NULL );
	}

	void clearJournal();
	void stopAllJournalling();
	JournallingObject * journallingObject( const jo_id_t _id )
	{
		if( m_joIDs.contains( _id ) )
		{
			return m_joIDs[_id];
		}
		return NULL;
	}


private:
	typedef QHash<jo_id_t, JournallingObject *> JoIdMap;

	struct CheckPoint
	{
		CheckPoint( jo_id_t initID = 0, const DataFile& initData = DataFile( DataFile::JournalData ) ) :
			joID( initID ),
			data( initData )
		{
		}
		jo_id_t joID;
		DataFile data;
	} ;
	typedef QStack<CheckPoint> CheckPointStack;

	JoIdMap m_joIDs;

	CheckPointStack m_undoCheckPoints;
	CheckPointStack m_redoCheckPoints;

	bool m_journalling;

} ;


#endif

