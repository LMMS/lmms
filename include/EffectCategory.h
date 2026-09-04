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

#ifndef LMMS_EFFECT_CATEGORY_H
#define LMMS_EFFECT_CATEGORY_H

#include <QLabel>
#include <QList>

#include "lmms_export.h"
namespace lmms 
{

class LMMS_EXPORT EffectCategory
{
public:
	static EffectCategory* instance();
	QString getCategoryName(QString effectName);
	QStringList getCategories();
private:
	static std::unique_ptr<EffectCategory> s_instance;
	QStringList m_categories;
	QStringList getCategoriesFromMap(std::map<QString, QString> map);
};

// Short-hand function
LMMS_EXPORT EffectCategory* getEffectCategory();

} // namespace lmms
#endif // LMMS_EFFECT_CATEGORY_H
