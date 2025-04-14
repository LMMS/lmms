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

class LMMS_EXPORT ActionSafeFnPtrTyped
{
public:
	// the typenames are placed in this way so
	// the class is the same size with different types -> doesn't have to be casted when stored + avoids mistakes
	// still enforces member functions
	template<typename BaseType, DataType>
	typedef void (BaseType::*FunctionPointer)(std::shared_ptr<DataType>)
	template<typename BaseType, DataType>
	ActionSafeFnPtrTyped(FunctionPointer function)
	{
		m_functionPtr = function;
		m_baseTypeId = typeid(BaseType).hash_code();
		m_dataTypeId = typeid(DataType).hash_code();
	}
	template<typename BaseType, DataType>
	void callFn(BaseType* object, std::shared_ptr<DataType> data)
	{
		assert(m_baseTypeId == typeid(BaseType).hash_code());
		assert(m_dataTypeId == typeid(DataType).hash_code());
		// firstly the `FunctionPointer<BaseType, DataType>` will be casted to a `void*`,
		// then it will be casted back to `FunctionPointer<BaseType, DataType>`
		((*object).*((FunctionPointer<BaseType, DataType>)(m_functionPtr)))(data);
	}
private:
	size_t m_baseTypeId;
	size_t m_dataTypeId;
	void* m_functionPtr;
};



class LMMS_EXPORT ActionSafeFnPtr
{
public:
	// the typenames are placed in this way so
	// the class is the same size with different types -> doesn't have to be casted when stored + avoids mistakes
	// still enforces member functions
	template<typename BaseType>
	typedef void (BaseType::*FunctionPointer)();
	template<typename BaseType>
	ActionSafeFnPtr(FunctionPointer<BaseType> function);
	template<typename BaseType>
	void callFn(BaseType* object);
private:
	size_t m_baseTypeId;
	void* m_functionPtr;
};


template<typename BaseType, DataType>
class LMMS_EXPORT GuiAction : public QUndoCommand
{
public:
	GuiAction(const QString& name, BaseType* object, ActionSafeFnPtr doFn, ActionSafeFnPtr undoFn, size_t runAmount);
	GuiAction(const QString& name, BaseType* object, ActionSafeFnPtrTyped doFn, ActionSafeFnPtrTyped undoFn, std::shared_ptr<DataType> data);
	~GuiAction();
	
	void undo() override;
    void redo() override;
private:
	const QString m_name;
	
	int m_runAmount;
	std::shared_ptr<DataType> m_data;
	
	ActionSafeFnPtr m_doFn;
	ActionSafeFnPtr m_undoFn;
	ActionSafeFnPtrTyped m_doTypedFn;
	ActionSafeFnPtrTyped m_undoTypedFn;
};

} // namespace lmms::gui

#endif // LMMS_GUI_ACTION_H
