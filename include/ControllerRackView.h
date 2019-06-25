/*
 * ControllerRackView.h - view for song's controllers
 *
 * Copyright (c) 2008-2009 Paul Giblock <drfaygo/at/gmail.com>
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

#ifndef CONTROLLER_RACK_VIEW_H
#define CONTROLLER_RACK_VIEW_H

#include <QCloseEvent>
#include <QMdiSubWindow>
#include <QWidget>

#include "lmms_basics.h"
#include "SerializingObject.h"


class QPushButton;
class QScrollArea;
class QVBoxLayout;

class ControllerView;
class Controller;


class ControllerRackView : public QWidget, public SerializingObject
{
	Q_OBJECT
public:
	ControllerRackView();
	virtual ~ControllerRackView();

	virtual void saveSettings(QDomDocument &, QDomElement & parent);
	virtual void loadSettings(const QDomElement & _this);

	inline virtual QString nodeName() const
	{
		return "ControllerRackView";
	}

	QMdiSubWindow *subWin() const;

	bool allExpanded() const;
	bool allCollapsed() const;

public slots:
	void deleteController(ControllerView * view);
	void collapsingAll();
	void expandAll();
	void onControllerAdded(Controller *);
	void onControllerRemoved(Controller *);
	void onControllerCollapsed();
	void setAllExpanded(bool allExpanded);
	void setAllCollapsed(bool allCollapsed);

protected:
	virtual void closeEvent(QCloseEvent * ce);
	virtual void resizeEvent(QResizeEvent *);
	virtual void paintEvent(QPaintEvent *);

private slots:
	void addLfoController();
	void moveControllerUp(ControllerView * cv);
	void moveControllerDown(ControllerView * cv);

private:
	QVector<ControllerView *> m_controllerViews;

	QScrollArea * m_scrollArea;
	QVBoxLayout * m_scrollAreaLayout;
	QPushButton * m_addButton;
	QMdiSubWindow * m_subWin;

	bool m_allExpanded;
	bool m_allCollapsed;
};

#endif
