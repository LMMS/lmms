/*
 * ProjectJournal.h - declaration of class ProjectJournal
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_PROJECT_JOURNAL_H
#define LMMS_PROJECT_JOURNAL_H

#include <QHash>
#include <QStack>
#include <uuid.h>

#include "lmms_basics.h"
#include "DataFile.h"


namespace lmms
{


class JournallingObject;


//! @warning many parts of this class may be rewritten soon
class ProjectJournal
{
public:
	static const int MAX_UNDO_STATES;

	ProjectJournal();
	virtual ~ProjectJournal() = default;

	void undo();
	void redo();

	bool canUndo() const;
	bool canRedo() const;

	void addJournalCheckPoint(JournallingObject* jo);

	bool isJournalling() const
	{
		return m_journalling;
	}

	void setJournalling(const bool enabled)
	{
		m_journalling = enabled;
	}

	/**
	 * @brief alloc new ID and associate the given object with it
	 * @return New lookup key for the given object.
	 */
	jo_id_t allocID(JournallingObject* obj);

	// if there's already something known about ID _id, but it is currently
	// unused (e.g. after jouralling object was deleted), register object
	// _obj to this id
	void reallocID(jo_id_t id, JournallingObject* obj);

	// make ID _id unused, but keep all global journalling information
	// (order of journalling entries etc.) referring to _id - needed for
	// restoring a journalling object later
	void freeID(jo_id_t id)
	{
		reallocID(id, nullptr);
	}

	void clearJournal();
	void stopAllJournalling();
	JournallingObject* journallingObject(const jo_id_t id)
	{
		if(m_joIDs.count(id) != 0) //TODO Under C++20 can be replaced by (m_joIDs.contains(_id))
		{
			return m_joIDs[id];
		}
		return nullptr;
	}


private:
//	using JoIdMap = QHash<jo_id_t, JournallingObject*>;
	using JoIdMap = std::unordered_map<jo_id_t, JournallingObject*>;

	struct CheckPoint
	{
		explicit CheckPoint(jo_id_t initID = Uuid::NullUuid(), const DataFile& initData = DataFile(DataFile::JournalData)) :
			joID( initID ),
			data( initData )
		{}
		jo_id_t joID;
		DataFile data;
	} ;
	using CheckPointStack = QStack<CheckPoint>;

	JoIdMap m_joIDs;

	CheckPointStack m_undoCheckPoints;
	CheckPointStack m_redoCheckPoints;

	bool m_journalling;

} ;


} // namespace lmms

#endif // LMMS_PROJECT_JOURNAL_H
