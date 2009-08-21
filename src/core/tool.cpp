/*
 * tool.cpp - base class for all tool plugins (graphs, extensions, etc)
 *
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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

#include "embed.h"
#include "engine.h"
#include "MainWindow.h"




tool::tool( const descriptor * _descriptor, model * _parent ) :
	plugin( _descriptor, _parent )
{
}




tool::~tool()
{
}




tool * tool::instantiate( const QString & _plugin_name, model * _parent )
{
	plugin * p = plugin::instantiate( _plugin_name, _parent, NULL );
	// check whether instantiated plugin is a tool
	if( p->type() == Tool )
	{
		// everything ok, so return pointer
		return( dynamic_cast<tool *>( p ) );
	}

	// not quite... so delete plugin
	delete p;
	return( NULL );
}





toolView::toolView( tool * _tool ) :
	pluginView( _tool, NULL )
{
	engine::mainWindow()->workspace()->addSubWindow( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, FALSE );

	setWindowTitle( _tool->displayName() );
	setWindowIcon( _tool->getDescriptor()->logo->pixmap() );
}


