/*
 * Menu.h - A subclass of QMenu used to fix unexpected behavior of
 *           the QMenu
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

#ifndef MENU_H
#define MENU_H

#include <QMenu>
#include <QWidget>
#include <QString>

namespace lmms::gui
{

class Menu: public QMenu
{
public:
	Menu(const QString &title, QWidget *parent = nullptr): QMenu(title,parent) {};
	Menu(QWidget *parent = nullptr): QMenu(parent) {};

	void keyReleaseEvent(QKeyEvent *event) override;
};

} // namespace lmms::gui

#endif
