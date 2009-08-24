/*
 * JournallingObject.h - declaration of class JournallingObject
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

#ifndef _JOURNALLING_OBJECT_H
#define _JOURNALLING_OBJECT_H

#include "lmms_basics.h"
#include "export.h"
#include "SerializingObject.h"

#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtCore/QStack>


typedef uint32_t t_action_id;


class JournalEntry
{
public:
	JournalEntry( const t_action_id _action_id, const QVariant & _data ) :
		m_actionID( _action_id ),
		m_data( _data )
	{
	}

	JournalEntry() :
		m_actionID( 0 ),
		m_data( 0 )
	{
	}

	~JournalEntry()
	{
	}

	t_action_id actionID() const
	{
		return m_actionID;
	}
	
	t_action_id & actionID()
	{
		return m_actionID;
	}
	
	const QVariant & data() const
	{
		return m_data;
	}

	QVariant & data()
	{
		return m_data;
	}


private:
	t_action_id m_actionID;
	QVariant m_data;

} ;


typedef QVector<JournalEntry> JournalEntryVector;


class EXPORT JournallingObject : public SerializingObject
{
public:
	JournallingObject();
	virtual ~JournallingObject();

	inline jo_id_t id() const
	{
		return m_id;
	}

	void undo();
	void redo();

	void clear()
	{
		m_journalEntries.clear();
		m_currentJournalEntry = m_journalEntries.end();
	}

	void clearRedoSteps()
	{
		m_journalEntries.erase( m_currentJournalEntry,
						m_journalEntries.end() );
		m_currentJournalEntry = m_journalEntries.end();
		
	}

	void saveJournallingState( const bool _new_state )
	{
		m_journallingStateStack.push( m_journalling );
		m_journalling = _new_state;
	}

	void restoreJournallingState()
	{
		m_journalling = m_journallingStateStack.pop();
	}

	virtual QDomElement saveState( QDomDocument & _doc,
							QDomElement & _parent );

	virtual void restoreState( const QDomElement & _this );


	inline bool isJournalling() const
	{
		return m_journalling;
	}

	inline void setJournalling( const bool _sr )
	{
		m_journalling = _sr;
	}

	inline bool testAndSetJournalling( const bool _sr )
	{
		const bool oldJournalling = m_journalling;
		m_journalling = _sr;
		return oldJournalling;
	}


protected:
	void changeID( jo_id_t _id );

	void addJournalEntry( const JournalEntry & _je );

	// to be implemented by sub-objects
	virtual void undoStep( JournalEntry & )
	{
	}
	virtual void redoStep( JournalEntry & )
	{
	}


private:
	void saveJournal( QDomDocument & _doc, QDomElement & _parent );
	void loadJournal( const QDomElement & _this );


	jo_id_t m_id;

	JournalEntryVector m_journalEntries;
	JournalEntryVector::Iterator m_currentJournalEntry;

	bool m_journalling;

	QStack<bool> m_journallingStateStack;

} ;


#endif

