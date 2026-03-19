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

#include "VestigeSubPluginFeatures.h"

#include <QDir>
#include <QLabel>

#include "ConfigManager.h"
#include "VstList.h"
#include "Effect.h"

namespace lmms {

VestigeSubPluginFeatures::VestigeSubPluginFeatures(Plugin::Type pluginType)
	: SubPluginFeatures(pluginType)
{
}

void VestigeSubPluginFeatures::fillDescriptionWidget(QWidget* parent, const Key* key) const
{
	new QLabel(QWidget::tr("Name: ") + key->name, parent);
	new QLabel(QWidget::tr("File: ") + key->attributes["file"], parent);
}

void VestigeSubPluginFeatures::listSubPluginKeys(const Plugin::Descriptor* desc, KeyList& keylist) const
{
	// TODO: eval m_type
	for (const auto& data : VstList::inst()->instrumentPlugins())
	{
		EffectKey::AttributeMap am;
		am["file"] = QString::fromStdString(data.path.string());
		keylist.push_back(Key(desc, QString::fromStdString(data.name), am));
	}
}

void VestigeSubPluginFeatures::addPluginsFromDir(QStringList* filenames, QString path) const
{
	QStringList dirs
		= QDir{ConfigManager::inst()->vstDir() + path}.entryList(QStringList{"*"}, QDir::Dirs, QDir::Name);
	for (const QString& dir : dirs)
	{
		if (dir[0] != '.') { addPluginsFromDir(filenames, path + QDir::separator() + dir); }
	}
	QStringList vstFilenames = QDir(ConfigManager::inst()->vstDir() + path)
		.entryList(QStringList{} << "*.dll"
#ifdef LMMS_BUILD_LINUX
			<< "*.so"
#endif
			, QDir::Files, QDir::Name);
	for (const auto & vstFilename : vstFilenames)
	{
		QString vstPath = path + QDir::separator() + vstFilename;
		vstPath.remove(0, 1);
		filenames->append(vstPath);
	}
}

} // namespace lmms
