/*
 * lv2_browser.h - dialog to display information about installed LV2 plugins
 *
 * Copyright (c) 2009 Martin Andrews <mdda/at/users.sourceforge.net>
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


#ifndef _LV2_BROWSER_H
#define _LV2_BROWSER_H


#include "lv2_manager.h"
#include "ToolPlugin.h"
#include "ToolPluginView.h"


class tabBar;


class lv2BrowserView : public ToolPluginView
{
	Q_OBJECT
public:
	lv2BrowserView( ToolPlugin * _tool );
	virtual ~lv2BrowserView();


public slots:
	void showPorts( const lv2_key_t & _key );


private:
	tabBar * m_tabBar;

	QWidget * createTab( QWidget * _parent, const QString & _txt,
						lv2PluginType _type );

} ;


class lv2Browser : public ToolPlugin
{
public:
	lv2Browser( void );
	virtual ~lv2Browser();

	virtual PluginView * instantiateView( QWidget * )
	{
		return new lv2BrowserView( this );
	}

	virtual QString nodeName( void ) const;

} ;


#endif
