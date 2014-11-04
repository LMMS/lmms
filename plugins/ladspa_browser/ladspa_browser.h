/*
 * ladspa_browser.h - dialog to display information about installed LADSPA
 *					plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
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

#ifndef _LADSPA_BROWSER_H
#define _LADSPA_BROWSER_H

#include "ladspa_manager.h"
#include "ToolPlugin.h"
#include "ToolPluginView.h"

class tabBar;


class ladspaBrowserView : public ToolPluginView
{
	Q_OBJECT
public:
	ladspaBrowserView( ToolPlugin * _tool );
	virtual ~ladspaBrowserView();


public slots:
	void showPorts( const ladspa_key_t & _key );


private:
	tabBar * m_tabBar;

	QWidget * createTab( QWidget * _parent, const QString & _txt,
						ladspaPluginType _type );

} ;


class ladspaBrowser : public ToolPlugin
{
public:
	ladspaBrowser();
	virtual ~ladspaBrowser();

	virtual PluginView * instantiateView( QWidget * )
	{
		return new ladspaBrowserView( this );
	}

	virtual QString nodeName() const;

	virtual void saveSettings( QDomDocument& doc, QDomElement& element )
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	virtual void loadSettings( const QDomElement& element )
	{
		Q_UNUSED(element)
	}


} ;


#endif
