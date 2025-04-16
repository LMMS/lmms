/*
 * GuiAction.h - implements Actions, a layer between the user and widgets
 *
 * Copyright (c) 2025 szeli1 <TODO/at/gmail/dot/com>
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

#include <cassert>
#include <memory>

#include <QString>
#include <QUndoCommand>

namespace lmms::gui
{

class InteractiveModelView;

class LMMS_EXPORT ActionSafeFnPtr
{
public:
	// the typenames are placed in this way -> doesn't have to be casted when stored
	// the class is the same size with different types -> avoids mistakes
	// still enforces the right function format
	template<typename DataType>
	typedef void (*FunctionPointer)(InteractiveModelView*, std::shared_ptr<DataType>)
	template<typename DataType>
	ActionSafeFnPtr(FunctionPointer function)
	{
		m_functionPtr = function;
		m_dataTypeId = typeid(DataType).hash_code();
	}
	template<typename DataType>
	void callFn(InteractiveModelView* object, std::shared_ptr<DataType> data)
	{
		assert(m_dataTypeId == typeid(DataType).hash_code());
		// firstly the `FunctionPointer<DataType>` will be casted to a `void*`, so it can be stored without templates,
		// then it will be casted back to `FunctionPointer<DataType>`, so it is safe to use
		*((FunctionPointer<DataType>)(m_functionPtr))(object, data);
	}
	bool isValid() const { return m_functionPtr; }
private:
	size_t m_dataTypeId;
	void* m_functionPtr;
};


template<typename DataType>
class LMMS_EXPORT GuiAction : public QUndoCommand
{
public:
	typedef void (*TypelessFn)(InteractiveModelView*);
	GuiAction(const QString& name, InteractiveModelView* object, TypelessFn doFn, TypelessFn undoFn, size_t runAmount);
	GuiAction(const QString& name, InteractiveModelView* object, ActionSafeFnPtr doFn, ActionSafeFnPtr undoFn, std::shared_ptr<DataType> data);
	~GuiAction();
	
	void undo() override;
	void redo() override;
	
	//! returns true if cleared, use this to delete pointers to destructing objects
	bool clearObjectIfMatch(InteractiveModelView* object);
private:
	const QString m_name;
	InteractiveModelView* m_target;
	
	int m_runAmount;
	std::shared_ptr<DataType> m_data;
	
	ActionSafeFnPtr m_doFn;
	ActionSafeFnPtr m_undoFn;
	ActionSafeFnPtrTyped m_doTypedFn;
	ActionSafeFnPtrTyped m_undoTypedFn;
};

} // namespace lmms::gui

#endif // LMMS_GUI_ACTION_H
