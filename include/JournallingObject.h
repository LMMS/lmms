/*
 * JournallingObject.h - declaration of class JournallingObject
 *
 * Copyright (c) 2006-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_JOURNALLING_OBJECT_H
#define LMMS_JOURNALLING_OBJECT_H

#include <QStack>

#include "LmmsTypes.h"
#include "SerializingObject.h"

namespace lmms
{

class LMMS_EXPORT JournallingObject : public SerializingObject
{
public:
	JournallingObject();
	~JournallingObject() override;

	inline jo_id_t id() const
	{
		return m_id;
	}

	void saveJournallingState( const bool newState )
	{
		m_journallingStateStack.push( m_journalling );
		m_journalling = newState;
	}

	void restoreJournallingState()
	{
		if( !isJournallingStateStackEmpty())
		{
			m_journalling = m_journallingStateStack.pop();
		}
	}

	void addJournalCheckPoint();

	QDomElement saveState( QDomDocument & _doc,
									QDomElement & _parent ) override;

	void restoreState( const QDomElement & _this ) override;

	inline bool isJournalling() const
	{
		return m_journalling;
	}

	inline void setJournalling( const bool _sr )
	{
		m_journalling = _sr;
	}

	inline bool testAndSetJournalling( const bool newState )
	{
		const bool oldJournalling = m_journalling;
		m_journalling = newState;
		return oldJournalling;
	}

	bool isJournallingStateStackEmpty() const
	{
		return m_journallingStateStack.isEmpty();
	}

protected:
	void changeID( jo_id_t _id );


private:
	jo_id_t m_id;

	bool m_journalling;

	QStack<bool> m_journallingStateStack;

} ;


} // namespace lmms

#endif // LMMS_JOURNALLING_OBJECT_H
