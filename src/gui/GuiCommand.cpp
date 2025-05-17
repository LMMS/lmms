/*
 * GuiCommand.h - implements Actions, a layer between the user and widgets
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

#include <cassert>

#include "GuiCommand.h"
#include "InteractiveModelView.h"

namespace lmms::gui
{

size_t CommandFnPtr::getTypeIdFromInteractiveModelView(InteractiveModelView* object)
{
	return object->getStoredTypeId();
}

AbstractGuiCommand::AbstractGuiCommand(const QString& name, InteractiveModelView* object, bool linkBack) :
	m_name(name),
	m_target(object),
	m_isLinkedBack(linkBack)
{
	assert(m_target != nullptr);
}
bool AbstractGuiCommand::getShouldLinkBack()
{
	return m_isLinkedBack;
}
bool AbstractGuiCommand::clearObjectIfMatch(InteractiveModelView* object)
{
	if (m_target == nullptr) { return false; }
	if (object == m_target)
	{
		m_target = nullptr;
		return true;
	}
	return false;
}

GuiCommand::GuiCommand(const QString& name, InteractiveModelView* object, std::shared_ptr<CommandFnPtr> doFn, std::shared_ptr<CommandFnPtr> undoFn, size_t runAmount, bool linkBack) :
	AbstractGuiCommand(name, object, linkBack),
	m_runAmount(runAmount),
	m_doFn(doFn),
	m_undoFn(undoFn)
{}
void GuiCommand::undo()
{
	if (m_target == nullptr || m_undoFn.get() == nullptr) { return; }
	for (size_t i = 0; i < m_runAmount; i++)
	{
		m_undoFn->callFnTypeless(m_target);
	}
}
void GuiCommand::redo()
{
	if (m_target == nullptr || m_doFn.get()== nullptr) { return; }
	for (size_t i = 0; i < m_runAmount; i++)
	{
		m_doFn->callFnTypeless(m_target);
	}
}

} // namespace lmms::gui
