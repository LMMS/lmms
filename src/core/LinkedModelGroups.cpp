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




/*
	LinkedModelGroup
*/




void LinkedModelGroup::makeLinkingProc()
{
	for (std::size_t i = 0; i < m_models.size(); ++i)
	{
		BoolModel* bmo = new BoolModel(true, this, tr("Link channels"));
		m_linkEnabled.push_back(bmo);
		connect(bmo, SIGNAL(dataChanged()), this, SLOT(linkStateChangedSlot()));
	}
}




void LinkedModelGroup::linkAllModels(bool state)
{
	for (BoolModel* bmo : m_linkEnabled) { bmo->setValue(state); }
}




void LinkedModelGroup::linkControls(LinkedModelGroup *other, int id)
{
	Q_ASSERT(id >= 0);
	std::size_t id2 = static_cast<std::size_t>(id);
	AutomatableModel::linkModels(
		m_models[id2].m_model, other->m_models[id2].m_model);
}




void LinkedModelGroup::unlinkControls(LinkedModelGroup *other, int id)
{
	Q_ASSERT(id >= 0);
	std::size_t id2 = static_cast<std::size_t>(id);
	AutomatableModel::unlinkModels(
		m_models[id2].m_model, other->m_models[id2].m_model);
}




void LinkedModelGroup::saveValues(QDomDocument &doc, QDomElement &that,
	const LinkedModelGroup *lmg0)
{
	Q_ASSERT(lmg0->isLinking());
	for (const ModelInfo& minf : m_models)
	{
		std::size_t idx = 0;
		if (this == lmg0) { idx = lmg0->models().size(); } // force saving
		else
		{
			for (; idx < lmg0->models().size(); ++idx)
			{
				if (lmg0->models()[idx].m_name == minf.m_name)
				{
					break;
				}
			}
		}
		if (idx < lmg0->models().size() &&
			lmg0->linkEnabledModel(idx)->value())
		{
			// link is enabled => nothing to save
		}
		else
		{
			minf.m_model->saveSettings(doc, that, minf.m_name);
		}
	}
}




void LinkedModelGroup::saveLinksEnabled(QDomDocument &doc, QDomElement &that)
{
	if (m_linkEnabled.size())
	{
		std::size_t count = 0;
		for (BoolModel* bmo : m_linkEnabled)
		{
			bmo->saveSettings(doc, that, m_models[count++].m_name);
		}
	}
}




void LinkedModelGroup::loadValues(const QDomElement &that,
	const LinkedModelGroup* lmg0)
{
	for (ModelInfo& minf : m_models)
	{
		std::size_t idx = 0;
		if (this == lmg0) { idx = lmg0->models().size(); } // force loading
		else
		{
			for (; idx < lmg0->models().size(); ++idx)
			{
				if (lmg0->models()[idx].m_name == minf.m_name)
				{
					break;
				}
			}
		}
		if (idx < lmg0->models().size() &&
			lmg0->linkEnabledModel(idx)->value())
		{
			// link is enabled => it will load automatically
		}
		else
		{
			// try to load, if it fails, this will load a sane initial value
			minf.m_model->loadSettings(that, minf.m_name);
		}
	}
}




void LinkedModelGroup::loadLinksEnabled(const QDomElement &that)
{
	if (m_linkEnabled.size())
	{
		std::size_t count = 0;
		for (BoolModel* bmo : m_linkEnabled)
		{
			bmo->loadSettings(that, m_models[count++].m_name);
		}
	}
}




void LinkedModelGroup::linkStateChangedSlot()
{
	QObject* sender = QObject::sender();
	Q_ASSERT(sender);
	BoolModel* bmo = qobject_cast<BoolModel*>(sender);
	Q_ASSERT(bmo);
	int modelNo = -1, count = 0;
	for (BoolModel* bmo2 : m_linkEnabled)
	{
		if (bmo2 == bmo) { modelNo = count; }
		++count;
	}
	Q_ASSERT(modelNo >= 0);
	emit linkStateChanged(modelNo, bmo->value());
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
	m_multiChannelLinkModel.reset(new BoolModel(true, nullptr));
}




void LinkedModelGroups::linkPort(int port, bool state)
{
	LinkedModelGroup* first = getGroup(0);
	LinkedModelGroup* cur;

	if (state)
	{
		for (std::size_t i = 1; (cur=getGroup(i)); ++i)
		{
			first->linkControls(cur, port);
		}
	}
	else
	{
		for (std::size_t i = 1; (cur=getGroup(i)); ++i)
		{
			first->unlinkControls(cur, port);
		}

		// m_multiChannelLinkModel.setValue() will call
		// updateLinkStatesFromGlobal()...
		// m_noLink will make sure that this will not unlink any other ports
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
	if (getGroup(0))
	{
		bool allLinked = false;
		if (m_multiChannelLinkModel)
		{
			m_multiChannelLinkModel->saveSettings(doc, that, "link");
			allLinked = m_multiChannelLinkModel->value();
		}

		if(!allLinked && getGroup(1))
		{
			QDomElement links = doc.createElement("links");
			getGroup(0)->saveLinksEnabled(doc, links);
			that.appendChild(links);
		}

		QDomElement models = doc.createElement("models");
		that.appendChild(models);

		char chanName[] = "chan0";
		for (char* chanPtr = chanName + 4; *chanPtr >= '0'; ++*chanPtr)
		{
			LinkedModelGroup* lmg = getGroup(static_cast<std::size_t>(
												*chanPtr - '0'));
			if (lmg)
			{
				QDomElement channel = doc.createElement(
										QString::fromUtf8(chanName));
				models.appendChild(channel);
				lmg->saveValues(doc, channel, getGroup(0));
			}
			else { *chanPtr = 0; } // end reached

			// if all models are linked, stop after first group
			if (allLinked) { *chanPtr = 0; }
		}
	}
	else { /* don't even add a "models" node */ }
}




void LinkedModelGroups::loadSettings(const QDomElement& that)
{
	QDomElement models = that.firstChildElement("models");
	if (!models.isNull() && getGroup(0))
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
			if(!links.isNull()) { getGroup(0)->loadLinksEnabled(links); }
		}

		QDomElement lastChan;
		char chanName[] = "chan0";
		for (char* chanPtr = chanName + 4; *chanPtr >= '0'; ++*chanPtr)
		{
			LinkedModelGroup* lmg = getGroup(static_cast<std::size_t>(
												*chanPtr - '0'));
			if (lmg)
			{
				QDomElement chan = models.firstChildElement(chanName);
				if (!chan.isNull()) { lastChan = chan; }
				lmg->loadValues(lastChan, getGroup(0));
			}
			else { *chanPtr = 0; } // end reached

			// if all models are linked, stop after first group
			if (allLinked) { *chanPtr = 0; }
		}
	}
}


