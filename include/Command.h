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

#include "CommandStack.h"

#include "lmms_export.h"

namespace lmms
{

class LMMS_EXPORT CommandDataBase
{
public:
	virtual ~CommandDataBase() = 0;
};

template<typename T>
class LMMS_EXPORT CommandData : public CommandDataBase
{
public:
	CommandData(T executeData)
		: m_currentState{executeData}
		, m_undoState{}
	{}
	~CommandData() override = default;
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

	virtual void executeCommand() const {};
	virtual void undoCommand() const {};
	virtual void executeCommand(CommandDataBase& data) const {};
	virtual void undoCommand(CommandDataBase& data) const {};
	
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
	~Command() override = default;
	
	//! pushes the command onto the stack and executes it
	void push() const
	{
		m_container->pushBack(static_cast<CommandBase>(*this), nullptr);
	}
	void operator()() const { push(); }

	//! executes the command without pushing it to the undo stack
	void executeCommand() const override
	{
		m_doFn();
	}
	void undoCommand() const override
	{
		m_undoFn();
	}
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
		m_container->pushBack(static_cast<CommandBase>(*this), static_cast<CommandDataBase*>(new CommandData<T>{data}));
	}
	void operator()(T data) const { push(data); }
};

template<typename T, typename DoFn, typename UndoFn>
class LMMS_EXPORT ParamCommandLambda : public ParamCommand<T>
{
public:
	ParamCommandLambda(CommandStack& container, DoFn doFn, UndoFn undoFn, T unused)
		: ParamCommand<T>{container}
		, m_doFn(doFn)
		, m_undoFn(undoFn)
	{}
	~ParamCommandLambda() override = default;

	//! executes the command without pushing it to the undo stack
	void execute(CommandDataBase& data)
	{
		T* castedData = dynamic_cast<CommandData<T>*>(&data);
		if (castedData != nullptr)
		{
			m_doFn(castedData->getExecuteState(), castedData->getUndoState());
		}
	}
	void undo(CommandDataBase& data)
	{
		T* castedData = dynamic_cast<CommandData<T>*>(&data);
		if (castedData != nullptr)
		{
			m_undoFn(castedData->getUndoState());
		}
	}
private:
	DoFn m_doFn;
	UndoFn m_undoFn;
};

} // namespace lmms

#endif // LMMS_COMMAND

