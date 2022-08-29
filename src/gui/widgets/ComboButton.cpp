/*
 * ComboButton.cpp - tool button that shows the last used action
 *
 * Copyright (c) 2022 Alex <allejok96/gmail>
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


namespace lmms::gui
{


ComboButton::ComboButton(QWidget* parent) :
	QToolButton(parent)
{
	setPopupMode(QToolButton::MenuButtonPopup);
	installEventFilter(this);
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
	auto action = new QAction(embed::getIconPixmap(pixmap), text, this);
	addAction(action);
	return action;
}





bool ComboButton::eventFilter(QObject *watched, QEvent *event)
{
	// Only filter out wheel events (return false to pass on events)
	if (event->type() != QEvent::Wheel) { return false; }

	auto wheelEvent = static_cast<QWheelEvent*>(event);

	// Disgard high-resolution wheel movement (needs better implementation!!!)
	int scroll = wheelEvent->angleDelta().y();
	if (std::abs(scroll) < 30) { return true; }

	const auto actionList = m_actionGroup ? m_actionGroup->actions() : actions();

	// Trigger next/previous action depending on scroll direction
	QAction* selected = m_actionGroup ? m_actionGroup->checkedAction() : defaultAction();
	int selectedIndex = selected ? actionList.indexOf(selected) : 0;
	int next = selectedIndex + ((scroll < 0) ? 1 : -1);
	if (0 <= next && next < actionList.length())
	{
		if (m_actionGroup)
		{
			actionList.at(next)->trigger();
		}
		else
		{
			setDefaultAction(actionList.at(next));
		}
	}

	return true;
}

} // namespace lmms
