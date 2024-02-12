/*
 * ClapPreset.cpp - CLAP presets
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#include "ClapPreset.h"

#ifdef LMMS_HAVE_CLAP

#include "ClapLog.h"
#include "ClapManager.h"
#include "Engine.h"

namespace lmms
{

auto ClapPresets::fromKey(const Plugin::Descriptor::SubPluginFeatures::Key* key) const
	-> std::vector<const Preset*>
{
	std::vector<const Preset*> ret;
	if (!key) { return ret; }

	for (auto& mapPair : presets())
	{
		for (auto& preset : mapPair.second)
		{
			if (preset.key == key)
			{
				ret.push_back(&preset);
			}
		}
	}

	return ret;
}

auto ClapPresets::queryPreset(const PresetLoadData& loadData,
	const Plugin::Descriptor::SubPluginFeatures::Key* key) const -> Preset
{
	// TODO: [NOT IMPLEMENTED YET]
	return PluginPresets::queryPreset(loadData, key);
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
