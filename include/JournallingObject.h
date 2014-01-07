/*
 * JournallingObject.h - declaration of class JournallingObject
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

#ifndef _JOURNALLING_OBJECT_H
#define _JOURNALLING_OBJECT_H

#include "lmms_basics.h"
#include "export.h"
#include "mmp.h"
#include "SerializingObject.h"

#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtCore/QStack>


class JournalCheckPoint
{
public:
	JournalCheckPoint( const multimediaProject &data =
						multimediaProject( multimediaProject::JournalData ) ) :
		m_data( data )
	{
	}

	~JournalCheckPoint()
	{
	}

	const multimediaProject &data() const
	{
		return m_data;
	}

	multimediaProject &data()
	{
		return m_data;
	}


private:
	multimediaProject m_data;

} ;


typedef QVector<JournalCheckPoint> JournalCheckPointVector;


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
		m_journalCheckPoints.clear();
		m_currentJournalCheckPoint = m_journalCheckPoints.end();
	}

	void clearRedoSteps()
	{
		m_journalCheckPoints.erase( m_currentJournalCheckPoint,
						m_journalCheckPoints.end() );
		m_currentJournalCheckPoint = m_journalCheckPoints.end();
		
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

	void addJournalCheckPoint();

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


private:
	void saveJournal( QDomDocument & _doc, QDomElement & _parent );
	void loadJournal( const QDomElement & _this );


	jo_id_t m_id;

	JournalCheckPointVector m_journalCheckPoints;
	JournalCheckPointVector::Iterator m_currentJournalCheckPoint;

	bool m_journalling;

	QStack<bool> m_journallingStateStack;

} ;


#endif

