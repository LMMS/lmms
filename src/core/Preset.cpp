/*
 * Preset.cpp - A generic preset class for plugins
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

#include "Preset.h"

#include <algorithm>

namespace lmms
{

auto Preset::supportsPlugin(std::string_view key) const -> bool
{
	if (m_keys.empty()) { return true; }
	return std::find(m_keys.begin(), m_keys.end(), key) != m_keys.end();
}

auto operator<(const Preset& a, const Preset& b) noexcept -> bool
{
	// TODO: Better way to do this?
	auto res = a.m_loadData.location.compare(b.m_loadData.location);
	if (res < 0) { return true; }
	if (res > 0) { return false; }
	return a.m_loadData.loadKey < b.m_loadData.loadKey;
}

} // namespace lmms
