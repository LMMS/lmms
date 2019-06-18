/*
 * LinkedModelGroupViews.h - views for groups of linkable models
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

#include <QGridLayout>

#include "Controls.h"
#include "LedCheckbox.h"
#include "LinkedModelGroups.h"
#include "stdshims.h"


/*
	LinkedModelGroupViewBase
*/


LinkedModelGroupView::LinkedModelGroupView(QWidget* parent,
	LinkedModelGroup *model, std::size_t colNum, std::size_t nProc, const QString& name) :
	QGroupBox(parent),
	m_colNum(colNum),
	m_isLinking(model->isLinking()),
	m_grid(new QGridLayout(this))
{
	if (model->models().size())
	{
		std::size_t curProc = model->curProc();
		QString chanName;
		if (name.isNull())
		{
			switch (nProc)
			{
				case 1: break; // don't display any channel name
				case 2:
					chanName = curProc
								? QObject::tr("Right")
								: QObject::tr("Left");
					break;
				default:
					chanName = QObject::tr("Channel %1").arg(curProc + 1);
					break;
			}
		}
		else { chanName = name; }

		if (!chanName.isNull()) { setTitle(chanName); }
	}
	else { setHidden(true); }
}




LinkedModelGroupView::~LinkedModelGroupView() {}




void LinkedModelGroupView::modelChanged(LinkedModelGroup *group)
{
	// reconnect models
	using ModelInfo = LinkedModelGroup::ModelInfo;
	std::vector<ModelInfo> models = group->models();
	Q_ASSERT(m_controls.size() == models.size());

	for (std::size_t i = 0; i < models.size(); ++i)
	{
		m_controls[i]->setModel(models[i].m_model);
	}

	for (std::size_t i = 0; i < m_leds.size(); ++i)
	{
		m_leds[i]->setModel(group->linkEnabledModel(i));
	}
}




void LinkedModelGroupView::addControl(Control* ctrl)
{
	int colNum2 = static_cast<int>(m_colNum * (1 + m_isLinking));
	int wdgNum = static_cast<int>(m_controls.size() * (1 + m_isLinking));
	if (ctrl)
	{
		int x = wdgNum%colNum2, y = wdgNum/colNum2;

		// start in row one, add widgets cell by cell
		if (m_isLinking)
		{
			LedCheckBox* cb = new LedCheckBox(
				ctrl->topWidget()->parentWidget());
			m_grid->addWidget(cb, y, x);
			m_leds.push_back(std::unique_ptr<LedCheckBox>(cb));
		}

		m_controls.push_back(std::unique_ptr<Control>(ctrl));
		m_grid->addWidget(ctrl->topWidget(), y, x + 1, Qt::AlignCenter);
		wdgNum += m_isLinking;
		++wdgNum;
	}

	// matter of taste (takes a lot of space):
	// makeAllGridCellsEqualSized();
}




void LinkedModelGroupView::makeAllGridCellsEqualSized()
{
	int rowHeight = 0, colWidth = 0;
	for (int row = 0; row < m_grid->rowCount(); ++row)
	{
		for (int col = 0; col < m_grid->columnCount(); ++col)
		{
			QLayoutItem* layout;
			if ((layout = m_grid->itemAtPosition(row, col)))
			{
				rowHeight = qMax(rowHeight, layout->geometry().height());
				colWidth = qMax(colWidth, layout->geometry().width());
			}
		}
	}

	for (int row = 0; row < m_grid->rowCount(); ++row)
	{
		m_grid->setRowMinimumHeight(row, rowHeight);
	}

	for (int col = 0; col < m_grid->columnCount(); ++col)
	{
		m_grid->setColumnMinimumWidth(col, colWidth);
	}
}


/*
	LinkedModelGroupsViewBase
*/


LinkedModelGroupsView::LinkedModelGroupsView(
	LinkedModelGroups *ctrlBase)
{
	if (ctrlBase->multiChannelLinkModel())
	{
		m_multiChannelLink = make_unique<LedCheckBox, MultiChannelLinkDeleter>
								(QObject::tr("Link Channels"), nullptr);
	}
}




void LinkedModelGroupsView::modelChanged(LinkedModelGroups *groups)
{
	if (groups->multiChannelLinkModel())
	{
		m_multiChannelLink->setModel(groups->multiChannelLinkModel());
	}

	LinkedModelGroupView* groupView;
	LinkedModelGroup* group;
	for (std::size_t i = 0;
		(group = groups->getGroup(i)) && (groupView = getGroupView(i));
		++i)
	{
		groupView->modelChanged(group);
	}
}




// If you wonder why the default deleter can not be used:
// https://stackoverflow.com/questions/9954518
void LinkedModelGroupsView::MultiChannelLinkDeleter::
	operator()(LedCheckBox *l) { delete l; }


