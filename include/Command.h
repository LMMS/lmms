/*
 * Command.h - implements Commands, a layer between the core and gui
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

/* Command:
 * performs instructions on core or gui data
 * ANY gui element should be able to trigger it
 * links gui to core
 * gui should only use commands on core
*/

#ifndef LMMS_COMMAND
#define LMMS_COMMAND

#include <cassert> // assert

#include "CommandStack.h"
#include "lmms_export.h"

namespace lmms
{

class LMMS_EXPORT CommandDataBase
{
public:
	virtual ~CommandDataBase() {};
};

template<typename T>
class LMMS_EXPORT CommandData : public CommandDataBase
{
public:
	CommandData(T executeData)
		: m_currentState{executeData}
		, m_undoState{}
	{}
	~CommandData() override {};
	//! gets the data that should be used to execute a command
	T& getExecuteState() { return m_currentState; }
	//! gets the data that should be used to undo a command
	T& getUndoState() { return m_undoState; }
private:
	// this can be optimized by using 1 value and swapping between current and old state
	// execute(val) -> val = old_state -> undo(val) -> val = current_state
	T m_currentState;
	T m_undoState;
};

/*
	Here is Command inheritance:

	CommandBase -----> TypelessCommand -> CommandLambda
	            |
	            |----> CommandFnPtr
	            \
	             ----> ParamCommand ----> ParamCommandLambda
	                               \
	                                ----> ParamCommandFnPtr
	Use these functions / methods:
	1. push(), operator() -> pushes a command onto a stack
	2. push(param), operator(param) -> pushes a command onto a stack with a parameter
*/


class LMMS_EXPORT CommandBase
{
public:
	CommandBase(CommandStack& container)
		: m_container{&container}
	{}
	virtual ~CommandBase()
	{
		m_container->remove(*this);
	}

	//! executes the command (without pushing it to the undo stack)
	virtual void executeCommand() const {}
	//! undoes the command (without pushing it to the undo stack)
	virtual void undoCommand() const {}
	virtual void executeCommand(CommandDataBase& data) const {}
	virtual void undoCommand(CommandDataBase& data) const {}
	
protected:
	CommandStack* m_container;
};





//! use this command if you need to construct a `CommandLambda`
class LMMS_EXPORT TypelessCommand : public CommandBase
{
public:
	TypelessCommand(CommandStack& container)
		: CommandBase{container} {}
	//! pushes the command onto the stack and executes it
	void push() const { m_container->pushBack(*this, nullptr); }
	void operator()() const { push(); }
};






//! use this command if you need to use a lambda (init `TypelessCommand*` with this)
template<typename DoFn, typename UndoFn>
class LMMS_EXPORT CommandLambda : public TypelessCommand
{
public:
	//! @param doFn lambda that does the action
	//! @undoFn doFn lambda that does the action
	CommandLambda(CommandStack& container, DoFn doLambda, UndoFn undoLambda)
		: TypelessCommand{container}
		, m_doLambda(doLambda)
		, m_undoLambda(undoLambda)
	{}
	~CommandLambda() override {};
	void executeCommand() const override { m_doLambda(); }
	void undoCommand() const override { m_undoLambda(); }
private:
	DoFn m_doLambda;
	UndoFn m_undoLambda;
};






//! use this command if "do" and "undo" functions are already available
template<typename Parent, typename returnTDo, typename returnTUndo>
class LMMS_EXPORT CommandFnPtr : public CommandBase
{
public:
	typedef returnTDo (Parent::*DoFnPtr)();
	typedef returnTUndo (Parent::*UndoFnPtr)();

	CommandFnPtr(CommandStack& container, Parent& parentInstance, DoFnPtr doFn, UndoFnPtr undoFn)
		: CommandBase{container}
		, m_doFn(doFn)
		, m_undoFn(undoFn)
		, m_parent(&parentInstance)
	{}
	~CommandFnPtr() override {};
	
	//! pushes the command onto the stack and executes it
	void push() const { m_container->pushBack(*this, nullptr); }
	void operator()() const { push(); }

	//! executes the command without pushing it to the undo stack
	void executeCommand() const override { (m_parent->*m_doFn)(); }
	void undoCommand() const override { (m_parent->*m_undoFn)(); }
private:
	DoFnPtr m_doFn;
	UndoFnPtr m_undoFn;
	Parent* m_parent;
};








//! use this command if you need to construct a `ParamCommandLambda`, this command has inputs
template<typename T>
class LMMS_EXPORT ParamCommand : public CommandBase //< command with param
{
public:
	ParamCommand(CommandStack& container)
		: CommandBase{container} {}
	//! pushes the command onto the stack and executes it
	void push(T data) const { m_container->pushBack(*this, static_cast<CommandDataBase*>(new CommandData<T>{data})); }
	void operator()(T data) const { push(data); }
};






//! use this command if you need to use a lambda (init `ParamCommand*` with this), this command has inputs
template<typename T, typename DoFn, typename UndoFn>
class LMMS_EXPORT ParamCommandLambda : public ParamCommand<T>
{
public:
	//! @param unused pass this in so the type can be deducted
	ParamCommandLambda(CommandStack& container, DoFn doLambda, UndoFn undoLambda, T unused)
		: ParamCommand<T>{container}
		, m_doLambda(doLambda)
		, m_undoLambda(undoLambda)
	{}
	~ParamCommandLambda() override {};

	void executeCommand(CommandDataBase& data) const override
	{
		auto* castedData = dynamic_cast<CommandData<T>*>(&data);
		if (castedData != nullptr)
		{
			//! `m_doLambda` gets T& do data and T& undo data, it must set the undo data
			m_doLambda(castedData->getExecuteState(), castedData->getUndoState());
		}
	}
	void undoCommand(CommandDataBase& data) const override
	{
		auto* castedData = dynamic_cast<CommandData<T>*>(&data);
		if (castedData != nullptr)
		{
			//! `m_undoLambda` gets T& do data and T& undo data
			m_undoLambda(castedData->getExecuteState(), castedData->getUndoState());
		}
	}
private:
	DoFn m_doLambda;
	UndoFn m_undoLambda;
};






//! use this command if "getter" and "setter" functions are already available, this command has inputs
template<typename T, typename Parent, typename returnT>
class LMMS_EXPORT ParamCommandFnPtr : public ParamCommand<T>
{
public:
	typedef T (Parent::*GetterFn)();
	typedef returnT (Parent::*SetterFn)(T);

	ParamCommandFnPtr(CommandStack& container, Parent& parentInstance, GetterFn getFnPtr, SetterFn setFnPtr, const T* paramPtr)
		: ParamCommand<T>{container}
		, m_getFnPtr{getFnPtr}
		, m_setFnPtr{setFnPtr}
		, m_parent{&parentInstance}
		, m_paramPtr(paramPtr)
	{
		assert((m_paramPtr == nullptr && m_getFnPtr != nullptr)
			|| (m_paramPtr != nullptr && m_getFnPtr == nullptr));
	}
	~ParamCommandFnPtr() override {};

	void executeCommand(CommandDataBase& data) const override
	{
		auto* castedData = dynamic_cast<CommandData<T>*>(&data);
		if (castedData != nullptr)
		{
			if (m_getFnPtr != nullptr)
			{
				castedData->getUndoState() = (m_parent->*m_getFnPtr)();
			}
			else
			{
				castedData->getUndoState() = *m_paramPtr;
			}
			
			(m_parent->*m_setFnPtr)(castedData->getExecuteState());
		}
	}
	void undoCommand(CommandDataBase& data) const override
	{
		auto* castedData = dynamic_cast<CommandData<T>*>(&data);
		if (castedData != nullptr)
		{
			(m_parent->*m_setFnPtr)(castedData->getUndoState());
		}
	}
private:
	GetterFn m_getFnPtr;
	SetterFn m_setFnPtr;
	Parent* m_parent;
	const T* m_paramPtr;
};

} // namespace lmms

#endif // LMMS_COMMAND

