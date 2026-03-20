/*
 * VstSubPluginFeatures.cpp - derivation from
 *                            Plugin::Descriptor::SubPluginFeatures for
 *                            hosting VST-plugins
 *
 * Copyright (c) 2006-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "VstSubPluginFeatures.h"

#include <QDir>
#include <QLabel>

#include "VstList.h"
#include "Plugin.h"

using AttributeMap = lmms::Plugin::Descriptor::SubPluginFeatures::Key::AttributeMap;

namespace
{

inline std::filesystem::path getPath(const lmms::Plugin::Descriptor::SubPluginFeatures::Key& key)
{
	return key.attributes["file"].toStdString();
}

} // namespace


namespace lmms
{

VstSubPluginFeatures::VstSubPluginFeatures(Plugin::Type pluginType)
	: SubPluginFeatures{pluginType}
	, m_pluginType{pluginType}
{
}

void VstSubPluginFeatures::fillDescriptionWidget(QWidget* parent, const Key* key) const
{
	new QLabel(QWidget::tr("Name: ") + key->name, parent);
	new QLabel(QWidget::tr("File: ") + key->attributes["file"], parent);
}

void VstSubPluginFeatures::listSubPluginKeys(const Plugin::Descriptor* desc, KeyList& keylist) const
{
	// TODO: eval m_type
	for (const auto& data : m_pluginType == Plugin::Type::Instrument
		? VstList::inst()->instrumentPlugins()
		: VstList::inst()->effectPlugins())
	{
		AttributeMap am;
		am["file"] = QString::fromStdString(data.path.string());
		keylist.push_back(Key(desc, QString::fromStdString(data.name), am));
	}
}

QString VstSubPluginFeatures::displayName(const Key& key) const
{
	return QString::fromStdString(VstList::inst()->plugins()[getPath(key)].name);
}

QString VstSubPluginFeatures::description(const Key& key) const
{
	return QString::fromStdString(VstList::inst()->plugins()[getPath(key)].product);
}

} // namespace lmms
