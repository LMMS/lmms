/*
 * ComboButton.h - a QToolButton that remembers its last used action
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

#ifndef COMBOBUTTON_H
#define COMBOBUTTON_H

#include <QToolButton>


class ComboButton : public QToolButton
{
	Q_OBJECT
public:
	ComboButton(QWidget* parent = nullptr, bool triggerOnScroll = false);

	void addAction(QAction* action);
	QAction* addAction(const QString& pixmap, const QString& text);

	void addActions(QList<QAction*> actions);

protected:
	void wheelEvent(QWheelEvent* event) override;

private:
	bool m_triggerOnScroll;
} ;

#endif
