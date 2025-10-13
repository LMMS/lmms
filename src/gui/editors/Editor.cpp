/*
 * Editor.cpp - implementation of Editor class
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
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

#include "Editor.h"

#include "DeprecationHelper.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "Song.h"

#include "embed.h"

#include <QAction>
#include <QShortcut>
#include <QCloseEvent>


namespace lmms::gui
{


void Editor::setPauseIcon(bool displayPauseIcon)
{
	// If we're playing, show a pause icon
	if (displayPauseIcon)
		m_playAction->setIcon(embed::getIconPixmap("pause"));
	else
		m_playAction->setIcon(embed::getIconPixmap("play"));
}

DropToolBar * Editor::addDropToolBarToTop(QString const & windowTitle)
{
	return addDropToolBar(Qt::TopToolBarArea, windowTitle);
}

DropToolBar * Editor::addDropToolBar(Qt::ToolBarArea whereToAdd, QString const & windowTitle)
{
	return addDropToolBar(this, whereToAdd, windowTitle);
}

DropToolBar * Editor::addDropToolBar(QWidget * parent, Qt::ToolBarArea whereToAdd, QString const & windowTitle)
{
	auto toolBar = new DropToolBar(parent);
	addToolBar(whereToAdd, toolBar);
	toolBar->setMovable(false);
	toolBar->setFloatable(false);
	toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
	toolBar->setWindowTitle(windowTitle);

	return toolBar;
}

void Editor::togglePlayStop()
{
	s_lastPlayedEditor = this;
	if (Engine::getSong()->isPlaying())
		stop();
	else
		play();
}

void Editor::togglePause()
{
	s_lastPlayedEditor = this;
	Engine::getSong()->togglePause();
}

void Editor::toggleMaximize()
{
	isMaximized() ? showNormal() : showMaximized();
}

Editor::Editor(bool record, bool stepRecord) :
	m_toolBar(new DropToolBar(this)),
	m_playAction(nullptr),
	m_recordAction(nullptr),
	m_recordAccompanyAction(nullptr),
	m_toggleStepRecordingAction(nullptr),
	m_stopAction(nullptr)
{
	m_toolBar = addDropToolBarToTop(tr("Transport controls"));

	auto addButton = [this](QAction* action, QString objectName) {
		m_toolBar->addAction(action);
		m_toolBar->widgetForAction(action)->setObjectName(objectName);
	};

	// Set up play and record actions
	m_playAction = new QAction(embed::getIconPixmap("play"), tr("Play (Space)"), this);
	m_stopAction = new QAction(embed::getIconPixmap("stop"), tr("Stop (Space)"), this);

	m_recordAction = new QAction(embed::getIconPixmap("record"), tr("Record"), this);
	m_recordAccompanyAction = new QAction(embed::getIconPixmap("record_accompany"), tr("Record while playing"), this);
	m_toggleStepRecordingAction = new QAction(embed::getIconPixmap("record_step_off"), tr("Toggle Step Recording"), this);

	// Set up connections
	connect(m_playAction, SIGNAL(triggered()), this, SLOT(play()));
	connect(m_recordAction, SIGNAL(triggered()), this, SLOT(record()));
	connect(m_recordAccompanyAction, SIGNAL(triggered()), this, SLOT(recordAccompany()));
	connect(m_toggleStepRecordingAction, SIGNAL(triggered()), this, SLOT(toggleStepRecording()));
	connect(m_stopAction, SIGNAL(triggered()), this, SLOT(stop()));
	new QShortcut(QKeySequence(combine(Qt::SHIFT, Qt::Key_F11)), this, SLOT(toggleMaximize()));

	// Add actions to toolbar
	addButton(m_playAction, "playButton");
	if (record)
	{
		addButton(m_recordAction, "recordButton");
		addButton(m_recordAccompanyAction, "recordAccompanyButton");
	}
	if(stepRecord)
	{
		addButton(m_toggleStepRecordingAction, "stepRecordButton");
	}
	addButton(m_stopAction, "stopButton");
}

QAction *Editor::playAction() const
{
	return m_playAction;
}

void Editor::closeEvent(QCloseEvent * event)
{
	if( parentWidget() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	getGUI()->mainWindow()->refocus();
	event->ignore();
 }

 void Editor::keyPressEvent(QKeyEvent *ke)
 {
	if (ke->key() == Qt::Key_Space)
	{
		if (ke->modifiers() & Qt::ShiftModifier)
		{
			togglePause();
		}
		else
		{
			togglePlayStop();
		}
		return;
	}
	ke->ignore();
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



} // namespace lmms::gui
