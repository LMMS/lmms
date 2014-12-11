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
#include "embed.h"

#include <QAction>
#include <QActionGroup>
#include <QMdiArea>


void Editor::setPauseIcon(bool displayPauseIcon)
{
	// If we're playing, show a pause icon
	if (displayPauseIcon)
		m_playAction->setIcon(embed::getIconPixmap("pause"));
	else
		m_playAction->setIcon(embed::getIconPixmap("play"));
}

int Editor::editMode() const
{
	return m_editMode;
}

void Editor::setEditMode(int mode)
{
	if (mode <= m_editModeGroup->actions().size())
	{
		m_editMode = mode;
	}
	emit(editModeChanged(mode));
}

QAction* Editor::addEditMode(const QIcon& icon, const QString& text, const QString& whatsThis)
{
	QAction* editModeAction = new QAction(icon, text, m_editModeGroup);
	editModeAction->setWhatsThis(whatsThis);
	editModeAction->setCheckable(true);
	return editModeAction;
}

void Editor::play()
{
}

void Editor::record()
{
}

void Editor::recordAccompany()
{
}

void Editor::stop()
{
}

void Editor::setEditModeByAction(QAction* action)
{
	int index = m_editModeGroup->actions().indexOf(action);
	if (index != -1)
		setEditMode(index);
}

Editor::Editor(bool record) :
	m_toolBar(new QToolBar(this)),
	m_playAction(nullptr),
	m_recordAction(nullptr),
	m_recordAccompanyAction(nullptr),
	m_stopAction(nullptr),
	m_editMode(0),
	m_editModeGroup(new QActionGroup(this))
{
	m_toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
	m_toolBar->setMovable(false);

	auto addButton = [this](const char* pixmap_name, QString text, QString objectName) {
		QAction* action = m_toolBar->addAction(embed::getIconPixmap(pixmap_name), text);
		m_toolBar->widgetForAction(action)->setObjectName(objectName);
		return action;
	};

	// Set up play button
	m_playAction = addButton("play", tr("Play (Space)"), "playButton");
	m_playAction->setShortcut(Qt::Key_Space);

	// Set up record buttons if wanted
	if (record)
	{
		m_recordAction = addButton("record", tr("Record"), "recordButton");
		m_recordAccompanyAction = addButton("record_accompany", tr("Record while playing"), "recordAccompanyButton");
	}

	// Set up stop button
	m_stopAction = addButton("stop", tr("Stop (Space)"), "stopButton");

	// Add toolbar to window
	addToolBar(Qt::TopToolBarArea, m_toolBar);

	// Set up connections
	connect(m_playAction, SIGNAL(triggered()), this, SLOT(play()));
	if (record)
	{
		connect(m_recordAction, SIGNAL(triggered()), this, SLOT(record()));
		connect(m_recordAccompanyAction, SIGNAL(triggered()), this, SLOT(recordAccompany()));
	}
	connect(m_stopAction, SIGNAL(triggered()), this, SLOT(stop()));

	// Connect edit mode
	connect(m_editModeGroup, SIGNAL(triggered(QAction*)), this, SLOT(setEditModeByAction(QAction*)));
}

Editor::~Editor()
{

}
