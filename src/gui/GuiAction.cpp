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

#include <QAction>

#include "GuiAction.h"
#include "InteractiveModelView.h"

namespace lmms::gui
{

GuiActionIO::GuiActionIO()
{}

GuiActionIO::GuiActionIO(const GuiActionIO& ref) :
	m_typeId(ref.m_typeId),
	m_valueA(ref.m_valueA),
	m_valueB(ref.m_valueB),
	m_valueC(ref.m_valueC),
	m_valueD(ref.m_valueD),
	m_valueE(ref.m_valueE),
	m_valueF(ref.m_valueF)
{}

GuiActionIO::GuiActionIO(const size_t& value) :
	m_typeId(GuiActionIO::getTypeId<size_t>()),
	m_valueA(value)
{}

GuiActionIO::GuiActionIO(const int& value) :
	m_typeId(GuiActionIO::getTypeId<int>()),
	m_valueB(value)
{}

GuiActionIO::GuiActionIO(const float& value) :
	m_typeId(GuiActionIO::getTypeId<float>()),
	m_valueC(value)
{}

GuiActionIO::GuiActionIO(const std::pair<float, float>& value) :
	m_typeId(GuiActionIO::getTypeId<std::pair<float, float>>()),
	m_valueD(value)
{}

GuiActionIO::GuiActionIO(const std::vector<float>& value) :
	m_typeId(GuiActionIO::getTypeId<std::vector<float>>()),
	m_valueE(value)
{}

GuiActionIO::GuiActionIO(bool* value) :
	m_typeId(GuiActionIO::getTypeId<bool*>()),
	m_valueF(value)
{}

GuiAction::GuiAction(ActionStruct& parentAction, const GuiActionIO& data, size_t runAmount, bool linkBack) :
	m_name(""),
	m_isLinkedBack(linkBack),
	m_runAmount(runAmount),
	m_data(data),
	m_parentAction(&parentAction)
{
	m_name = m_parentAction->getText();
}
void GuiAction::undo()
{
	if (m_parentAction == nullptr || m_parentAction->undoFn == nullptr) { return; }
	m_parentAction->setData(m_data);
	for (size_t i = 0; i < m_runAmount; i++)
	{
		m_parentAction->undoFn->trigger();
	}
}
void GuiAction::redo()
{
	if (m_parentAction == nullptr || m_parentAction->doFn == nullptr) { return; }
	m_parentAction->setData(m_data);
	for (size_t i = 0; i < m_runAmount; i++)
	{
		m_parentAction->doFn->trigger();
	}
}

bool GuiAction::getShouldLinkBack()
{
	return m_isLinkedBack;
}

bool GuiAction::clearActionIfMatch(ActionStruct* action)
{
	if (action != m_parentAction) { return false; }

	m_parentAction = nullptr;
	return true;
}

} // template<typename DataType>
