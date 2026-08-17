/*
 * SerializingObject.cpp - implementation of SerializingObject
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QDomDocument>
#include <QDomElement>

#include "SerializingObject.h"

namespace lmms
{

SerializingObject::SerializingObject() :
	m_hook( nullptr )
{
}




SerializingObject::~SerializingObject()
{
	if( m_hook )
	{
		m_hook->m_hookedIn = nullptr;
	}
}




QDomElement SerializingObject::saveState( QDomDocument& doc, QDomElement& parent )
{
	QDomElement element = doc.createElement( nodeName() );
	parent.appendChild( element );

	saveSettings( doc, element );

	if( hook() )
	{
		hook()->saveSettings( doc, element );
	}
	else
	{
		for (auto it = m_deferredHookAttributes.cbegin(); it != m_deferredHookAttributes.cend(); ++it)
		{
			element.setAttribute(it.key(), it.value());
		}
	}

	return element;
}




void SerializingObject::restoreState( const QDomElement& element )
{
	loadSettings( element );
	m_deferredHookAttributes.clear();

	if( hook() )
	{
		hook()->loadSettings( element );
	}
	else
	{
		for (const auto& name : m_hookAttributeNames)
		{
			if (element.hasAttribute(name))
			{
				m_deferredHookAttributes.insert(name, element.attribute(name));
			}
		}
	}
}




void SerializingObject::setHook( SerializingObjectHook* hook )
{
	if( m_hook )
	{
		m_hook->m_hookedIn = nullptr;
	}

	m_hook = hook;

	if( m_hook )
	{
		m_hook->m_hookedIn = this;

		if (!m_deferredHookAttributes.isEmpty())
		{
			QDomDocument doc;
			QDomElement element = doc.createElement(nodeName());
			doc.appendChild(element);
			for (auto it = m_deferredHookAttributes.cbegin(); it != m_deferredHookAttributes.cend(); ++it)
			{
				element.setAttribute(it.key(), it.value());
			}
			m_deferredHookAttributes.clear();
			m_hook->loadSettings(element);
		}
	}
}




QString SerializingObject::deferredHookAttribute(const QString& name) const
{
	return m_deferredHookAttributes.value(name);
}




void SerializingObject::setHookAttributeNames(const QStringList& names)
{
	m_hookAttributeNames = names;
}




void SerializingObject::saveSettings( QDomDocument& doc, QDomElement& element )
{
	Q_UNUSED(doc)
	Q_UNUSED(element)
}




void SerializingObject::loadSettings( const QDomElement& element )
{
	Q_UNUSED(element)
}

} // namespace lmms
