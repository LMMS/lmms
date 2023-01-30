/*
 * ClapSubPluginFeatures.h - derivation from
 *                           Plugin::Descriptor::SubPluginFeatures for
 *                           hosting CLAP plugins
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_CLAP_SUBPLUGIN_FEATURES_H
#define LMMS_CLAP_SUBPLUGIN_FEATURES_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapFile.h"
#include "Plugin.h"

#include "lmms_export.h"

namespace lmms
{


class LMMS_EXPORT ClapSubPluginFeatures : public Plugin::Descriptor::SubPluginFeatures
{
public:
	ClapSubPluginFeatures(Plugin::PluginTypes type);

	void fillDescriptionWidget(QWidget* parent, const Key* key) const override;
	void listSubPluginKeys(const Plugin::Descriptor* desc, KeyList& kl) const override;
	auto additionalFileExtensions(const Key& key) const -> QString override;
	auto displayName(const Key& key) const -> QString override;
	auto description(const Key& key) const -> QString override;
	auto logo(const Key& key) const -> const PixmapLoader* override;

private:
	static auto getPluginInfo(const Key& key) -> const ClapPluginInfo*;
};


} // namespace lmms

#endif // LMMS_HAVE_CLAP

#endif // LMMS_CLAP_SUBPLUGIN_FEATURES_H
