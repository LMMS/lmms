/*
 * ControllerRackView.cpp - view for song's controllers
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2010-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ControllerRackView.h"

#include <QApplication>
#include <QLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

#include "ControllerView.h"
#include "embed.h"
#include "GroupBox.h"
#include "GuiApplication.h"
#include "LfoController.h"
#include "MainWindow.h"
#include "Song.h"


ControllerRackView::ControllerRackView() :
	QWidget(),
	m_allExpanded(false),
	m_allCollapsed(false)
{
	setWindowIcon(embed::getIconPixmap("controller"));
	setWindowTitle(tr("Controller Rack"));

	m_scrollArea = new QScrollArea(this);
	m_scrollArea->setPalette(QApplication::palette(m_scrollArea));
	m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scrollArea->setFrameStyle(QFrame::Plain);
	m_scrollArea->setFrameShadow(QFrame::Plain);

	QWidget * scrollAreaWidget = new QWidget(m_scrollArea);
	m_scrollAreaLayout = new QVBoxLayout(scrollAreaWidget);
	m_scrollAreaLayout->addStretch();
	m_scrollAreaLayout->setMargin(0);
	m_scrollAreaLayout->setSpacing(0);
	scrollAreaWidget->setLayout(m_scrollAreaLayout);

	m_scrollArea->setWidget(scrollAreaWidget);
	m_scrollArea->setWidgetResizable(true);

	m_addButton = new QPushButton(this);
	m_addButton->setText(tr("Add LFO"));
	connect(m_addButton, SIGNAL(clicked()), this, SLOT(addLfoController()));

	connect(Engine::getSong(), SIGNAL(controllerAdded(Controller*)), SLOT(onControllerAdded(Controller*)));
	connect(Engine::getSong(), SIGNAL(controllerRemoved(Controller*)), SLOT(onControllerRemoved(Controller*)));

	QVBoxLayout * layout = new QVBoxLayout();
	layout->addWidget(m_scrollArea);
	layout->addWidget(m_addButton);
	layout->setMargin(0);
	setLayout(layout);

	m_subWin = gui->mainWindow()->addWindowedWidget(this);

	Qt::WindowFlags flags = m_subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	m_subWin->setWindowFlags(flags);

	m_subWin->setAttribute(Qt::WA_DeleteOnClose, false);
	m_subWin->move(680, 60);
	m_subWin->resize(400, 249);
	m_subWin->setFixedWidth(249);
}




ControllerRackView::~ControllerRackView()
{
}




void ControllerRackView::saveSettings(QDomDocument &,
							QDomElement & parent)
{
	MainWindow::saveWidgetState(this, parent);
}




void ControllerRackView::loadSettings(const QDomElement & _this)
{
	MainWindow::restoreWidgetState(this, _this);
}




void ControllerRackView::deleteController(ControllerView * view)
{
	Controller * c = view->getController();

	if(c->connectionCount() > 0)
	{
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Question);
		msgBox.setWindowTitle(tr("Confirm Delete"));
		msgBox.setText(tr("Confirm delete? There are existing connection(s) "
				"associated with this controller. There is no way to undo."));
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		if(msgBox.exec() != QMessageBox::Ok)
		{
			return;
		}
	}

	Engine::getSong()->removeController(c);
}




void ControllerRackView::collapsingAll()
{
	for (auto &it : m_controllerViews)
	{
		it->collapseController();
	}
	m_allCollapsed = true;
	m_allExpanded = false;
}




void ControllerRackView::expandAll()
{
	for (auto &it : m_controllerViews)
	{
		it->expandController();
	}
	m_allCollapsed = false;
	m_allExpanded = true;
}




void ControllerRackView::onControllerAdded(Controller * controller)
{
	ControllerView * controllerView = new ControllerView(controller, m_scrollArea->widget());
	connect(controllerView, SIGNAL(deleteController(ControllerView*)), this, SLOT(deleteController(ControllerView*)), Qt::QueuedConnection);
	connect(controllerView, SIGNAL(controllerCollapsed()), this, SLOT(onControllerCollapsed()));
	connect(controllerView, SIGNAL(collapseAll()), this, SLOT(collapsingAll()));
	connect(controllerView, SIGNAL(expandAll()), this, SLOT(expandAll()));
	connect(controllerView, SIGNAL(controllerMoveUp(ControllerView*)), this, SLOT(moveControllerUp(ControllerView*)));
	connect(controllerView, SIGNAL(controllerMoveDown(ControllerView*)), this, SLOT(moveControllerDown(ControllerView*)));
	m_controllerViews.append(controllerView);
	int n = m_scrollAreaLayout->count() - 1; //-1 because the stretch?
	m_scrollAreaLayout->insertWidget(n, controllerView);

	update();
}




void ControllerRackView::onControllerRemoved(Controller * removedController)
{
	for (auto &it : m_controllerViews)
	{
		if (it->getController() == removedController)
		{
			ControllerView * viewOfRemovedController = it;
			m_controllerViews.erase( std::find( m_controllerViews.begin(),
						m_controllerViews.end(), viewOfRemovedController ) );

			delete viewOfRemovedController;
			m_scrollArea->verticalScrollBar()->hide();
			update();
			break;
		}
	}
}




void ControllerRackView::onControllerCollapsed()
{
	m_scrollArea->verticalScrollBar()->hide();
	update();
}




void ControllerRackView::addLfoController()
{
	Engine::getSong()->addController(new LfoController(Engine::getSong()));
	setFocus();
}




void ControllerRackView::moveControllerUp(ControllerView *cv)
{
	if (cv != m_controllerViews.first())
	{
		int index = m_controllerViews.indexOf(cv);
		m_scrollAreaLayout->removeWidget(cv);
		m_scrollAreaLayout->insertWidget(index - 1, cv);
		m_controllerViews.move(index, index - 1);
	}
}




void ControllerRackView::moveControllerDown(ControllerView *cv)
{
	//move up the controller among is the same
	if (cv != m_controllerViews.last())
	{
		int index = m_controllerViews.indexOf(cv) + 1;
		moveControllerUp(m_controllerViews.at(index));
	}
}

const QVector<ControllerView *> ControllerRackView::controllerViews() const
{
	return m_controllerViews;
}




void ControllerRackView::setAllCollapsed(bool allCollapsed)
{
	m_allCollapsed = allCollapsed;
}




void ControllerRackView::setAllExpanded(bool allExpanded)
{
	m_allExpanded = allExpanded;
}




bool ControllerRackView::allCollapsed() const
{
	return m_allCollapsed;
}




bool ControllerRackView::allExpanded() const
{
	return m_allExpanded;
}




QMdiSubWindow *ControllerRackView::subWin() const
{
	return m_subWin;
}




void ControllerRackView::closeEvent(QCloseEvent * ce)
{
	if (parentWidget())
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	ce->ignore();
}




void ControllerRackView::resizeEvent(QResizeEvent *)
{
	m_subWin->setFixedWidth( m_scrollArea->verticalScrollBar()->isVisible() ? 262 : 249 );
}




void ControllerRackView::paintEvent(QPaintEvent *)
{
	m_subWin->setFixedWidth(m_scrollArea->verticalScrollBar()->isVisible() ? 262 : 249);
	m_scrollArea->verticalScrollBar()->show();
}
