/*
 * JournallingObject.cpp - implementation of journalling-object related stuff
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
	m_journalCheckPoints(),
	m_currentJournalCheckPoint( m_journalCheckPoints.end() ),
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
	if( m_journalCheckPoints.empty() == true )
	{
		return;
	}

	if( m_currentJournalCheckPoint - 1 >= m_journalCheckPoints.begin() )
	{
		--m_currentJournalCheckPoint;
		restoreState( m_currentJournalCheckPoint->data().content().firstChildElement() );
	}
}




void JournallingObject::redo()
{
	if( m_journalCheckPoints.empty() == true )
	{
		return;
	}

	if( m_currentJournalCheckPoint < m_journalCheckPoints.end() )
	{
		restoreState( m_currentJournalCheckPoint->data().content().firstChildElement() );
		++m_currentJournalCheckPoint;
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




void JournallingObject::addJournalCheckPoint()
{
	if( engine::projectJournal()->isJournalling() && isJournalling() )
	{
		m_journalCheckPoints.erase( m_currentJournalCheckPoint,
						m_journalCheckPoints.end() );

		multimediaProject mmp( multimediaProject::JournalData );
		saveState( mmp, mmp.content() );

		m_journalCheckPoints.push_back( JournalCheckPoint( mmp ) );

		m_currentJournalCheckPoint = m_journalCheckPoints.end();
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
	if( m_journalCheckPoints.size() == 0 )
	{
		return;
	}*/
	QDomElement journal_de = _doc.createElement( "journal" );
	journal_de.setAttribute( "id", id() );
	journal_de.setAttribute( "entries", m_journalCheckPoints.size() );
	journal_de.setAttribute( "curentry", (int)( m_currentJournalCheckPoint -
						m_journalCheckPoints.begin() ) );
	journal_de.setAttribute( "metadata", true );

	for( JournalCheckPointVector::const_iterator it = m_journalCheckPoints.begin();
					it != m_journalCheckPoints.end(); ++it )
	{
		QDomElement je_de = _doc.createElement( "entry" );
		je_de.setAttribute( "pos", (int)( it -
						m_journalCheckPoints.begin() ) );
		//je_de.setAttribute( "data", base64::encode( it->data().toString() ) );
		je_de.setAttribute( "data", it->data().toString() );
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

	m_journalCheckPoints.resize( _this.attribute( "entries" ).toInt() );

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			const QDomElement & je = node.toElement();
			m_journalCheckPoints[je.attribute( "pos" ).toInt()] =
				JournalCheckPoint(
					multimediaProject( je.attribute( "data" ).toUtf8() ) );
		}
		node = node.nextSibling();
	}

	m_currentJournalCheckPoint = m_journalCheckPoints.begin() +
					_this.attribute( "curentry" ).toInt();
}


