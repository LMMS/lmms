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
#include <QVariant>
#include <QVBoxLayout>

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
	m_filter{new ControlFilterWidget{this}}
{
	// This is required to remove the focus of the line edit
	// when e.g. another spin box is being clicked.
	// Removing the focus is wanted because in many cases, the user wants to
	// quickly play notes on the virtual keyboard.
	setFocusPolicy(Qt::StrongFocus);

	const auto controlContainer = new QWidget{this};
	m_layout = new ControlLayout{controlContainer};

	const auto thisLayout = new QVBoxLayout{this};
	thisLayout->addWidget(m_filter);
	thisLayout->addWidget(controlContainer);

	connect(m_filter, &ControlFilterWidget::filterChanged, m_layout, &ControlLayout::setFilterString);
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




void LinkedModelGroupView::addControl(std::unique_ptr<Control> ctrl, const std::string& id,
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
				this, [this, ctrl = ctrl.get()](bool){
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
		box->setProperty(ControlLayout::KeyProperty, QString::fromStdString(display));
		m_layout->addWidget(box);

		// add control to map
		m_widgets.emplace(id, std::move(ctrl));
	}

	if (isHidden()) { setHidden(false); }
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
