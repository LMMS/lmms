/*
 * ModelGroup.cpp - base classes for groups of models
 *
 * Copyright (c) 2019-2024 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#include "ModelGroup.h"


#include "AutomatableModel.h"



namespace lmms
{

/*
	ModelGroup
*/


void ModelGroup::saveSettings(QDomDocument &doc, QDomElement &that)
{
	QDomElement models = doc.createElement("models");
	that.appendChild(models);
	foreach_model([&doc, &models](const std::string& , ModelInfo& inf)
	{
		inf.m_model->saveSettings(doc, models, inf.m_name); /* TODO: m_name useful */
	});
}




void ModelGroup::loadSettings(const QDomElement &that)
{
	QDomElement models = that.firstChildElement("models");
	foreach_model([&models](const std::string& , ModelInfo& inf)
	{
		// try to load, if it fails, this will load a sane initial value
		inf.m_model->loadSettings(models, inf.m_name); /* TODO: m_name useful */
	});
}





void ModelGroup::addModel(AutomatableModel *model, const QString &name)
{
	model->setObjectName(name);
	m_models.emplace(std::string(name.toUtf8().data()), ModelInfo(name, model));
	/*
	 * The following code is currently not used because Models can not be removed
	 * in no kind of implementation that uses ModelGroup. The code is
	 * deactivated because this is not a QObject anymore (an ugly connection
	 * handler would be required for this)
	 */
# if 0
	QObject::connect(model, &AutomatableModel::destroyed,
				this, [this, model](jo_id_t){
					if (const auto it = m_models.find(model->objectName().toStdString()); it != m_models.end())
					{
						m_models.erase(it);
					}
				},
				Qt::DirectConnection);
#endif
	// View needs to create another child view, e.g. a new knob:
	emit m_parent->dataChanged();
}




void ModelGroup::removeModel(AutomatableModel* mdl)
{
	if (const auto it = m_models.find(mdl->objectName().toStdString()); it != m_models.end())
	{
		m_models.erase(it);
	}
}


} // namespace lmms
