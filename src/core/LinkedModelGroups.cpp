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
	AutomatableModel::linkModels(m_models[id2], other->m_models[id2]);
}




void LinkedModelGroup::unlinkControls(LinkedModelGroup *other, int id)
{
	Q_ASSERT(id >= 0);
	std::size_t id2 = static_cast<std::size_t>(id);
	AutomatableModel::unlinkModels(
		m_models[id2], other->m_models[id2]);
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




void LinkedModelGroup::addModel(AutomatableModel *model) {
	m_models.push_back(model); }




/*
	LinkedModelGroups
*/




LinkedModelGroups::~LinkedModelGroups() {}




void LinkedModelGroups::createMultiChannelLinkModel() {
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

	// if global channel link state has changed, always ignore link
	// status of individual ports in the future
	m_noLink = false;
}




