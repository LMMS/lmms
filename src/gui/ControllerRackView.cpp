/*
 * ControllerRackView.cpp - view for song's controllers
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2010-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2019 Steffen Baranowsky <BaraMGB/at/freenet.de>
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
#include <QAction>
#include <QPushButton>
#include <QScrollArea>
#include <QMessageBox>
#include <QVBoxLayout>

#include "ControllerView.h"
#include "DeprecationHelper.h"
#include "embed.h"
#include <QMdiSubWindow>

#include "GuiApplication.h"
#include "LfoController.h"
#include "MainWindow.h"
#include "Song.h"
#include "SubWindow.h"

namespace lmms::gui
{


ControllerRackView::ControllerRackView()
	: QWidget{}
	, m_collapsingStateOnLoad()
{
	setWindowIcon( embed::getIconPixmap( "controller" ) );
	setWindowTitle( tr( "Controller Rack" ) );

	m_subWin = getGUI()->mainWindow()->addWindowedWidget(this);

	m_scrollArea = new QScrollArea(m_subWin);
	m_scrollArea->setPalette(QApplication::palette(m_scrollArea));
	m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	auto scrollAreaWidget = new QWidget(m_scrollArea);
	m_scrollAreaLayout = new QVBoxLayout( scrollAreaWidget );
	m_scrollAreaLayout->addStretch();
	m_scrollAreaLayout->setContentsMargins(0, 0, 0, 0);
	m_scrollAreaLayout->setSpacing(0);
	scrollAreaWidget->setLayout( m_scrollAreaLayout );

	m_scrollArea->setWidget( scrollAreaWidget );
	m_scrollArea->setWidgetResizable( true );

	m_addButton = new QPushButton(m_subWin);
	m_addButton->setText( tr( "Add" ) );

	connect( m_addButton, SIGNAL(clicked()),
			this, SLOT(addController()));

	Song * song = Engine::getSong();
	connect(song, &Song::controllerAdded, this, qOverload<Controller*>(&ControllerRackView::addController));
	connect(song, &Song::controllerRemoved, this, &ControllerRackView::removeController);

	auto layout = new QVBoxLayout();
	layout->addWidget( m_scrollArea );
	layout->addWidget( m_addButton );
	layout->setContentsMargins(0, 0, 0, 0);
	this->setLayout( layout );

	setFixedWidth(254);
	resize(254, 400);
	setMinimumHeight(200);

	Qt::WindowFlags flags = m_subWin->windowFlags();
	flags &= ~Qt::WindowMaximizeButtonHint;
	m_subWin->setWindowFlags(flags);

	m_subWin->setAttribute(Qt::WA_DeleteOnClose, false);
	m_subWin->move(680, 60);
}




void ControllerRackView::saveSettings(QDomDocument &, QDomElement & parent)
{
	parent.setAttribute(QString("controllerCount"), m_controllerViews.size());
	for (auto i = 1; i <= m_controllerViews.size(); i++)
	{
		bool collapsed = m_controllerViews.at(i - 1)->isCollapsed();
		parent.setAttribute(QString("controllerView") + QString::number(i), collapsed);
	}
	MainWindow::saveWidgetState(this, parent);
}




void ControllerRackView::loadSettings(const QDomElement & _this)
{
	int controllerCount = _this.attribute(QString("controllerCount")).toInt();
	for (auto i = 1; i <= controllerCount; i++)
	{
		bool collapsingState = _this.attribute(QString("controllerView") + QString::number(i)).toInt();
		m_collapsingStateOnLoad.append(collapsingState);
	}
	MainWindow::restoreWidgetState(this, _this);
}




void ControllerRackView::deleteController( ControllerView * _view )
{
	Controller * c = _view->getController();

	if( c->connectionCount() > 0 )
	{
		QMessageBox msgBox;
		msgBox.setIcon( QMessageBox::Question );
		msgBox.setWindowTitle( tr("Confirm Delete") );
		msgBox.setText( tr("Confirm delete? There are existing connection(s) "
				"associated with this controller. There is no way to undo.") );
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		if( msgBox.exec() != QMessageBox::Ok )
		{
			return;
		}
	}

	Song * song = Engine::getSong();
	song->removeController( c );
}

void ControllerRackView::moveUp(ControllerView* view)
{
	if (view != m_controllerViews.first())
	{
		int index = m_controllerViews.indexOf(view);
		m_scrollAreaLayout->removeWidget(view);
		m_scrollAreaLayout->insertWidget(index - 1, view);
		m_controllerViews.remove(index);
		m_controllerViews.insert(index - 1, view);
		Engine::getSong()->moveControllerUp(view->getController());
	}
}




void ControllerRackView::moveDown(ControllerView* view)
{
	if (view != m_controllerViews.last())
	{
		int index = m_controllerViews.indexOf(view);
		moveUp(m_controllerViews.at(index + 1));
	}
}

void ControllerRackView::addController(Controller* controller)
{
	QWidget * scrollAreaWidget = m_scrollArea->widget();

	auto controllerView = new ControllerView(controller, scrollAreaWidget);
	connect(controllerView, &ControllerView::controllerCollapsed, this, &ControllerRackView::onControllerCollapsed);
	connect(controllerView, &ControllerView::collapseAll, this, &ControllerRackView::collapsingAll);
	connect(controllerView, &ControllerView::expandAll, this, &ControllerRackView::expandAll);
	connect(controllerView, &ControllerView::movedUp, this, &ControllerRackView::moveUp);
	connect(controllerView, &ControllerView::movedDown, this, &ControllerRackView::moveDown);
	connect(controllerView, &ControllerView::removedController, this, &ControllerRackView::deleteController, Qt::QueuedConnection);

	auto moveUpAction = new QAction(controllerView);
	moveUpAction->setShortcut(keySequence(Qt::Key_Up, Qt::AltModifier));
	moveUpAction->setShortcutContext(Qt::WidgetShortcut);
	connect(moveUpAction, &QAction::triggered, controllerView, &ControllerView::moveUp);
	controllerView->addAction(moveUpAction);

	auto moveDownAction = new QAction(controllerView);
	moveDownAction->setShortcut(keySequence(Qt::Key_Down, Qt::AltModifier));
	moveDownAction->setShortcutContext(Qt::WidgetShortcut);
	connect(moveDownAction, &QAction::triggered, controllerView, &ControllerView::moveDown);
	controllerView->addAction(moveDownAction);

	m_controllerViews.append( controllerView );
	int n = m_scrollAreaLayout->count() - 1;
	m_scrollAreaLayout->insertWidget(n, controllerView);

	if (Engine::getSong()->isLoadingProject())
	{
		if (!m_collapsingStateOnLoad.empty() && m_collapsingStateOnLoad.at(m_controllerViews.size() - 1))
		{
			controllerView->collapseController(true);
		}
	}

	update();
}

void ControllerRackView::removeController(Controller* removedController)
{
	for (auto &it : m_controllerViews)
	{
		if (it->getController() == removedController)
		{
			ControllerView * viewOfRemovedController = it;
			m_controllerViews.erase(std::find(m_controllerViews.begin(),
						m_controllerViews.end(), viewOfRemovedController));

			delete viewOfRemovedController;
			update();
			break;
		}
	}
}




void ControllerRackView::addController()
{
	// TODO: Eventually let the user pick from available controller types

	Engine::getSong()->addController( new LfoController( Engine::getSong() ) );

	// fix bug which always made ControllerRackView loose focus when adding
	// new controller
	setFocus();
}


void ControllerRackView::collapsingAll()
{
	for (auto &it : m_controllerViews)
	{
		it->collapseController(true);
	}
}

void ControllerRackView::expandAll()
{
	for (auto &it : m_controllerViews)
	{
		it->collapseController(false);
	}
}



void ControllerRackView::onControllerCollapsed()
{
	update();
}


const QVector<ControllerView *> ControllerRackView::controllerViews() const
{
	return m_controllerViews;
}




bool ControllerRackView::allCollapsed() const
{
	for (auto &it : m_controllerViews)
	{
		if (!it->isCollapsed())
		{
			return false;
		}
	}
	return true;
}




bool ControllerRackView::allExpanded() const
{
	for (auto &it : m_controllerViews)
	{
		if (it->isCollapsed())
		{
			return false;
		}
	}
	return true;
}




QMdiSubWindow *ControllerRackView::subWin() const
{
	return m_subWin;
}





} // namespace lmms::gui
