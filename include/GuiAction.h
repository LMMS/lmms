/*
 * GuiAction.h - implements Actions, a layer between the user and widgets
 *
 * Copyright (c) 2024 - 2025 szeli1 <TODO/at/gmail/dot/com>
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

#ifndef LMMS_GUI_ACTION_H
#define LMMS_GUI_ACTION_H

#include <QString>
#include <QUndoCommand>

namespace lmms::gui
{

class InteractiveModelView;

typedef void (*ActionFn)();
typedef void (*FloatActionFn)(float, float);

//template<typename BaseType>
class LMMS_EXPORT GuiAction : public QUndoCommand
{
public:
	using 
	GuiAction(const QString& name, InteractiveModelView* object, ActionFn doFn, ActionFn undoFn, size_t runAmount);
	GuiAction(const QString& name, InteractiveModelView* object, FloatActionFn doFn, FloatActionFn undoFn, float valueA, float valueB);
	~GuiAction();
	
	void undo() override;
    void redo() override;
private:
	const QString m_name;
	
	int m_runAmount;
	float m_valueA;
	float m_valueB;
	
	ActionFn m_doFn;
	ActionFn m_undoFn;
	FloatActionFn m_doFloatFn;
	FloatActionFn m_undoFloatFn;
};

} // namespace lmms::gui

#endif // LMMS_GUI_ACTION_H
