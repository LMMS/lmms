/*
 * LadspaBrowser.h - dialog to display information about installed LADSPA
 *					plugins
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _LADSPA_BROWSER_H
#define _LADSPA_BROWSER_H

#include "LadspaManager.h"
#include "ToolPlugin.h"
#include "ToolPluginView.h"

namespace lmms
{


namespace gui
{


class TabBar;

class LadspaBrowserView : public ToolPluginView
{
	Q_OBJECT
public:
	LadspaBrowserView( ToolPlugin * _tool );
	~LadspaBrowserView() override = default;


public slots:
	void showPorts( const ::lmms::ladspa_key_t & _key );


private:
	TabBar * m_tabBar;

	QWidget * createTab( QWidget * _parent, const QString & _txt,
						LadspaPluginType _type );

};


} // namespace gui


class LadspaBrowser : public ToolPlugin
{
public:
	LadspaBrowser();
	~LadspaBrowser() override = default;

	gui::PluginView* instantiateView( QWidget * ) override
	{
		return new gui::LadspaBrowserView( this );
	}

	QString nodeName() const override;

	void saveSettings( QDomDocument& doc, QDomElement& element ) override
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	void loadSettings( const QDomElement& element ) override
	{
		Q_UNUSED(element)
	}


} ;


} // namespace lmms

#endif
