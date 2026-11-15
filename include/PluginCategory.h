/*
 * PluginCategory - Associates a plugin to a category (for grouping/filtering afterwards)
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

#ifndef LMMS_PLUGIN_CATEGORY_H
#define LMMS_PLUGIN_CATEGORY_H

#include <QDomDocument>
#include <QDomElement>
#include <QLabel>
#include <QList>

#include "lmms_export.h"
namespace lmms {

class PluginCategoryData
{
public:
	QString m_category;
	bool m_is_favorite;
};

class LMMS_EXPORT PluginCategory
{
public:
	static PluginCategory* instance();
	QString getCategoryName(QString pluginName);
	bool getIsFavorite(QString pluginName);
	QStringList getCategories();
	void setCategory(QString pluginName, QString categoryName);
	void toggleFavorite(QString pluginName, bool isFavorite);
	void save();

private:
	static std::unique_ptr<PluginCategory> s_instance;
	QStringList m_categories_list;
	std::map<QString, PluginCategoryData> m_categories_map;
	QStringList getCategoriesFromMap(std::map<QString, PluginCategoryData> map);
	void loadData();
	QDomElement createXMLTag(QDomDocument doc, QString tagName, QString value);
};

// Short-hand function
LMMS_EXPORT PluginCategory* getPluginCategory();

} // namespace lmms
#endif // LMMS_PLUGIN_CATEGORY_H
