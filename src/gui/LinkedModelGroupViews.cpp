/*
 * LinkedModelGroupViews.h - view for groups of linkable models
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

#include "LinkedModelGroupViews.h"

#include <QPushButton>
#include "Controls.h"
#include "ControlLayout.h"
#include "LinkedModelGroups.h"

namespace lmms::gui
{


/*
	LinkedModelGroupViewBase
*/


LinkedModelGroupView::LinkedModelGroupView(QWidget* parent,
	LinkedModelGroup *model, std::size_t colNum) :
	QWidget(parent),
	m_model(model),
	m_colNum(colNum),
	m_layout(new ControlLayout(this))
{
	// This is required to remove the focus of the line edit
	// when e.g. another spin box is being clicked.
	// Removing the focus is wanted because in many cases, the user wants to
	// quickly play notes on the virtual keyboard.
	setFocusPolicy( Qt::StrongFocus );
}




void LinkedModelGroupView::modelChanged(LinkedModelGroup *group)
{
	// reconnect models
	group->foreach_model([this](const std::string& str,
		const LinkedModelGroup::ModelInfo& minf)
	{
		auto itr = m_widgets.find(str);
		// in case there are new or deleted widgets, the subclass has already
		// modified m_widgets, so this will go into the else case
		if (itr == m_widgets.end())
		{
			// no widget? this can happen when the whole view is being destroyed
			// (for some strange reasons)
		}
		else
		{
			itr->second->setModel(minf.m_model);
		}
	});

	m_model = group;
}




void LinkedModelGroupView::addControl(Control* ctrl, const std::string& id,
	const std::string &display, bool removable)
{
	if (ctrl)
	{
		auto box = new QWidget(this);
		auto boxLayout = new QHBoxLayout(box);
		boxLayout->addWidget(ctrl->topWidget());

		if (removable)
		{
			auto removeBtn = new QPushButton;
			removeBtn->setIcon( embed::getIconPixmap( "discard" ) );
			QObject::connect(removeBtn, &QPushButton::clicked,
				this, [this,ctrl](bool){
					AutomatableModel* controlModel = ctrl->model();
					// remove control out of model group
					// (will also remove it from the UI)
					m_model->removeControl(controlModel);
					// delete model (includes disconnecting all connections)
					delete controlModel;
				},
				Qt::DirectConnection);
			boxLayout->addWidget(removeBtn);
		}

		// required, so the Layout knows how to sort/filter widgets by string
		box->setObjectName(QString::fromStdString(display));
		m_layout->addWidget(box);

		// take ownership of control and add it
		m_widgets.emplace(id, std::unique_ptr<Control>(ctrl));
	}

	if (isHidden()) { setHidden(false); }
}




void LinkedModelGroupView::removeControl(const QString& key)
{
	auto itr = m_widgets.find(key.toStdString());
	if (itr != m_widgets.end())
	{
		QLayoutItem* item = m_layout->itemByString(key);
		Q_ASSERT(!!item);
		QWidget* wdg = item->widget();
		Q_ASSERT(!!wdg);

		// remove item from layout
		m_layout->removeItem(item);
		// the widget still exists and is visible - remove it now
		delete wdg;
		// erase widget pointer from dictionary
		m_widgets.erase(itr);
		// repaint immediately, so we don't have dangling model views
		m_layout->update();
	}
}




void LinkedModelGroupView::removeFocusFromSearchBar()
{
	m_layout->removeFocusFromSearchBar();
}


/*
	LinkedModelGroupsViewBase
*/


void LinkedModelGroupsView::modelChanged(LinkedModelGroups *groups)
{
	LinkedModelGroupView* groupView = getGroupView();
	LinkedModelGroup* group0 = groups->getGroup(0);
	if (group0 && groupView)
	{
		groupView->modelChanged(group0);
	}
}


} // namespace lmms::gui
