/*
 * LinkedModelGroupViews.h - view for groups of linkable models
 *
 * Copyright (c) 2019-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef LMMS_GUI_LINKED_MODEL_GROUP_VIEWS_H
#define LMMS_GUI_LINKED_MODEL_GROUP_VIEWS_H

#include <cstddef>
#include <memory>
#include <QWidget>

namespace lmms
{


class LinkedModelGroup;
class LinkedModelGroups;


namespace gui
{

class Control;  // IWYU pragma: keep

/**
	@file LinkedModelGroupViews.h
	See Lv2ViewBase.h for example usage
*/


/**
	View for a representative processor

	Features:
	* Remove button for removable models
	* Simple handling of adding, removing and model changing

	@note Neither this class, nor any inheriting classes, shall inherit
		ModelView. The "view" in the name is just for consistency
		with LinkedModelGroupsView.
*/
class LinkedModelGroupView : public QWidget
{
public:
	/**
		@param colNum numbers of columns for the controls
			(link LEDs not counted)
	*/
	LinkedModelGroupView(QWidget* parent, LinkedModelGroup* model,
		std::size_t colNum);
	~LinkedModelGroupView() override = default;

	//! Reconnect models if model changed
	void modelChanged(LinkedModelGroup* linkedModelGroup);

protected:
	//! Add a control to this widget
	//! @warning This widget will own this control, do not free it
	void addControl(Control* ctrl, const std::string &id,
					const std::string& display, bool removable);

	void removeControl(const QString &key);

	void removeFocusFromSearchBar();

private:
	class LinkedModelGroup* m_model;

	//! column number in surrounding grid in LinkedModelGroupsView
	std::size_t m_colNum;
	class ControlLayout* m_layout;
	std::map<std::string, std::unique_ptr<class Control>> m_widgets;
};


/**
	Container class for one LinkedModelGroupView

	@note It's intended this class does not inherit from ModelView.
		Inheriting classes need to do that, see e.g. Lv2Instrument.h
*/
class LinkedModelGroupsView
{
protected:
	~LinkedModelGroupsView() = default;

	//! Reconnect models if model changed; to be called by child virtuals
	void modelChanged(LinkedModelGroups* ctrlBase);

private:
	//! The base class must return the addressed group view,
	//! which has the same value as "this"
	virtual LinkedModelGroupView* getGroupView() = 0;
};


} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_LINKED_MODEL_GROUP_VIEWS_H
