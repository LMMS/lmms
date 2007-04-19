#ifndef SINGLE_SOURCE_COMPILE

/*
 * journalling_object.cpp - implementation of journalling-object related stuff
 *
 * Copyright (c) 2006-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "journalling_object.h"
#include "project_journal.h"
#include "base64.h"
#include "engine.h"

#include "qt3support.h"

#ifndef QT3

#include <Qt/QtXml>

#else

#include <qdom.h>

#endif



journallingObject::journallingObject( void ) :
	m_id( engine::getProjectJournal()->allocID( this ) ),
	m_journalEntries(),
	m_currentJournalEntry( m_journalEntries.end() ),
	m_journalling( TRUE )
{
}




journallingObject::~journallingObject()
{
	if( engine::getProjectJournal() )
	{
		engine::getProjectJournal()->freeID( id() );
	}
}




void journallingObject::undo( void )
{
	printf("undo: %d\n", id() );
	if( m_journalEntries.empty() == TRUE )
	{
		return;
	}

	if( m_currentJournalEntry - 1 >= m_journalEntries.begin() )
	{
		undoStep( *--m_currentJournalEntry );
	}
}




void journallingObject::redo( void )
{
	printf("undo: %d\n", id() );
	if( m_journalEntries.empty() == TRUE )
	{
		return;
	}

	if( m_currentJournalEntry < m_journalEntries.end() )
	{
		redoStep( *m_currentJournalEntry++ );
	}
}




QDomElement journallingObject::saveState( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement _this = _doc.createElement( nodeName() );
	_parent.appendChild( _this );
	saveSettings( _doc, _this );
	saveJournal( _doc, _this );
	return( _this );
}




void journallingObject::restoreState( const QDomElement & _this )
{
	saveJournallingState( FALSE );

	// load actual settings
	loadSettings( _this );

	// search for journal-node
	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() && node.nodeName() == "journal" )
		{
			loadJournal( node.toElement() );
		}
		node = node.nextSibling();
        }

	restoreJournallingState();
}




void journallingObject::addJournalEntry( const journalEntry & _je )
{
	if( engine::getProjectJournal()->isJournalling() && isJournalling() )
	{
		m_journalEntries.erase( m_currentJournalEntry,
						m_journalEntries.end() );
		m_journalEntries.push_back( _je );
		m_currentJournalEntry = m_journalEntries.end();
		engine::getProjectJournal()->journalEntryAdded( id() );
	}
}




void journallingObject::saveJournal( QDomDocument & _doc,
							QDomElement & _parent )
{
/*	// avoid creating empty journal-nodes
	if( m_journalEntries.size() == 0 )
	{
		return;
	}*/
	QDomElement journal_de = _doc.createElement( "journal" );
	journal_de.setAttribute( "id", id() );
	journal_de.setAttribute( "entries", m_journalEntries.size() );
	journal_de.setAttribute( "curentry", m_currentJournalEntry -
						m_journalEntries.begin() );
	journal_de.setAttribute( "metadata", TRUE );

	for( journalEntryVector::const_iterator it = m_journalEntries.begin();
					it != m_journalEntries.end(); ++it )
	{
		QDomElement je_de = _doc.createElement( "entry" );
		je_de.setAttribute( "pos", it - m_journalEntries.begin() );
		je_de.setAttribute( "actionid", it->actionID() );
		je_de.setAttribute( "data", base64::encode( it->data() ) );
		journal_de.appendChild( je_de );
	}

	_parent.appendChild( journal_de );
}




void journallingObject::loadJournal( const QDomElement & _this )
{
	clear();

	const jo_id_t new_id = _this.attribute( "id" ).toInt();

	if( new_id == 0 )
	{
		return;
	}

	if( id() != new_id )
	{
		engine::getProjectJournal()->forgetAboutID( id() );
		engine::getProjectJournal()->reallocID( new_id, this );
		m_id = new_id;
	}

	m_journalEntries.resize( _this.attribute( "entries" ).toInt() );

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			const QDomElement & je = node.toElement();
			m_journalEntries[je.attribute( "pos" ).toInt()] =
				journalEntry(
					je.attribute( "actionid" ).toInt(),
				base64::decode( je.attribute( "data" ) ) );
		}
		node = node.nextSibling();
        }

	m_currentJournalEntry = m_journalEntries.begin() +
					_this.attribute( "curentry" ).toInt();
}



#endif
