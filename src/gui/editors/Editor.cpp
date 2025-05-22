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
	if (Engine::getSong()->isPlaying())
		stop();
	else
		play();
}

void Editor::togglePause()
{
	Engine::getSong()->togglePause();
}

void Editor::toggleMaximize()
{
	isMaximized() ? showNormal() : showMaximized();
}

Editor::Editor(bool record, bool recordAccompany, bool stepRecord) :
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

	// Play action setup
	m_playAction = new QAction(embed::getIconPixmap("play"), tr("Play (Space)"), this); //setup play action
	connect(m_playAction, SIGNAL(triggered()), this, SLOT(play())); // Set up connection for play action
	addButton(m_playAction, "playButton"); // Add actions to toolbar for play action

	// Record action setup
	if (record)
	{
		m_recordAction = new QAction(embed::getIconPixmap("record"), tr("Record"), this); //setup record action
		connect(m_recordAction, SIGNAL(triggered()), this, SLOT(record())); // Set up connection for record action
		addButton(m_recordAction, "recordButton"); // Add actions to toolbar for record action
	}

	// RecordAccompany action setup
	if (recordAccompany)
	{
		m_recordAccompanyAction = new QAction(embed::getIconPixmap("record_accompany"), tr("Record while playing"), this); //setup recordAccompany action
		connect(m_recordAccompanyAction, SIGNAL(triggered()), this, SLOT(recordAccompany())); // Set up connection for recordAccompany action
		addButton(m_recordAccompanyAction, "recordAccompanyButton"); // Add actions to toolbar for recordAccompany action
	}

	// StepRecord action setup
	if (stepRecord)
	{
		m_toggleStepRecordingAction = new QAction(embed::getIconPixmap("record_step_off"), tr("Toggle Step Recording"), this); //setup stepRecord action
		connect(m_toggleStepRecordingAction, SIGNAL(triggered()), this, SLOT(toggleStepRecording())); // Set up connection for stepRecord action
		addButton(m_toggleStepRecordingAction, "stepRecordButton"); // Add actions to toolbar for stepRecord action
	}

	// Stop action setup
	m_stopAction = new QAction(embed::getIconPixmap("stop"), tr("Stop (Space)"), this); //setup stop action
	connect(m_stopAction, SIGNAL(triggered()), this, SLOT(stop())); // Set up connection for stop action
	addButton(m_stopAction, "stopButton"); // Add actions to toolbar for stop action

	// Set up shortcuts for actions
	new QShortcut(Qt::Key_Space, this, SLOT(togglePlayStop()));
	new QShortcut(QKeySequence(combine(Qt::SHIFT, Qt::Key_Space)), this, SLOT(togglePause()));
	new QShortcut(QKeySequence(combine(Qt::SHIFT, Qt::Key_F11)), this, SLOT(toggleMaximize()));
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
