/*
 * ActionGroup.cpp - wrapper around QActionGroup to provide a more useful triggered(int) slot
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

#include "ActionGroup.h"

ActionGroup::ActionGroup(QObject* parent) : QActionGroup(parent)
{
	connect(this, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered_(QAction*)));
}

QAction* ActionGroup::addAction(QAction* a)
{
	a->setCheckable(true);

	return QActionGroup::addAction(a);
}

QAction* ActionGroup::addAction(const QString& text)
{
	return addAction(new QAction(text, this));
}

QAction* ActionGroup::addAction(const QIcon& icon, const QString& text)
{
	return addAction(new QAction(icon, text, this));
}

void ActionGroup::actionTriggered_(QAction* action)
{
	Q_ASSERT(action != 0);
	Q_ASSERT(actions().contains(action));

	emit triggered(actions().indexOf(action));
}
