/*
 * ActionGroup.h - wrapper around QActionGroup to provide a more useful triggered(int) slot
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

#ifndef LMMS_GUI_ACTION_GROUP_H
#define LMMS_GUI_ACTION_GROUP_H

#include <QActionGroup>

namespace lmms::gui
{

/// \brief Convenience subclass of QActionGroup
///
/// This class provides the same functionality as QActionGroup, but in addition
/// has the actionTriggered(int) signal.
/// It also sets every added action's checkable property to true.
class ActionGroup : public QActionGroup
{
	Q_OBJECT
public:
	ActionGroup(QObject* parent);

	QAction* addAction(QAction *a);
	QAction* addAction(const QString &text);
	QAction* addAction(const QIcon &icon, const QString &text);

signals:
	/// This signal is emitted when the action at the given index is triggered.
	void triggered(int index);

private slots:
	void actionTriggered_(QAction* action);

private:
	QList<QAction*> m_actions;
};

} // namespace lmms::gui

#endif // LMMS_GUI_ACTION_GROUP_H
