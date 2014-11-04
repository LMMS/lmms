/*
 * InlineAutomation.h - class for automating something "inline"
 *
 * Copyright (c) 2008-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _INLINE_AUTOMATION_H
#define _INLINE_AUTOMATION_H

#include "AutomationPattern.h"
#include "shared_object.h"


class InlineAutomation : public FloatModel, public sharedObject
{
public:
	InlineAutomation() :
		FloatModel(),
		sharedObject(),
		m_autoPattern( NULL )
	{
	}

	virtual ~InlineAutomation()
	{
		if( m_autoPattern )
		{
			delete m_autoPattern;
		}
	}

	virtual float defaultValue() const = 0;

	bool hasAutomation() const
	{
		if( m_autoPattern != NULL && m_autoPattern->getTimeMap().isEmpty() == false )
		{
			// prevent saving inline automation if there's just one value which equals value
			// of model which is going to be saved anyways
			if( isAtInitValue() &&
				m_autoPattern->getTimeMap().size() == 1 &&
				m_autoPattern->getTimeMap().keys().first() == 0 &&
				m_autoPattern->getTimeMap().values().first() == value() )
			{
				return false;
			}

			return true;
		}

		return false;
	}

	AutomationPattern * automationPattern()
	{
		if( m_autoPattern == NULL )
		{
			m_autoPattern = new AutomationPattern( NULL );
			m_autoPattern->addObject( this );
		}
		return m_autoPattern;
	}

	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );


private:
	AutomationPattern * m_autoPattern;

} ;


#endif
