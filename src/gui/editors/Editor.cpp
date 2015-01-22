/*
 * Editor.cpp - implementation of Editor class
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
 *
 * This file is part of LMMS - http://lmms.io
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

#include "Editor.h"

#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QMdiArea>
#include <QShortcut>


void Editor::setPauseIcon(bool displayPauseIcon)
{
	// If we're playing, show a pause icon
	if (displayPauseIcon)
		m_playAction->setIcon(QPixmap("icons:pause.png"));
	else
		m_playAction->setIcon(QPixmap("icons:play.png"));
}

void Editor::togglePlayStop()
{
	if (Engine::getSong()->isPlaying())
		stop();
	else
		play();
}

Editor::Editor(bool record) :
	m_toolBar(new DropToolBar(this)),
	m_playAction(nullptr),
	m_recordAction(nullptr),
	m_recordAccompanyAction(nullptr),
	m_stopAction(nullptr)
{
	m_toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
	m_toolBar->setMovable(false);

	auto addButton = [this](QAction* action, QString objectName) {
		m_toolBar->addAction(action);
		m_toolBar->widgetForAction(action)->setObjectName(objectName);
	};

	// Set up play and record actions
	m_playAction = new QAction(QPixmap("icons:play"), tr("Play (Space).png"), this);
	m_stopAction = new QAction(QPixmap("icons:stop"), tr("Stop (Space).png"), this);

	m_recordAction = new QAction(QPixmap("icons:record"), tr("Record.png"), this);
	m_recordAccompanyAction = new QAction(QPixmap("icons:record_accompany"), tr("Record while playing.png"), this);

	// Set up connections
	connect(m_playAction, SIGNAL(triggered()), this, SLOT(play()));
	connect(m_recordAction, SIGNAL(triggered()), this, SLOT(record()));
	connect(m_recordAccompanyAction, SIGNAL(triggered()), this, SLOT(recordAccompany()));
	connect(m_stopAction, SIGNAL(triggered()), this, SLOT(stop()));
	new QShortcut(Qt::Key_Space, this, SLOT(togglePlayStop()));

	// Add toolbar to window
	addToolBar(Qt::TopToolBarArea, m_toolBar);

	// Add actions to toolbar
	addButton(m_playAction, "playButton");
	if (record)
	{
		addButton(m_recordAction, "recordButton");
		addButton(m_recordAccompanyAction, "recordAccompanyButton");
	}
	addButton(m_stopAction, "stopButton");
}

Editor::~Editor()
{

}




DropToolBar::DropToolBar(QWidget* parent) : QToolBar(parent)
{
	setAcceptDrops(true);
}

void DropToolBar::dragEnterEvent(QDragEnterEvent* event)
{
	dragEntered(event);
}

void DropToolBar::dropEvent(QDropEvent* event)
{
	dropped(event);
}
