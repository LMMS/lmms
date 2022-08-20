/*
 * ComboButton.h - tool button that shows the last used action
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

#ifndef COMBO_BUTTON_H
#define COMBO_BUTTON_H

#include <QToolButton>

class QActionGroup;

namespace lmms::gui
{

class ComboButton : public QToolButton
{
	Q_OBJECT
public:
	ComboButton(QWidget* parent = nullptr);

	void addAction(QAction* action);
	QAction* addAction(const QString& pixmap, const QString& text);

	/*! \brief Trigger actions in a QActionGroup when scrolling this widget
	 *
	 *  By default, scrolling the button will switch its defaultAction. No actions are triggered.
	 *  But when connected to a QActionGroup, scrolling will trigger the next action in that group.
	 */
	void setActionGroupForScroll(QActionGroup* group) { m_actionGroup = group; }

	/*! \brief Capture wheel events and change the active action
	 *
	 *  When connected to an action group, scrolling this button may activate other buttons.
	 *  But the reverse can also be achieved using this event filter. To make scrolling another
	 *  widget change the active action of this button (and others in the same group), call:
	 *
	 *    otherWidget->installEventFilter(comboButton)
	 */
	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	QActionGroup* m_actionGroup = nullptr;
};

} // namespace lmms::gui

#endif
