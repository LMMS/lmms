/*
 * GuiAction.cpp - action listener for .. ?? TODO
 *
 * Copyright (c) 2014 Lukas W <lukaswhl/at/gmail.com>
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

#include "GuiAction.h"

namespace lmms {

static void ActionContainer::registerData(ActionData data)
{
	// TODO
}

static ActionData* ActionContainer::findData(const char* name)
{
	// TODO
}

ActionData::ActionData(QString name, ActionTrigger::Top trigger)
	: m_name{name}
	, m_trigger{trigger}
{
}

static ActionData* ActionData::getOrCreate(QString name, ActionTrigger::Top trigger)
{
	if (auto it = s_dataMap.find(name); it != s_dataMap.end())
	{
		return &*it;
	}
	else
	{
		s_dataMap[name] = ActionData(name, trigger);
		return &s_dataMap[name];
	}
}

static ActionData* findData(const char* name)
{
	auto it = s_dataMap.find(name);
	return (it == s_dataMap.end()) ? nullptr : &*it;
}

GuiAction::GuiAction(QObject* parent, ActionData* data)
	: QObject(parent)
	, m_data{data}
	, m_active{false}
{
	if (parent != nullptr)
	{
		parent->installEventFilter(this);
		// TODO: how to detect when parent has changed?
	}

	// TODO: how to signal `GuiAction` when the ActionData has had its trigger changed? Does it even need that?
}

} // namespace lmms
