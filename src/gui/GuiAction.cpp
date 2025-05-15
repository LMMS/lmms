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

#include <cassert>

#include "GuiAction.h"

namespace lmms::gui
{

AbstractGuiAction::AbstractGuiAction(const QString& name, InteractiveModelView* object, bool linkBack) :
	m_name(name),
	m_target(object),
	m_isLinkedBack(linkBack)
{
	assert(m_target != nullptr);
}
bool AbstractGuiAction::getShouldLinkBack()
{
	return m_isLinkedBack;
}
bool AbstractGuiAction::clearObjectIfMatch(InteractiveModelView* object)
{
	if (m_target == nullptr) { return false; }
	if (object == m_target)
	{
		m_target = nullptr;
		return true;
	}
	return false;
}
GuiAction::GuiAction(const QString& name, InteractiveModelView* object, CommandFnPtr doFn, CommandFnPtr undoFn, size_t runAmount, bool linkBack) :
	AbstractGuiAction(name, object, linkBack),
	m_runAmount(runAmount),
	m_doFn(doFn),
	m_undoFn(undoFn)
{}
void GuiAction::undo()
{
	if (m_target == nullptr || m_undoFn == nullptr) { return; }
	for (size_t i = 0; i < m_runAmount; i++)
	{
		m_undoFn.callFnTypeless(m_target);
	}
}
void GuiAction::redo()
{
	if (m_target == nullptr || m_doFn == nullptr) { return; }
	for (size_t i = 0; i < m_runAmount; i++)
	{
		m_doFn.callFnTypeless(m_target);
	}
}

} // template<typename DataType>
