/*
 * project_journal.h - declaration of class projectJournal
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _PROJECT_JOURNAL_H
#define _PROJECT_JOURNAL_H

#include "types.h"
#include "engine.h"
#include "qt3support.h"

#ifndef QT3

#include <QVariant>
#include <QVector>
#include <QMap>

#else

#include <qvariant.h>
#include <qvaluevector.h>
#include <qmap.h>

#endif


class journallingObject;


class projectJournal : public engineObject
{
public:
	projectJournal( engine * _engine );
	virtual ~projectJournal();

	void undo( void );
	void redo( void );

	// tell history that a new journal entry was added to object with ID _id
	void journalEntryAdded( const jo_id_t _id );

	bool isJournalling( void ) const
	{
		return( m_journalling );
	}

	void setJournalling( const bool _on )
	{
		m_journalling = _on;
	}

	// alloc new ID and register object _obj to it
	jo_id_t allocID( journallingObject * _obj );

	// if there's already something known about ID _id, but it is currently
	// unused (e.g. after jouralling object was deleted), register object
	// _obj to this id
	void reallocID( const jo_id_t _id, journallingObject * _obj );

	// make ID _id unused, but keep all global journalling information
	// (order of journalling entries etc.) referring to _id - needed for
	// restoring a journalling object later
	void freeID( const jo_id_t _id )
	{
		reallocID( _id, NULL );
	}

	// completely remove everything linked with ID _id - all global
	// journalling information about the ID get's lost
	void forgetAboutID( const jo_id_t _id );

	void clear( void );

	journallingObject * getJournallingObject( const jo_id_t _id )
	{
		if( m_joIDs.contains( _id ) )
		{
			return( m_joIDs[_id] );
		}
		return( NULL );
	}


private:
	typedef QMap<jo_id_t, journallingObject *> joIDMap;
	typedef vvector<jo_id_t> journalEntryVector;

	joIDMap m_joIDs;

	journalEntryVector m_journalEntries;
	journalEntryVector::iterator m_currentJournalEntry;

	bool m_journalling;

} ;


#endif

