#ifndef SINGLE_SOURCE_COMPILE

/*
 * clipboard.cpp - the clipboard for patterns, notes etc.
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "clipboard.h"
#include "journalling_object.h"


namespace clipboard
{

	map content;


	void copy( journallingObject * _obj )
	{
		QDomDocument doc;
		QDomElement parent = doc.createElement( "clipboard" );
		_obj->saveState( doc, parent );
		content[_obj->nodeName()] = parent.firstChild().toElement();
	}




	const QDomElement * getContent( const QString & _node_name )
	{
		if( content.find( _node_name ) != content.end() )
		{
			return( &content[_node_name] );
		}
		return( NULL );
	}

} ;



#endif
