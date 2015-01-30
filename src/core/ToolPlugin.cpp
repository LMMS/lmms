/*
 * ToolPlugin.cpp - base class for all tool plugins (graphs, extensions, etc)
 *
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ToolPlugin.h"


ToolPlugin::ToolPlugin( const Descriptor * _descriptor, Model * _parent ) :
	Plugin( _descriptor, _parent )
{
}




ToolPlugin::~ToolPlugin()
{
}




ToolPlugin * ToolPlugin::instantiate( const QString & _plugin_name, Model * _parent )
{
	Plugin * p = Plugin::instantiate( _plugin_name, _parent, NULL );
	// check whether instantiated plugin is a tool
	if( p->type() == Plugin::Tool )
	{
		// everything ok, so return pointer
		return dynamic_cast<ToolPlugin *>( p );
	}

	// not quite... so delete plugin
	delete p;
	return NULL;
}

