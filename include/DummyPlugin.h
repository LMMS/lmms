/*
 * DummyPlugin.h - empty plugin which is used as fallback if a plugin couldn't
 *                 be found
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef DUMMY_PLUGIN_H
#define DUMMY_PLUGIN_H

#include "Plugin.h"
#include "PluginView.h"


class DummyPlugin : public Plugin
{
public:
	DummyPlugin() :
		Plugin( NULL, NULL )
	{
	}

	virtual ~DummyPlugin()
	{
	}

	void saveSettings( QDomDocument &, QDomElement & ) override
	{
	}

	void loadSettings( const QDomElement & ) override
	{
	}

	QString nodeName() const override
	{
		return "DummyPlugin";
	}


protected:
	PluginView * instantiateView( QWidget * _parent ) override
	{
		return new PluginView( this, _parent );
	}

} ;


#endif
