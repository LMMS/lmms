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
	for (std::size_t i = 0; i < m_models.size(); ++i)
	{
		BoolModel* bmo = new BoolModel(true, this, tr("Link channels"));
		m_linkEnabled.push_back(bmo);
		connect(bmo, &BoolModel::dataChanged, this,
				[this, bmo, i]() { emit linkStateChanged(i, bmo->value()); });
	}
}




void LinkedModelGroup::linkAllModels(bool state)
{
	for (BoolModel* bmo : m_linkEnabled) { bmo->setValue(state); }
}




void LinkedModelGroup::linkControls(LinkedModelGroup *other, std::size_t id)
{
	AutomatableModel::linkModels(
		m_models[id].m_model, other->m_models[id].m_model);
}




void LinkedModelGroup::unlinkControls(LinkedModelGroup *other, std::size_t id)
{
	AutomatableModel::unlinkModels(
		m_models[id].m_model, other->m_models[id].m_model);
}




void LinkedModelGroup::saveValues(QDomDocument &doc, QDomElement &that,
	const LinkedModelGroup *lmg0)
{
	// if multiple lmgs, the first one must currently be the linking one
	Q_ASSERT(this == lmg0 || lmg0->isLinking());
	for (std::size_t idx = 0; idx < m_models.size(); ++idx)
	{
		if (this == lmg0 || !lmg0->linkEnabledModel(idx)->value())
		{
			// try to load, if it fails, this will load a sane initial value
			m_models[idx].m_model->saveSettings(doc, that, m_models[idx].m_name);
		}
		else
		{
			// model has the same value as in the first LinkedModelGroup
		}
	}
}




void LinkedModelGroup::saveLinksEnabled(QDomDocument &doc, QDomElement &that)
{
	for (std::size_t i = 0; i < m_linkEnabled.size(); ++i)
	{
		m_linkEnabled[i]->saveSettings(doc, that, m_models[i].m_name);
	}
}




void LinkedModelGroup::loadValues(const QDomElement &that,
	const LinkedModelGroup* lmg0)
{
	for (std::size_t idx = 0; idx < m_models.size(); ++idx)
	{
		if (this == lmg0 || !lmg0->linkEnabledModel(idx)->value())
		{
			// try to load, if it fails, this will load a sane initial value
			m_models[idx].m_model->loadSettings(that, m_models[idx].m_name);
		}
		else
		{
			// model has the same value as in the first LinkedModelGroup
		}
	}
}




void LinkedModelGroup::loadLinksEnabled(const QDomElement &that)
{
	for (std::size_t i = 0; i < m_linkEnabled.size(); ++i)
	{
		m_linkEnabled[i]->loadSettings(that, m_models[i].m_name);
	}
}




void LinkedModelGroup::addModel(AutomatableModel *model, const QString &name)
{
	m_models.emplace_back(name, model);
}




/*
	LinkedModelGroups
*/




LinkedModelGroups::~LinkedModelGroups() {}




void LinkedModelGroups::createMultiChannelLinkModel()
{
	m_multiChannelLinkModel = make_unique<BoolModel>(true, nullptr);
}




void LinkedModelGroups::linkModel(std::size_t model, bool state)
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


