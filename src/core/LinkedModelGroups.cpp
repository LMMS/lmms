/*
 * LinkedModelGroups.cpp - base classes for groups of linkable models
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

#include "LinkedModelGroups.h"

#include <QDomDocument>
#include <QDomElement>

#include "AutomatableModel.h"
#include "stdshims.h"




/*
	LinkedModelGroup
*/




void LinkedModelGroup::makeLinkingProc()
{
	foreach_model([this](const std::string& str, ModelInfo inf)
	{
		BoolModel* bmo = new BoolModel(true, this, tr("Link channels"));
		inf.m_linkEnabled = bmo;
		connect(bmo, &BoolModel::dataChanged, this,
				[this, bmo, str]() { emit linkStateChanged(str, bmo->value()); });
	});
}




void LinkedModelGroup::linkAllModels(bool state)
{
	// this will call AutomatableModel::linkModels (??? TODO)
	for (auto pr : d.m_models) { pr.second.m_linkEnabled->setValue(state); }
}




void LinkedModelGroup::linkControls(LinkedModelGroup *other, const std::string& id)
{
	auto itr1 = models().find(id);
	auto itr2 = other->models().find(id);
	Q_ASSERT(itr1 != models().end() && itr2 != other->models().end());
	AutomatableModel::linkModels(itr1->second.m_model, itr2->second.m_model);
}




void LinkedModelGroup::unlinkControls(LinkedModelGroup *other, const std::string& id)
{
	auto itr1 = models().find(id);
	auto itr2 = other->models().find(id);
	Q_ASSERT(itr1 != models().end() && itr2 != other->models().end());
	AutomatableModel::unlinkModels(itr1->second.m_model, itr2->second.m_model);
}




bool LinkedModelGroup::isLinking() const {
	return d.m_models.size()
		?!!d.m_models.begin()->second.m_linkEnabled : false;
}




AutomatableModel *LinkedModelGroup::modelWithName(const QString &name) const
{
	auto itr = std::find_if(models().begin(), models().end(),
		[&name](const std::pair<std::string, ModelInfo>& pr) -> bool
	{ return pr.second.m_name == name; });
	return itr == models().end() ? nullptr : itr->second.m_model;
}




void LinkedModelGroup::saveValues(QDomDocument &doc, QDomElement &that,
	const LinkedModelGroup *lmg0)
{
	// if multiple lmgs, the first one must currently be the linking one
	Q_ASSERT(this == lmg0 || lmg0->isLinking());
	foreach_model([&lmg0, this, &doc, &that](const std::string& str, ModelInfo& inf)
	{
		if (this == lmg0 || !lmg0->linkEnabledModel(str)->value())
		{
			// try to load, if it fails, this will load a sane initial value
			inf.m_model->saveSettings(doc, that, /*models()[idx].m_name*/ inf.m_name); /* TODO: m_name useful */
		}
		else
		{
			// model has the same value as in the first LinkedModelGroup
		}
	});
}




void LinkedModelGroup::saveLinksEnabled(QDomDocument &doc, QDomElement &that)
{
	//for (std::size_t i = 0; i < d.m_linkEnabled.size(); ++i)
	foreach_model([&doc, &that](const std::string&, ModelInfo& inf)
	{
		inf.m_linkEnabled->saveSettings(doc, that, inf.m_name);
	});
}




void LinkedModelGroup::loadValues(const QDomElement &that,
	const LinkedModelGroup* lmg0)
{
	foreach_model([lmg0, this, &that](const std::string& str, ModelInfo& inf)
	{
		if (this == lmg0 || !lmg0->linkEnabledModel(str)->value())
		{
			// try to load, if it fails, this will load a sane initial value
			inf.m_model->loadSettings(that, /*models()[idx].m_name*/ inf.m_name); /* TODO: m_name useful */
		}
		else
		{
			// model has the same value as in the first LinkedModelGroup
		}
	});
}




void LinkedModelGroup::loadLinksEnabled(const QDomElement &that)
{
	foreach_model([&that](const std::string&, ModelInfo& inf)
	{
		inf.m_linkEnabled->loadSettings(that, inf.m_name);
	});
}




void LinkedModelGroup::addModel(AutomatableModel *model, const QString &name)
{
	model->setObjectName(name);
	models().emplace(std::string(name.toUtf8().data()), ModelInfo(name, model));
}




void LinkedModelGroup::clearModels()
{
	using datatype = decltype(d);
	d.~datatype();
	new (&d) datatype();
}




/*
	LinkedModelGroups
*/




LinkedModelGroups::~LinkedModelGroups() {}




void LinkedModelGroups::createMultiChannelLinkModel()
{
	m_multiChannelLinkModel =
		make_unique<BoolModel, BoolModelDeleter>(true, nullptr);
}




void LinkedModelGroups::linkModel(const std::string& model, bool state)
{
	LinkedModelGroup* first = getGroup(0);
	LinkedModelGroup* cur;

	if (state)
	{
		for (std::size_t i = 1; (cur = getGroup(i)); ++i)
		{
			first->linkControls(cur, model);
		}
	}
	else
	{
		for (std::size_t i = 1; (cur = getGroup(i)); ++i)
		{
			first->unlinkControls(cur, model);
		}

		// m_multiChannelLinkModel.setValue() will call
		// updateLinkStatesFromGlobal()...
		// m_noLink will make sure that this will not unlink any other models
		m_noLink = true;
		m_multiChannelLinkModel->setValue( false );
	}
}




void LinkedModelGroups::updateLinkStatesFromGlobal()
{
	LinkedModelGroup* first = getGroup(0);
	if (m_multiChannelLinkModel->value())
	{
		first->linkAllModels(true);
	}
	else if (!m_noLink)
	{
		first->linkAllModels(false);
	}

	m_noLink = false;
}




void LinkedModelGroups::saveSettings(QDomDocument& doc, QDomElement& that)
{
	LinkedModelGroup* grp0 = getGroup(0);
	if (grp0)
	{
		bool allLinked = false;
		if (m_multiChannelLinkModel)
		{
			m_multiChannelLinkModel->saveSettings(doc, that, "link");
			allLinked = m_multiChannelLinkModel->value();
		}

		if (!allLinked && getGroup(1))
		{
			QDomElement links = doc.createElement("links");
			grp0->saveLinksEnabled(doc, links);
			that.appendChild(links);
		}

		QDomElement models = doc.createElement("models");
		that.appendChild(models);

		char chanName[] = "chan0";
		LinkedModelGroup* lmg;
		for (std::size_t chanIdx = 0;
			// stop after last group
			// if all models are linked, store only the first group
			(lmg = getGroup(chanIdx)) && !(allLinked && chanIdx > 0);
			++chanIdx)
		{
			chanName[4] = '0' + static_cast<char>(chanIdx);
			QDomElement channel = doc.createElement(
									QString::fromUtf8(chanName));
			models.appendChild(channel);
			lmg->saveValues(doc, channel, grp0);
		}
	}
	else { /* don't even add a "models" node */ }
}




void LinkedModelGroups::loadSettings(const QDomElement& that)
{
	QDomElement models = that.firstChildElement("models");
	LinkedModelGroup* grp0;
	if (!models.isNull() && (grp0 = getGroup(0)))
	{
		bool allLinked = false;
		if (m_multiChannelLinkModel)
		{
			m_multiChannelLinkModel->loadSettings(that, "link");
			allLinked = m_multiChannelLinkModel->value();
		}

		if (!allLinked && getGroup(1))
		{
			QDomElement links = that.firstChildElement("links");
			if (!links.isNull()) { grp0->loadLinksEnabled(links); }
		}

		QDomElement lastChan;
		char chanName[] = "chan0";
		LinkedModelGroup* lmg;
		for (std::size_t chanIdx = 0;
			// stop after last group
			// if all models are linked, read only the first group
			(lmg = getGroup(chanIdx)) && !(allLinked && chanIdx > 0);
			++chanIdx)
		{
			chanName[4] = '0' + static_cast<char>(chanIdx);
			QDomElement chan = models.firstChildElement(chanName);
			if (!chan.isNull()) { lastChan = chan; }

			lmg->loadValues(lastChan, grp0);
		}
	}
}




void LinkedModelGroups::BoolModelDeleter::operator()(BoolModel *l)
{
	delete l;
}
