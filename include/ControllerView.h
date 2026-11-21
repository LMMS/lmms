/*
 * ControllerView.h - view-component for an control
 *
 * Copyright (c) 2008 Paul Giblock <drfaygo/at/gmail.com>
 * Copyright (c) 2019 Steffen Baranowsky <BaraMGB/at/freenet.de>
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

#ifndef LMMS_GUI_CONTROLLER_VIEW_H
#define LMMS_GUI_CONTROLLER_VIEW_H

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "AutomatableModel.h"
#include "Controller.h"
#include "ModelView.h"


namespace lmms::gui
{




class ControllerView : public QFrame, public ModelView
{
	Q_OBJECT
public:
	ControllerView( Controller * _controller, QWidget * _parent );
	~ControllerView() override = default;

	inline Controller * getController()
	{
		return castModel<Controller>();
	}

	inline const Controller * getController() const
	{
		return castModel<Controller>();
	}

	bool isCollapsed() const;


public slots:
	void removeController();
	void moveUp();
	void moveDown();
	void collapseController(bool collapse);
	void toggleCollapseController();
	void rename();
	void renameFinished();


signals:
	void removedController(ControllerView * view);
	void controllerCollapsed();
	void collapseAll();
	void expandAll();
	void movedUp(ControllerView * view);
	void movedDown(ControllerView * view);


protected:
	void contextMenuEvent( QContextMenuEvent * _me ) override;
	void modelChanged() override;
	void mouseDoubleClickEvent( QMouseEvent * event ) override;
	void paintEvent(QPaintEvent *) override;
	void dragEnterEvent(QDragEnterEvent * dee) override;
	void dropEvent(QDropEvent * de) override;


private:
	ControllerDialog * m_controllerDlg;
	const int m_titleBarHeight;
	QLabel * controllerTypeLabel;
	QLineEdit * m_nameLineEdit;
	QPushButton * m_collapseButton;
	Controller * m_modelC;
};

} // namespace lmms::gui

#endif // LMMS_GUI_CONTROLLER_VIEW_H
