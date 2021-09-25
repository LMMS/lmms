/*
 * HexMenu.h - hexagonal menu widget
 *
 * Copyright (c) 2021 Alex <allejok96/gmail>
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


#ifndef CIRCULAR_MENU_H
#define CIRCULAR_MENU_H

#include <QWidget>
#include <QMenu>
#include "lmms_export.h"

class LMMS_EXPORT HexMenu : public QWidget
{
	Q_OBJECT
public:
	HexMenu(QWidget* parent = nullptr);
	virtual ~HexMenu();
	void addAction(QAction* action);
	void addMenu(QMenu* menu);

protected slots:
	void mouseMoveEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	bool m_mouseHasMoved = false;

	QMenu* m_contextMenu = nullptr;

	std::vector<QAction*> m_actions;
	int m_hoveredAction;
};

#endif
