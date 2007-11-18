#ifndef SINGLE_SOURCE_COMPILE

/*
 * tool.cpp - base class for all tool plugins (graphs, extensions, etc)
 *
 * Copyright (c) 2006-2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "tool.h"

#include <QtGui/QIcon>
#include <QtGui/QMdiArea>

#include "main_window.h"




tool::tool( mainWindow * _window, const descriptor * _descriptor ) :
	QWidget( _window->workspace() ),
	plugin( _descriptor )
{
	setWindowTitle( _descriptor->public_name );
	setWindowIcon( *_descriptor->logo );
}




tool::~tool()
{
}




tool * tool::instantiate( const QString & _plugin_name, mainWindow * _window )
{
	plugin * p = plugin::instantiate( _plugin_name, _window );
	// check whether instantiated plugin is an instrument
	if( dynamic_cast<tool *>( p ) != NULL )
	{
		// everything ok, so return pointer
		return( dynamic_cast<tool *>( p ) );
	}

	// not quite... so delete plugin
	delete p;
	return( NULL );
}




#endif
