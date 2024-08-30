/*
 * InteractiveModelView.h - TODO
 *
 * Copyright (c) 2024 szeli1 <TODO/at/gmail/dot/com>
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

#include "InteractiveModelView.h"

#include <algorithm>

#include <QKeyEvent>

namespace lmms::gui
{

InteractiveModelView::InteractiveModelView(QWidget* widget) :
	QWidget(widget),
	m_isHighlighted(false),
	m_lastShortcutCounter(0)
{
	s_interactiveWidgets.push_back(this);

	m_lastShortcut.m_key = Qt::Key_F35;
	m_lastShortcut.m_modifier = Qt::NoModifier;
	m_lastShortcut.m_times = 0;
	m_lastShortcut.m_shortcutDescription = "";
}

InteractiveModelView::~InteractiveModelView()
{
	auto it = std::find(s_interactiveWidgets.begin(), s_interactiveWidgets.end(), this);
	if (it != s_interactiveWidgets.end())
	{
		s_interactiveWidgets.erase(it);
	}
}

void InteractiveModelView::startHighlighting(Clipboard::StringPairDataType dataType)
{
	for (auto it = s_interactiveWidgets.begin(); it != s_interactiveWidgets.end(); ++it)
	{
		if ((*it)->canAcceptClipBoardData(dataType))
		{
			(*it)->setIsHighlighted(true);
		}
	}
}

void InteractiveModelView::stopHighlighting()
{
	for (auto it = s_interactiveWidgets.begin(); it != s_interactiveWidgets.end(); ++it)
	{
		(*it)->setIsHighlighted(false);
	}
}

void InteractiveModelView::keyPressEvent(QKeyEvent* event)
{
	std::vector<ModelShortcut> shortcuts(getShortcuts());
	for (size_t i = 0; i < shortcuts.size(); i++)
	{
		if (doesShortcutMatch(&(shortcuts[i]), event, m_lastShortcutCounter))
		{
			ModelShortcut temp = shortcuts[i];
			if (m_lastShortcut == temp)
			{
				m_lastShortcutCounter++;
			}
			else
			{
				m_lastShortcut = shortcuts[i];
				m_lastShortcutCounter = 0;
			}
			qDebug("found shortcut");
			shortcutPressedEvent(i, event);
			break;
		}
	}
}

void InteractiveModelView::enterEvent(QEvent* event)
{
	m_lastShortcutCounter = 0;

	m_lastShortcut.m_key = Qt::Key_F35;
	m_lastShortcut.m_modifier = Qt::NoModifier;
	m_lastShortcut.m_times = 0;
	m_lastShortcut.m_shortcutDescription = "";
	
	qDebug("enter event");
}

void InteractiveModelView::leaveEvent(QEvent* event)
{
	qDebug("leave event");
}

bool InteractiveModelView::canAcceptClipBoardData(Clipboard::StringPairDataType dataType)
{
	return false;
}

bool InteractiveModelView::getIsHighlighted() const
{
	return m_isHighlighted;
}

void InteractiveModelView::setIsHighlighted(bool isHighlighted)
{
	if (m_isHighlighted != isHighlighted)
	{
		m_isHighlighted = isHighlighted;
		update();
	}
}

bool InteractiveModelView::doesShortcutMatch(const ModelShortcut* shortcut, QKeyEvent* event, unsigned int times) const
{
	return shortcut->m_key == event->key() && (event->modifiers() & shortcut->m_modifier) && shortcut->m_times == times;
}

} // namespace lmms::gui
