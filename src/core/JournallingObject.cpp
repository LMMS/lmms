/*
 * JournallingObject.cpp - implementation of journalling-object related stuff
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

#include <QtXml/QDomElement>

#include <cstdio>

#include "JournallingObject.h"
#include "AutomatableModel.h"
#include "ProjectJournal.h"
#include "base64.h"
#include "engine.h"



JournallingObject::JournallingObject() :
	SerializingObject(),
	m_id( engine::projectJournal()->allocID( this ) ),
	m_journalEntries(),
	m_currentJournalEntry( m_journalEntries.end() ),
	m_journalling( true ),
	m_journallingStateStack()
{
}




JournallingObject::~JournallingObject()
{
	if( engine::projectJournal() )
	{
		engine::projectJournal()->freeID( id() );
	}
}




void JournallingObject::undo()
{
	if( m_journalEntries.empty() == true )
	{
		return;
	}

	if( m_currentJournalEntry - 1 >= m_journalEntries.begin() )
	{
		undoStep( *--m_currentJournalEntry );
	}
}




void JournallingObject::redo()
{
	if( m_journalEntries.empty() == true )
	{
		return;
	}

	if( m_currentJournalEntry < m_journalEntries.end() )
	{
		redoStep( *m_currentJournalEntry++ );
	}
}




QDomElement JournallingObject::saveState( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement _this = SerializingObject::saveState( _doc, _parent );
	saveJournal( _doc, _this );
	return _this;
}




void JournallingObject::restoreState( const QDomElement & _this )
{
	SerializingObject::restoreState( _this );

	saveJournallingState( false );

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




void JournallingObject::addJournalEntry( const JournalEntry & _je )
{
	if( engine::projectJournal()->isJournalling() && isJournalling() )
	{
		m_journalEntries.erase( m_currentJournalEntry,
						m_journalEntries.end() );
		m_journalEntries.push_back( _je );
		m_currentJournalEntry = m_journalEntries.end();
		engine::projectJournal()->journalEntryAdded( id() );
	}
}




void JournallingObject::changeID( jo_id_t _id )
{
	if( id() != _id )
	{
		JournallingObject * jo = engine::projectJournal()->
											journallingObject( _id );
		if( jo != NULL )
		{
			QString used_by = jo->nodeName();
			if( used_by == "automatablemodel" &&
				dynamic_cast<AutomatableModel *>( jo ) )
			{
				used_by += ":" +
					dynamic_cast<AutomatableModel *>( jo )->
								displayName();
			}
			fprintf( stderr, "JO-ID %d already in use by %s!\n",
				(int) _id, used_by.toUtf8().constData() );
			return;
		}
		engine::projectJournal()->forgetAboutID( id() );
		engine::projectJournal()->reallocID( _id, this );
		m_id = _id;
	}
}




void JournallingObject::saveJournal( QDomDocument & _doc,
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
	journal_de.setAttribute( "curentry", (int)( m_currentJournalEntry -
						m_journalEntries.begin() ) );
	journal_de.setAttribute( "metadata", true );

	for( JournalEntryVector::const_iterator it = m_journalEntries.begin();
					it != m_journalEntries.end(); ++it )
	{
		QDomElement je_de = _doc.createElement( "entry" );
		je_de.setAttribute( "pos", (int)( it -
						m_journalEntries.begin() ) );
		je_de.setAttribute( "actionid", it->actionID() );
		je_de.setAttribute( "data", base64::encode( it->data() ) );
		journal_de.appendChild( je_de );
	}

	_parent.appendChild( journal_de );
}




void JournallingObject::loadJournal( const QDomElement & _this )
{
	clear();

	const jo_id_t new_id = _this.attribute( "id" ).toInt();

	if( new_id == 0 )
	{
		return;
	}

	changeID( new_id );

	m_journalEntries.resize( _this.attribute( "entries" ).toInt() );

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			const QDomElement & je = node.toElement();
			m_journalEntries[je.attribute( "pos" ).toInt()] =
				JournalEntry(
					je.attribute( "actionid" ).toInt(),
				base64::decode( je.attribute( "data" ) ) );
		}
		node = node.nextSibling();
        }

	m_currentJournalEntry = m_journalEntries.begin() +
					_this.attribute( "curentry" ).toInt();
}


