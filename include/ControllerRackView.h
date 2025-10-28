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

#ifndef LMMS_GUI_CONTROLLER_RACK_VIEW_H
#define LMMS_GUI_CONTROLLER_RACK_VIEW_H

#include <QWidget>

#include "SerializingObject.h"


class QPushButton;
class QScrollArea;
class QVBoxLayout;


namespace lmms
{

class Controller;

namespace gui
{

class ControllerView;


class ControllerRackView : public QWidget, public SerializingObject
{
	Q_OBJECT
public:
	ControllerRackView();
	~ControllerRackView() override = default;

	void saveSettings( QDomDocument & _doc, QDomElement & _parent ) override;
	void loadSettings( const QDomElement & _this ) override;

	inline QString nodeName() const override
	{
		return "ControllerRackView";
	}


public slots:
	void deleteController(ControllerView* view);
	void moveUp(ControllerView* view);
	void moveDown(ControllerView* view);
	void addController(Controller* controller);
	void removeController(Controller* controller);

protected:
	void closeEvent( QCloseEvent * _ce ) override;

private slots:
	void addController();


private:
	QVector<ControllerView *> m_controllerViews;

	QScrollArea * m_scrollArea;
	QVBoxLayout * m_scrollAreaLayout;
	QPushButton * m_addButton;

	// Stores the index of where to insert the next ControllerView.
	// Needed so that the StretchItem always stays at the last position.
	int m_nextIndex;
} ;

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_CONTROLLER_RACK_VIEW_H
