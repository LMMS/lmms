/*
 * CommandStack.h - provides undo and redo for commands
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

#include "CommandStack.h"

#include "Command.h"

namespace lmms
{

CommandStack::CommandStack()
	: m_undoSize{0}
	, m_stack()
{}

CommandStack::~CommandStack()
{
	while (m_stack.size() > 0)
	{
		popBack();
	}
}

void CommandStack::pushBack(const CommandBase& command, CommandDataBase* commandData)
{
	size_t diff{m_stack.size() - m_undoSize};
	for (size_t i{0}; i < diff; ++i)
	{
		popBack();
	}

	m_stack.push_back(std::make_pair(&command, commandData));
	redo();
}

void CommandStack::undo()
{
	if (m_undoSize <= 0) { return; }

	--m_undoSize;
	if (m_stack[m_undoSize].second != nullptr)
	{
		m_stack[m_undoSize].first->undoCommand(*m_stack[m_undoSize].second);
	}
	else
	{
		m_stack[m_undoSize].first->undoCommand();
	}
}

void CommandStack::redo()
{
	if (m_undoSize >= m_stack.size()) { return; }

	if (m_stack[m_undoSize].second != nullptr)
	{
		m_stack[m_undoSize].first->executeCommand(*m_stack[m_undoSize].second);
	}
	else
	{
		m_stack[m_undoSize].first->executeCommand();
	}

	++m_undoSize;
}

void CommandStack::remove(const CommandBase& command)
{
	size_t foundIndex{0};
	bool found{false};
	for (size_t i{0}; i < m_stack.size(); ++i)
	{
		if (m_stack[i].first == &command)
		{
			foundIndex = i;
			found = true;
		}
	}
	if (found == false) { return; }
	if (foundIndex < m_undoSize) { --m_undoSize; }

	// could be optimized by deleting [foundIndex].second and just copying instead of swaping
	for (size_t i{foundIndex}; i + 1 < m_stack.size(); ++i)
	{
		auto swap{m_stack[foundIndex]};
		m_stack[foundIndex] = m_stack[foundIndex + 1];
		m_stack[foundIndex + 1] = swap;
	}
	popBack();
}

void CommandStack::popBack()
{
	delete m_stack[m_stack.size() - 1].second;
	m_stack.pop_back();
}

} // namespace lmms
