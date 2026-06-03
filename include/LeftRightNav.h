/*
 * LeftRightNav.h - side-by-side left-facing and right-facing arrows for navigation (looks like < > )
 *
 * Copyright (c) 2015 Colin Wallace <wallacoloo/at/gmail.com>
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

#ifndef LMMS_GUI_LEFT_RIGHT_NAV_H
#define LMMS_GUI_LEFT_RIGHT_NAV_H

#include <QPushButton>


namespace lmms::gui
{

class LeftRightNav : public QWidget
{
	Q_OBJECT
public:
	LeftRightNav(QWidget *parent=nullptr);
	void setShortcuts(const QKeySequence &leftShortcut=Qt::Key_Minus, const QKeySequence &rightShortcut=Qt::Key_Plus);
signals:
	void onNavLeft();
	void onNavRight();
private:
	QHBoxLayout m_layout;
	QPushButton m_leftBtn;
	QPushButton m_rightBtn;
};


} // namespace lmms::gui

#endif // LMMS_GUI_LEFT_RIGHT_NAV_H
