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

#include "VstEffectSubPluginFeatures.h"

#include <QDir>
#include <QLabel>

#include "VstList.h"
#include "Effect.h"

namespace lmms {

VstEffectSubPluginFeatures::VstEffectSubPluginFeatures(Plugin::Type pluginType)
	: SubPluginFeatures(pluginType)
{
}

void VstEffectSubPluginFeatures::fillDescriptionWidget(QWidget* parent, const Key* key) const
{
	new QLabel(QWidget::tr("Name: ") + key->name, parent);
	new QLabel(QWidget::tr("File: ") + key->attributes["file"], parent);
}

void VstEffectSubPluginFeatures::listSubPluginKeys(const Plugin::Descriptor* desc, KeyList& keylist) const
{
	// TODO: eval m_type
	for (const auto& data : VstList::inst()->effectPlugins())
	{
		EffectKey::AttributeMap am;
		am["file"] = QString::fromStdString(data.path.string());
		keylist.push_back(Key(desc, QString::fromStdString(data.name), am));
	}
}

} // namespace lmms
