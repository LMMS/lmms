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
		// if this assert fails, you need to change your `DataType` template to the one this one was constructed with (`helpConstruct<DataType>`)
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


class AbstractGuiAction// : public QUndoCommand
{
public:
	AbstractGuiAction(const QString& name, InteractiveModelView* object, bool linkBack);
	~AbstractGuiAction() {}
	//! should be undone along with the action before this
	bool getShouldLinkBack();
	//! returns true if cleared, use this to delete pointers to destructing objects
	bool clearObjectIfMatch(InteractiveModelView* object);
protected:
	const QString m_name;
	InteractiveModelView* m_target;
	bool m_isLinkedBack; //! if this is undone, undo the one before this
};


typedef void (*ActionTypelessFnPtr)(InteractiveModelView*);
class GuiAction : public AbstractGuiAction
{
public:
	GuiAction(const QString& name, InteractiveModelView* object, ActionTypelessFnPtr doFn, ActionTypelessFnPtr undoFn, size_t runAmount, bool linkBack);
	void undo();// override
	void redo();// override
private:
	size_t m_runAmount;
	ActionTypelessFnPtr m_doFn;
	ActionTypelessFnPtr m_undoFn;
};



template<typename DataType>
class GuiActionTyped : public AbstractGuiAction
{
public:
	GuiActionTyped(const QString& name, InteractiveModelView* object, ActionSafeFnPtr doFn, ActionSafeFnPtr undoFn, DataType data, bool linkBack) :
		AbstractGuiAction(name, object, linkBack),
		m_data(nullptr),
		m_doFn(doFn),
		m_undoFn(undoFn)
	{
		m_data = std::make_unique<DataType>(data);
	}
	void undo()// override
	{
		if (m_target == nullptr || m_undoFn.isValid() == false) { return; }
		m_undoFn.callFn<DataType>(m_target, *m_data.get());
	}
	void redo()// override
	{
		if (m_target == nullptr || m_doFn.isValid() == false) { return; }
		m_doFn.callFn<DataType>(m_target, *m_data.get());
	}
private:
	std::unique_ptr<DataType> m_data;
	ActionSafeFnPtr m_doFn;
	ActionSafeFnPtr m_undoFn;
};

} // namespace lmms::gui

#endif // LMMS_GUI_ACTION_H
