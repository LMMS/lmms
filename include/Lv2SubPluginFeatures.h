/*
 * Lv2SubPluginFeatures.h - derivation from
 *                          Plugin::Descriptor::SubPluginFeatures for
 *                          hosting LV2 plugins
 *
 * Copyright (c) 2018-2023 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_LV2_SUBPLUGIN_FEATURES_H
#define LMMS_LV2_SUBPLUGIN_FEATURES_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>

#include "lmms_export.h"
#include "Plugin.h"

namespace lmms
{


class LMMS_EXPORT Lv2SubPluginFeatures : public Plugin::Descriptor::SubPluginFeatures
{
private:
	static const LilvPlugin *getPlugin(const Key &k);
	static QString pluginName(const LilvPlugin *plug);

public:
	Lv2SubPluginFeatures(Plugin::Type type);

	void fillDescriptionWidget(
		QWidget *parent, const Key *k) const override;

	QString additionalFileExtensions(const Key &k) const override;
	QString displayName(const Key &k) const override;
	QString description(const Key &k) const override;
	const PixmapLoader *logo(const Key &k) const override;

	void listSubPluginKeys(
		const Plugin::Descriptor *desc, KeyList &kl) const override;
};


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2_SUBPLUGIN_FEATURES_H
