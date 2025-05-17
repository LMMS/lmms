/*
 * GuiCommand.h - implements Commands, a layer between the user and widgets
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

// a safe function pointer for commands
class CommandFnPtr
{
public:
	template<typename DataType>
	void callFn(InteractiveModelView* object, DataType data)
	{
		callFnInternal(object, static_cast<void*>(&data), typeid(DataType).hash_code());
	}
	void callFnTypeless(InteractiveModelView* object)
	{
		callFnInternal(object, nullptr, 0);
	}
	size_t getTypeIdFromInteractiveModelView(InteractiveModelView* object);
	
	virtual CommandFnPtr* clone() const = 0;
	virtual bool isValid() const = 0;
private:
	virtual void callFnInternal(InteractiveModelView* object, void* data, size_t dataTypeId) = 0;
};

template<typename BaseType, typename DataType>
class TypedCommandFnPtr : public CommandFnPtr
{
public:
	typedef void (BaseType::*InternalFnPtr)(DataType);
	
	TypedCommandFnPtr(InternalFnPtr fnPtr) :
		m_functionPtr(fnPtr)
	{}
	TypedCommandFnPtr() :
		m_functionPtr(nullptr)
	{}

	CommandFnPtr* clone() const override
	{
		return new TypedCommandFnPtr<BaseType, DataType>(m_functionPtr);
	}
	bool isValid() const override { return m_functionPtr != nullptr; }
private:
	void callFnInternal(InteractiveModelView* object, void* data, size_t dataTypeId) override
	{
		if (m_functionPtr == nullptr || object == nullptr) { return; }
		assert(data != nullptr);
		// if this assert fails, `InteractiveModelView::getStoredTypeId()` is wrong type
		assert(typeid(BaseType).hash_code() == getTypeIdFromInteractiveModelView(object));
		// if this assert fails, you need to change your `DataType` template to the one this class was constructed with
		assert(typeid(DataType).hash_code() == dataTypeId);
		// calling the function ptr
		(static_cast<BaseType*>(object)->*m_functionPtr)(*static_cast<DataType*>(data));
	}
	InternalFnPtr m_functionPtr;
};

template<typename BaseType>
class BasicCommandFnPtr : public CommandFnPtr
{
public:
	typedef void (BaseType::*InternalFnPtr)();
	
	BasicCommandFnPtr(InternalFnPtr fnPtr) :
		m_functionPtr(fnPtr)
	{}
	BasicCommandFnPtr() :
		m_functionPtr(nullptr)
	{}

	CommandFnPtr* clone() const override
	{
		return new BasicCommandFnPtr<BaseType>(m_functionPtr);
	}
	bool isValid() const override { return m_functionPtr != nullptr; }
private:
	void callFnInternal(InteractiveModelView* object, void* data, size_t dataTypeId) override
	{
		if (m_functionPtr == nullptr || object == nullptr) { return; }
		// if this assert fails, `InteractiveModelView::getStoredTypeId()` is wrong type (function isn't member to type)
		assert(typeid(BaseType).hash_code() == getTypeIdFromInteractiveModelView(object));
		// calling the function ptr
		(static_cast<BaseType*>(object)->*m_functionPtr)();
	}
	InternalFnPtr m_functionPtr;
};


class AbstractGuiCommand// : public QUndoCommand
{
public:
	AbstractGuiCommand(const QString& name, InteractiveModelView* object, bool linkBack);
	~AbstractGuiCommand() {}
	//! should be undone along with the action before this
	bool getShouldLinkBack();
	//! returns true if cleared, use this to delete pointers to destructing objects
	bool clearObjectIfMatch(InteractiveModelView* object);
protected:
	const QString m_name;
	InteractiveModelView* m_target;
	bool m_isLinkedBack; //! if this is undone, undo the one before this
};

class GuiCommand : public AbstractGuiCommand
{
public:
	GuiCommand(const QString& name, InteractiveModelView* object, std::shared_ptr<CommandFnPtr> doFn, std::shared_ptr<CommandFnPtr> undoFn, size_t runAmount, bool linkBack);
	void undo();// override
	void redo();// override
private:
	size_t m_runAmount;
	std::shared_ptr<CommandFnPtr> m_doFn;
	std::shared_ptr<CommandFnPtr> m_undoFn;
};



template<typename DataType>
class GuiCommandTyped : public AbstractGuiCommand
{
public:
	GuiCommandTyped(const QString& name, InteractiveModelView* object, std::shared_ptr<CommandFnPtr> doFn, std::shared_ptr<CommandFnPtr> undoFn, DataType doData, DataType* undoData, bool linkBack) :
		AbstractGuiCommand(name, object, linkBack),
		m_doData(nullptr),
		m_undoData(nullptr),
		m_doFn(doFn),
		m_undoFn(undoFn)
	{
		m_doData = std::make_unique<DataType>(doData);
		m_undoData = undoData != nullptr ? std::make_unique<DataType>(*undoData) : nullptr;
	}
	void undo()// override
	{
		if (m_target == nullptr) { return; }
		if (m_undoFn.get() != nullptr && m_undoFn->isValid())
		{
			if (m_undoData.get() != nullptr)
			{
				m_undoFn->callFn<DataType>(m_target, *m_undoData.get());
			}
			else
			{
				m_undoFn->callFn<DataType>(m_target, *m_doData.get());
			}
		}
		else if (m_doFn->isValid() && m_undoData.get() != nullptr)
		{
			m_doFn->callFn<DataType>(m_target, *m_undoData.get());
		}
	}
	void redo()// override
	{
		if (m_target == nullptr || m_doFn->isValid() == false) { return; }
		m_doFn->callFn<DataType>(m_target, *m_doData.get());
	}
private:
	std::unique_ptr<DataType> m_doData;
	std::unique_ptr<DataType> m_undoData;
	std::shared_ptr<CommandFnPtr> m_doFn;
	std::shared_ptr<CommandFnPtr> m_undoFn;
};

} // namespace lmms::gui

#endif // LMMS_GUI_ACTION_H
