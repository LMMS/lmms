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
#include "LinkedModelGroupLayout.h"
#include "LinkedModelGroups.h"
#include "stdshims.h"


/*
	LinkedModelGroupViewBase
*/


LinkedModelGroupView::LinkedModelGroupView(QWidget* parent,
	LinkedModelGroup *model, std::size_t colNum, std::size_t nProc, const QString& name) :
	QGroupBox(parent),
	m_model(model),
	m_colNum(colNum),
	m_layout(new LinkedModelGroupLayout(this))
{
	// make viewable: if there are no knobs, the user should at least see
	// a rectangle to drop controls in
	setMinimumSize(QSize(50, 50));

	if (model->modelNum())
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
	else { /*setHidden(true);*/ }
}




LinkedModelGroupView::~LinkedModelGroupView() {}




void LinkedModelGroupView::modelChanged(LinkedModelGroup *group)
{
	// reconnect models
	group->foreach_model([this](const std::string& str,
		const LinkedModelGroup::ModelInfo& minf)
	{
		auto itr = m_widgets.find(str);
		// in case there are new or deleted widgets, the subclass has already
		// modified m_widgets, so this will go into the else case
		if(itr == m_widgets.end())
		{
			// no widget? this can happen when the whole view is being destroyed
			// (for some strange reasons)
		}
		else
		{
			itr->second.m_ctrl->setModel(minf.m_model);
		}
	});

	m_model = group;
}




void LinkedModelGroupView::addControl(Control* ctrl, const std::string& id,
	const std::string &display, bool removable)
{
	int wdgNum = static_cast<int>(m_widgets.size() * (1 + m_isLinking));
	if (ctrl)
	{
		QWidget* box = new QWidget(this);
		QHBoxLayout* boxLayout = new QHBoxLayout(box);

		// book-keeper of widget pointers
		WidgetsPerModel widgets;

		if (m_isLinking)
		{
			widgets.m_led = new LedCheckBox(ctrl->topWidget()->parentWidget());
			boxLayout->addWidget(widgets.m_led);
		}

		widgets.m_ctrl.reset(ctrl);
		boxLayout->addWidget(ctrl->topWidget());

		if (removable)
		{
			QPushButton* removeBtn = new QPushButton;
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

		m_widgets.emplace(id, std::move(widgets)); // TODO: use set?
		wdgNum += m_isLinking;
		++wdgNum;
	}

	if(isHidden()) { setHidden(false); }

	// matter of taste (takes a lot of space):
	// makeAllGridCellsEqualSized();
}




void LinkedModelGroupView::removeControl(const QString& key)
{
	auto itr = m_widgets.find(key.toStdString());
	if(itr != m_widgets.end())
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




void LinkedModelGroupView::makeAllGridCellsEqualSized()
{
#if 0
	int rowHeight = 0, colWidth = 0;
	for (int row = 0; row < m_layout->rowCount(); ++row)
	{
		for (int col = 0; col < m_layout->columnCount(); ++col)
		{
			QLayoutItem* layout;
			if ((layout = m_layout->itemAtPosition(row, col)))
			{
				rowHeight = qMax(rowHeight, layout->geometry().height());
				colWidth = qMax(colWidth, layout->geometry().width());
			}
		}
	}

	for (int row = 0; row < m_layout->rowCount(); ++row)
	{
		m_layout->setRowMinimumHeight(row, rowHeight);
	}

	for (int col = 0; col < m_layout->columnCount(); ++col)
	{
		m_layout->setColumnMinimumWidth(col, colWidth);
	}
#endif
}


/*
	LinkedModelGroupsViewBase
*/


void LinkedModelGroupsView::modelChanged(LinkedModelGroups *groups)
{
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


