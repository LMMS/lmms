/*
 * EffectCategory - Associates an effect to a category (for grouping/filtering afterwards)
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

#include "EffectCategory.h"

namespace lmms {

std::unique_ptr<EffectCategory> EffectCategory::s_instance;

QString defaultCategory = "Other";

EffectCategory* EffectCategory::instance()
{
	if (s_instance == nullptr) { s_instance = std::make_unique<EffectCategory>(); }

	return s_instance.get();
}

EffectCategory* getEffectCategory()
{ return EffectCategory::instance(); }

QString EffectCategory::getCategoryName(QString effectName)
{ return defaultCategory; }

} // namespace lmms
