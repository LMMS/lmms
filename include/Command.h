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

#include <stdio.h> // debug

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
	T& getExecuteState() { return m_currentState; }
	T& getUndoState() { return m_undoState; }
private:
	// this can be optimized by using 1 value and swapping between current and old state
	// execute(val) -> val = old_state -> undo(val) -> val = current_state
	T m_currentState;
	T m_undoState;
};

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

	virtual void executeCommand() const { printf("debug CommandBase execA\n"); }
	virtual void undoCommand() const {}
	virtual void executeCommand(CommandDataBase& data) const = 0;// { printf("debug CommandBase execB\n"); }
	virtual void undoCommand(CommandDataBase& data) const {}
	
protected:
	CommandStack* m_container;
};

template<typename DoFn, typename UndoFn>
class LMMS_EXPORT Command : public CommandBase
{
public:
	Command(CommandStack& container, DoFn doFn, UndoFn undoFn)
		: CommandBase{container}
		, m_doFn(doFn)
		, m_undoFn(undoFn)
	{}
	~Command() override {};
	
	//! pushes the command onto the stack and executes it
	void push() const
	{
		m_container->pushBack(*this, nullptr);
	}
	void operator()() const { push(); }

	//! executes the command without pushing it to the undo stack
	void executeCommand() const override
	{
		printf("debug Command exec\n");
		m_doFn();
	}
	void undoCommand() const override
	{
		m_undoFn();
	}
	void executeCommand(CommandDataBase& data) const {};
private:
	DoFn m_doFn;
	UndoFn m_undoFn;
};


template<typename T>
class LMMS_EXPORT ParamCommand : public CommandBase //< command with param
{
public:
	ParamCommand(CommandStack& container)
		: CommandBase{container}
	{}

	//! pushes the command onto the stack and executes it
	void push(T data) const
	{
		m_container->pushBack(*this, static_cast<CommandDataBase*>(new CommandData<T>{data}));
	}
	void operator()(T data) const { push(data); }
};

template<typename T, typename DoFn, typename UndoFn>
class LMMS_EXPORT ParamCommandLambda : public ParamCommand<T>
{
public:
	ParamCommandLambda(CommandStack& container, DoFn doFn, UndoFn undoFn, T* paramPtr)
		: ParamCommand<T>{container}
		, m_doFn(doFn)
		, m_undoFn(undoFn)
		, m_paramPtr(paramPtr)
	{}
	~ParamCommandLambda() override {};

	//! executes the command without pushing it to the undo stack
	void executeCommand(CommandDataBase& data) const override
	{
		printf("debug ParamCommandLambda exec\n");
		auto* castedData = dynamic_cast<CommandData<T>*>(&data);
		if (castedData != nullptr)
		{
			if (m_paramPtr != nullptr)
			{
				castedData->getUndoState() = *m_paramPtr;
				//m_doFn(castedData->getExecuteState());
			}
			else
			{
				m_doFn(castedData->getExecuteState(), castedData->getUndoState());
			}
		}
	}
	void undoCommand(CommandDataBase& data) const override
	{
		auto* castedData = dynamic_cast<CommandData<T>*>(&data);
		if (castedData != nullptr)
		{
			if (m_paramPtr != nullptr)
			{
				//m_undoFn(castedData->getUndoState());
			}
			else
			{
				m_undoFn(castedData->getExecuteState(), castedData->getUndoState());
			}
		}
	}
private:
	DoFn m_doFn;
	UndoFn m_undoFn;
	const T* m_paramPtr;
};

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

	//! executes the command without pushing it to the undo stack
	void executeCommand(CommandDataBase& data) const override
	{
		printf("debug ParamCommandFnPtr exec\n");
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

