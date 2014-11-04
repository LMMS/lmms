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

#include <QtXml/QDomElement>

#include "InlineAutomation.h"


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
		}
	}
}

