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
#include <utility>

#include <QString>
//#include <QUndoCommand>

namespace lmms::gui
{

class InteractiveModelView;

class ActionSafeFnPtr
{
public:
	// the typenames are placed in this way -> doesn't have to be casted when stored
	// the class is the same size with different types -> avoids mistakes
	// still enforces the right function format
	template<typename DataType>
	static std::pair<void*, size_t> helpConstruct(void (*inputFunction)(InteractiveModelView*, DataType))
	{
		return std::make_pair(reinterpret_cast<void*>(inputFunction), typeid(DataType).hash_code());
	}
	ActionSafeFnPtr();
	ActionSafeFnPtr(std::pair<void*, size_t> input);
	ActionSafeFnPtr(const ActionSafeFnPtr& ref);
	void setFn(std::pair<void*, size_t> input);
	template<typename DataType>
	constexpr void callFn(InteractiveModelView* object, DataType data)
	{
		qDebug("type assert debug: %ld, %ld, %s", m_dataTypeId, typeid(DataType).hash_code(), typeid(DataType).name());
		if (m_functionPtr == nullptr) { return; }
		assert(m_dataTypeId == typeid(DataType).hash_code());
		// firstly the FunctionPointer will be casted to a `void*`, so it can be stored without templates,
		// then it will be casted back to FunctionPointer, so it is safe to use
		void (*functionPtr)(InteractiveModelView*, DataType);
		functionPtr = reinterpret_cast<void (*)(InteractiveModelView*, DataType)>(m_functionPtr);
		functionPtr(object, data);
	}
	bool isValid() const { return m_functionPtr; }
	bool isMatch(void* functionPtr) const { return functionPtr == m_functionPtr; }
private:
	size_t m_dataTypeId;
	void* m_functionPtr;
};

typedef void (*ActionTypelessFnPtr)(InteractiveModelView*);

template<typename DataType>
class GuiAction// : public QUndoCommand
{
public:
	GuiAction(const QString& name, InteractiveModelView* object, ActionTypelessFnPtr doFn, ActionTypelessFnPtr undoFn, size_t runAmount, bool linkBack) :
		m_target(object),
		m_runAmount(runAmount),
		m_data(nullptr),
		m_doFn(doFn),
		m_undoFn(undoFn),
		m_doTypedFn(),
		m_undoTypedFn()
	{}
	GuiAction(const QString& name, InteractiveModelView* object, ActionSafeFnPtr doFn, ActionSafeFnPtr undoFn, DataType data, bool linkBack) :
		m_target(object),
		m_runAmount(0),
		m_data(nullptr),
		m_doFn(nullptr),
		m_undoFn(nullptr),
		m_doTypedFn(doFn),
		m_undoTypedFn(undoFn)
	{
		m_data = std::make_unique<DataType>(data);
	}

	void undo()// override
	{
		if (m_target == nullptr) { return; }
		if (m_undoFn != nullptr)
		{
			for (size_t i = 0; i < m_runAmount; i++)
			{
				m_undoFn(m_target);
			}
		}
		else if (m_doTypedFn.isValid())
		{
			m_undoTypedFn.callFn<DataType>(m_target, *m_data.get());
		}
	}
	void redo()// override
	{
		if (m_target == nullptr) { return; }
		if (m_doFn != nullptr)
		{
			for (size_t i = 0; i < m_runAmount; i++)
			{
				m_doFn(m_target);
			}
		}
		else if (m_doTypedFn.isValid())
		{
			m_doTypedFn.callFn<DataType>(m_target, *m_data.get());
		}
	}
	//! returns true if cleared, use this to delete pointers to destructing objects
	bool clearObjectIfMatch(InteractiveModelView* object)
	{
		if (m_target == nullptr) { return false; }
		if (object == m_target)
		{
			m_target = nullptr;
			m_data = nullptr;
			return true;
		}
		return false;
	}
private:
	const QString m_name;
	InteractiveModelView* m_target;
	bool m_isLinkedBack; //! if this is undone, undo the one before this
	
	size_t m_runAmount;
	std::unique_ptr<DataType> m_data;
	
	ActionTypelessFnPtr m_doFn;
	ActionTypelessFnPtr m_undoFn;
	ActionSafeFnPtr m_doTypedFn;
	ActionSafeFnPtr m_undoTypedFn;
};

} // namespace lmms::gui

#endif // LMMS_GUI_ACTION_H
