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

#include "embed.h"

#include <QAction>
#include <QToolBar>


void Editor::setPauseIcon(bool displayPauseIcon)
{
	// If we're playing, show a pause icon
	if (displayPauseIcon)
		m_playButton->setIcon(embed::getIconPixmap("pause"));
	else
		m_playButton->setIcon(embed::getIconPixmap("play"));
}

Editor::Editor(bool record) :
	m_toolBar(new QToolBar(this)),
	m_playButton(nullptr),
	m_recordButton(nullptr),
	m_recordAccompanyButton(nullptr),
	m_stopButton(nullptr)
{
	auto addButton = [this](const char* pixmap_name, QString text, QString objectName) {
		ToolButton* button = new ToolButton(embed::getIconPixmap(pixmap_name), text);
		button->setObjectName(objectName);
		m_toolBar->addWidget(button);
		return button;
	};

	// Set up play button
	m_playButton = addButton("play", tr("Play (Space)"), "playButton");

	// Set up record buttons if wanted
	if (record)
	{
		m_recordButton= addButton("record", tr("Record"), "recordButton");
		m_recordAccompanyButton = addButton("record_accompany", tr("Record while playing"), "recordAccompanyButton");
	}

	// Set up stop button
	m_stopButton = addButton("stop", tr("Stop (Space)"), "stopButton");

	// Add toolbar to window
	addToolBar(Qt::TopToolBarArea, m_toolBar);
}

Editor::~Editor()
{

}
