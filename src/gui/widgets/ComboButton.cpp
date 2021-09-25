/*
 * ComboButton.cpp - a QToolButton that remembers its last used action
 *
 * Copyright (c) 2021 Alex <allejok96>
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


#include "ComboButton.h"

#include <QAction>
#include <QWheelEvent>

#include "embed.h"


ComboButton::ComboButton(QWidget* parent, bool triggerOnScroll) :
	QToolButton(parent),
	m_triggerOnScroll(triggerOnScroll)
{
	setPopupMode(QToolButton::MenuButtonPopup);
}


void ComboButton::addAction(QAction* action)
{
	QToolButton::addAction(action);
	if (actions().length() == 1)
	{
		setDefaultAction(action);
	}

	connect(action, &QAction::triggered, [this, action](){ setDefaultAction(action); });
}

QAction* ComboButton::addAction(const QString& pixmap, const QString& text)
{
	QAction* action = new QAction(embed::getIconPixmap(pixmap), text, this);
	addAction(action);
	return action;
}

void ComboButton::addActions(QList<QAction*> actions)
{
	for (auto action: actions)
	{
		addAction(action);
	}
}

void ComboButton::wheelEvent(QWheelEvent* event)
{
	// Move to next/previous depending on scroll
	int next = actions().indexOf(defaultAction()) + ((event->angleDelta().y() < 0) ? 1 : -1);
	if (0 <= next && next < actions().length())
	{
		if (m_triggerOnScroll)
		{
			actions()[next]->trigger();
		}
		else
		{
			setDefaultAction(actions()[next]);
		}
	}
}


