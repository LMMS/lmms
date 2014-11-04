/*
 * ToolPluginView.cpp - implementation of ToolPluginView
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
#include "ToolPluginView.h"

#include <QtGui/QIcon>
#include <QtGui/QMdiArea>

#include "embed.h"
#include "engine.h"
#include "MainWindow.h"


ToolPluginView::ToolPluginView( ToolPlugin * _toolPlugin ) :
	PluginView( _toolPlugin, NULL )
{
	engine::mainWindow()->workspace()->addSubWindow( this );
	parentWidget()->setAttribute( Qt::WA_DeleteOnClose, false );

	setWindowTitle( _toolPlugin->displayName() );
	setWindowIcon( _toolPlugin->descriptor()->logo->pixmap() );
}


