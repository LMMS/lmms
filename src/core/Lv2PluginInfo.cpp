/*
 * Lv2PluginInfo - definition of Lv2PluginInfoclass
 *
 * Copyright (c) 2018 Alexandros Theodotou @faiyadesu
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

#include <lilv/lilvmm.hpp>
#include <QDebug>
#include <QRegExp>
#include <QStringList>

#include "Lv2PluginInfo.h"

Lv2PluginInfo::Lv2PluginInfo(Lilv::Plugin *plugin ) :
	uri(  plugin->get_uri().as_string() ),
	name( plugin->get_name().as_string() ),
	childClass(  plugin->get_class().get_label().as_string() ),
	numPorts( plugin->get_num_ports() ),
	authorName( plugin->get_author_name().as_string() ),
	authorEmail( plugin->get_author_email().as_string() ),
	authorHomePage( plugin->get_author_homepage().as_string() ),
	m_raw_plugin( plugin)
{
	QRegExp rx(".+#([a-zA-Z]+)");
	rx.indexIn(plugin->get_class().get_parent_uri().as_string());
	QStringList list = rx.capturedTexts();
	parentClass = list.at(1);
}

Lv2PluginInfo::~Lv2PluginInfo()
{

}

void Lv2PluginInfo::debugPrint() const
{
	qDebug() << uri << name << parentClass << childClass << numPorts << authorName << authorEmail << authorHomePage;
}
