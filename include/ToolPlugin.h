/*
 * ToolPlugin.h - declaration of class ToolPlugin, a standard interface for all
 *                tool plugins
 *
 * Copyright (c) 2006-2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _TOOL_PLUGIN_H
#define _TOOL_PLUGIN_H

#include "Plugin.h"

class EXPORT ToolPlugin : public Plugin
{
public:
	ToolPlugin( const Descriptor * _descriptor, Model * _parent );
	virtual ~ToolPlugin();

	// instantiate tool-plugin with given name or return NULL
	// on failure
	static ToolPlugin * instantiate( const QString & _plugin_name,
										Model * _parent );

} ;

#endif
