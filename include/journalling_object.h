/*
 * journalling_object.h - declaration of class journallingObject
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


#ifndef _JOURNALLING_OBJECT_H
#define _JOURNALLING_OBJECT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "types.h"
#include "engine.h"
#include "qt3support.h"

#ifndef QT3

#include <QVariant>
#include <QVector>
#include <QStack>

#else

#include <qvariant.h>
#include <qvaluevector.h>
#include <qvaluestack.h>

#endif


class QDomDocument;
class QDomElement;


typedef Uint32 t_action_id;


class journallingObject;


class journalEntry
{
public:
	journalEntry( const t_action_id _action_id, const QVariant & _data ) :
		m_actionID( _action_id ),
		m_data( _data )
	{
	}

	journalEntry( void ) :
		m_actionID( 0 ),
		m_data( 0 )
	{
	}

	~journalEntry()
	{
	}

	t_action_id actionID( void ) const
	{
		return( m_actionID );
	}
	
	t_action_id & actionID( void )
	{
		return( m_actionID );
	}
	
	const QVariant & data( void ) const
	{
		return( m_data );
	}

	QVariant & data( void )
	{
		return( m_data );
	}


private:
	t_action_id m_actionID;
	QVariant m_data;

} ;


typedef vvector<journalEntry> journalEntryVector;



class journallingObject : public engineObject
{
public:
	journallingObject( engine * _engine );
	virtual ~journallingObject();

	inline const jo_id_t id( void ) const
	{
		return( m_id );
	}

	void undo( void );
	void redo( void );

	void clear( void )
	{
		m_journalEntries.clear();
		m_currentJournalEntry = m_journalEntries.end();
	}

	void clearRedoSteps( void )
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

	void restoreJournallingState( void )
	{
		m_journalling = m_journallingStateStack.pop();
	}

	virtual QDomElement FASTCALL saveState( QDomDocument & _doc,
							QDomElement & _parent );

	virtual void FASTCALL restoreState( const QDomElement & _this );


	// to be implemented by actual object
	virtual QString nodeName( void ) const = 0;


protected:
	void addJournalEntry( const journalEntry & _je );

	inline bool isJournalling( void ) const
	{
		return( m_journalling );
	}

	inline void setJournalling( const bool _sr )
	{
		m_journalling = _sr;
	}


	// to be implemented by sub-objects
	virtual void FASTCALL saveSettings( QDomDocument & _doc,
						QDomElement & _this )
	{
	}

	virtual void FASTCALL loadSettings( const QDomElement & _this )
	{
	}

	virtual void undoStep( journalEntry & _je )
	{
	}
	virtual void redoStep( journalEntry & _je )
	{
	}


private:
	void saveJournal( QDomDocument & _doc, QDomElement & _parent );
	void loadJournal( const QDomElement & _this );


	jo_id_t m_id;

	journalEntryVector m_journalEntries;
	journalEntryVector::iterator m_currentJournalEntry;

	bool m_journalling;

	vstack<bool> m_journallingStateStack;

} ;



#endif

