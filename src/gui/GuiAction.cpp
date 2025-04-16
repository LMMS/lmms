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

template<typename DataType>
GuiAction::GuiAction(const QString& name, InteractiveModelView* object, TypelessFn doFn, TypelessFn undoFn, size_t runAmount) :
	m_target(object),
	m_runAmount(runAmount),
	m_data(nullptr),
	m_doFn(doFn),
	m_undoFn(undoFn),
	m_doTypedFn(nullptr),
	m_undoTypedFn(nullptr)
{
}

template<typename DataType>
GuiAction::GuiAction(const QString& name, InteractiveModelView* object, ActionSafeFnPtr doFn, ActionSafeFnPtr undoFn, std::shared_ptr<DataType> data) :
	m_target(object),
	m_runAmount(0),
	m_data(data),
	m_doFn(nullptr),
	m_undoFn(nullptr),
	m_doTypedFn(doFn),
	m_undoTypedFn(undoFn)
{
}

template<typename DataType>
GuiAction::~GuiAction()
{
}

template<typename DataType>
void GuiAction::undo()
{
	if (m_target == nullptr) { return false; }
	if (m_undoFn != nullptr)
	{
		*m_undoFn(object);
	}
	else if (m_doTypedFn.isValid())
	{
		m_undoTypedFn.callFn<DataType>(m_target, m_data);
	}
}

template<typename DataType>
void GuiAction::redo()
{
	if (m_target == nullptr) { return false; }
	if (m_doFn != nullptr)
	{
		*m_doFn(object);
	}
	else if (m_doTypedFn.isValid())
	{
		m_doTypedFn.callFn<DataType>(m_target, m_data);
	}
}

template<typename DataType>
bool GuiAction::clearObjectIfMatch(InteractiveModelView* object)
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

} // template<typename DataType>
