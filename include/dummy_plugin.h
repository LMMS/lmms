/*
 * dummy_plugin.h - empty plugin which is used as fallback if a plugin wasn't
 *                  found
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifndef _DUMMY_PLUGIN_H
#define _DUMMY_PLUGIN_H

#include "plugin.h"


class dummyPlugin : public plugin
{
public:
	inline dummyPlugin( void ) :
		plugin( "Dummy plugin", plugin::UNDEFINED )
	{
	}

	inline virtual ~dummyPlugin()
	{
	}


	inline virtual void saveSettings( QDomDocument &, QDomElement & )
	{
	}

	inline virtual void loadSettings( const QDomElement & )
	{
	}

	inline virtual QString nodeName( void ) const
	{
		return( "dummyplugin" );
	}

} ;


#endif
