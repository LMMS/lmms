/*
 * PluginView.h - declaration of class PluginView
 *
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

#ifndef _PLUGIN_VIEW_H
#define _PLUGIN_VIEW_H

#include <QtGui/QWidget>

#include "Plugin.h"
#include "ModelView.h"


class EXPORT PluginView  : public QWidget, public ModelView
{
public:
	PluginView( Plugin * _plugin, QWidget * _parent ) :
		QWidget( _parent ),
		ModelView( _plugin, this )
	{
	}

} ;


#endif
