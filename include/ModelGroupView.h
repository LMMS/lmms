/*
 * ModelGroupView.h - view for groups of models
 *
 * Copyright (c) 2019-2024 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef LMMS_GUI_MODEL_GROUP_VIEWS_H
#define LMMS_GUI_MODEL_GROUP_VIEWS_H

#include <QWidget>
#include <cstddef>
#include <memory>

namespace lmms {

class ModelGroup;

namespace gui {

class Control;

/**
	@file ModelGroupView.h
	See Lv2ViewBase.h for example usage
*/

class ModelGroupView
{
public:
	//! @param colNum numbers of columns for the controls
	ModelGroupView(QWidget* parent, ModelGroup* model);

	//! Reconnect models if model changed
	void modelChanged(ModelGroup* modelGroup);

protected:
	//! Add a control to this widget
	//! @warning This widget will own this control, do not free it
	void addControl(QWidget* parent, Control* ctrl, const std::string& id, const std::string& display, bool removable);

	void removeControl(const QString& key);

	void removeFocusFromSearchBar();

private:
	class ModelGroup* m_model;

	//! column number in surrounding grid in ModelGroupView
	std::size_t m_colNum;
	class ControlLayout* m_layout;
	std::map<std::string, std::unique_ptr<class Control>> m_widgets;
};

} // namespace gui

} // namespace lmms

#endif // LMMS_GUI_MODEL_GROUP_VIEWS_H
