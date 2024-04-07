/*
 * LinkedModelGroups.cpp - base classes for groups of linked models
 *
 * Copyright (c) 2019-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include <QDomDocument>
#include <QDomElement>

#include "LinkedModelGroups.h"


#include "AutomatableModel.h"



namespace lmms
{

/*
	LinkedModelGroup
*/


void LinkedModelGroup::linkControls(LinkedModelGroup *other)
{
	foreach_model([&other](const std::string& id, ModelInfo& inf)
	{
		auto itr2 = other->m_models.find(id);
		Q_ASSERT(itr2 != other->m_models.end());
		AutomatableModel::linkModels(inf.m_model, itr2->second.m_model);
	});
}




void LinkedModelGroup::saveValues(QDomDocument &doc, QDomElement &that)
{
	foreach_model([&doc, &that](const std::string& , ModelInfo& inf)
	{
		inf.m_model->saveSettings(doc, that, /*m_models[idx].m_name*/ inf.m_name); /* TODO: m_name useful */
	});
}




void LinkedModelGroup::loadValues(const QDomElement &that)
{
	foreach_model([&that](const std::string& , ModelInfo& inf)
	{
		// try to load, if it fails, this will load a sane initial value
		inf.m_model->loadSettings(that, /*m_models()[idx].m_name*/ inf.m_name); /* TODO: m_name useful */
	});
}




void LinkedModelGroup::addModel(AutomatableModel *model, const QString &name)
{
	model->setObjectName(name);
	m_models.emplace(std::string(name.toUtf8().data()), ModelInfo(name, model));
	connect(model, &AutomatableModel::destroyed,
				this, [this, model](jo_id_t){
					if(containsModel(model->objectName()))
					{
						emit modelRemoved(model);
						eraseModel(model->objectName());
					}
				},
				Qt::DirectConnection);

	// View needs to create another child view, e.g. a new knob:
	emit modelAdded(model);
	emit dataChanged();
}




void LinkedModelGroup::removeControl(AutomatableModel* mdl)
{
	if(containsModel(mdl->objectName()))
	{
		emit modelRemoved(mdl);
		eraseModel(mdl->objectName());
	}
}




bool LinkedModelGroup::eraseModel(const QString& name)
{
	return m_models.erase(name.toStdString()) > 0;
}




void LinkedModelGroup::clearModels()
{
	m_models.clear();
}




bool LinkedModelGroup::containsModel(const QString &name) const
{
	return m_models.find(name.toStdString()) != m_models.end();
}




/*
	LinkedModelGroups
*/



void LinkedModelGroups::linkAllModels()
{
	LinkedModelGroup* first = getGroup(0);
	for (size_t i = 1; auto cur = getGroup(i); ++i)
	{
		first->linkControls(cur);
	}
}




void LinkedModelGroups::saveSettings(QDomDocument& doc, QDomElement& that)
{
	LinkedModelGroup* grp0 = getGroup(0);
	if (grp0)
	{
		QDomElement models = doc.createElement("models");
		that.appendChild(models);
		grp0->saveValues(doc, models);
	}
	else { /* don't even add a "models" node */ }
}




void LinkedModelGroups::loadSettings(const QDomElement& that)
{
	QDomElement models = that.firstChildElement("models");
	if (auto grp0 = getGroup(0); !models.isNull() && grp0)
	{
		// only load the first group, the others are linked to the first
		grp0->loadValues(models);
	}
}


} // namespace lmms
