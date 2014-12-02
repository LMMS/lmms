/*
 * InlineAutomation.cpp - class for automating something "inline"
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

#include <QDomElement>

#include "InlineAutomation.h"
#include "AutomationPattern.h"
#include "MidiTime.h"

InlineAutomation::InlineAutomation() :
		FloatModel(),
		sharedObject(),
		m_autoPattern( NULL )
{
}


InlineAutomation::~InlineAutomation()
{
	if( m_autoPattern )
	{
		delete m_autoPattern;
	}
}


bool InlineAutomation::hasAutomation() const
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


AutomationPattern * InlineAutomation::automationPattern()
{
	if( m_autoPattern == NULL )
	{
		m_autoPattern = new AutomationPattern( NULL, MidiTime( 0 ) );
		m_autoPattern->addInlineObject( this );
	}
	return m_autoPattern;
}


void InlineAutomation::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	if( hasAutomation() )
	{
		QDomElement ap = _doc.createElement(
					AutomationPattern::classNodeName() );
		QDomElement v = _doc.createElement( nodeName() );
		automationPattern()->saveSettings( _doc, v );
		ap.appendChild( v );
		_parent.appendChild( ap );
	}
}




void InlineAutomation::loadSettings( const QDomElement & _this )
{
	QDomNode node = _this.namedItem( AutomationPattern::classNodeName() );
	if( node.isElement() )
	{
		node = node.namedItem( nodeName() );
		if( node.isElement() )
		{
			automationPattern()->loadSettings(
							node.toElement() );
			automationPattern()->addInlineObject( this );
		}
	}
}

