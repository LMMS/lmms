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

#ifndef LMMS_COMMAND_STACK
#define LMMS_COMMAND_STACK

#include <utility>
#include <vector>

#include "lmms_export.h"

namespace lmms
{

class CommandBase;
class CommandDataBase;

class LMMS_EXPORT CommandStack
{
public:
	CommandStack();
	~CommandStack();

	void pushBack(const CommandBase& command, CommandDataBase* commandData);
	void undo();
	void redo();

	void remove(const CommandBase& command);
private:
	void popBack();
	size_t m_undoSize;
	//! stores the used commands (not nullptr) among with the changed data (that could be nullptr)
	std::vector<std::pair<const CommandBase*, CommandDataBase*>> m_stack;
};

} // namespace lmms

#endif // LMMS_COMMAND_STACK
