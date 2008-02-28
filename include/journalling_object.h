/*
 * journalling_object.h - declaration of class journallingObject
 *
 * Copyright (c) 2006-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "types.h"

#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtCore/QStack>


class QDomDocument;
class QDomElement;


typedef uint32_t t_action_id;


class journallingObject;
class journallingObjectHook;


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


typedef QVector<journalEntry> journalEntryVector;


class journallingObject
{
public:
	journallingObject( void );
	virtual ~journallingObject();

	inline jo_id_t id( void ) const
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

	virtual QDomElement saveState( QDomDocument & _doc,
							QDomElement & _parent );

	virtual void restoreState( const QDomElement & _this );


	// to be implemented by actual object
	virtual QString nodeName( void ) const = 0;

	inline bool isJournalling( void ) const
	{
		return( m_journalling );
	}

	inline void setJournalling( const bool _sr )
	{
		m_journalling = _sr;
	}

	inline bool testAndSetJournalling( const bool _sr )
	{
		bool old_journalling = m_journalling;
		m_journalling = _sr;
		return( old_journalling );
	}

	void setHook( journallingObjectHook * _hook );

	journallingObjectHook * getHook( void )
	{
		return( m_hook );
	}


protected:
	void addJournalEntry( const journalEntry & _je );

	// to be implemented by sub-objects
	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this )
	{
	}

	virtual void loadSettings( const QDomElement & _this )
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

	QStack<bool> m_journallingStateStack;

	journallingObjectHook * m_hook;

} ;


class journallingObjectHook
{
public:
	journallingObjectHook() :
		m_hookedIn( NULL )
	{
	}
	virtual ~journallingObjectHook()
	{
		if( m_hookedIn != NULL )
		{
			m_hookedIn->setHook( NULL );
		}
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _this ) = 0;
	virtual void loadSettings( const QDomElement & _this ) = 0;

private:
	journallingObject * m_hookedIn;

	friend class journallingObject;

} ;


#endif

