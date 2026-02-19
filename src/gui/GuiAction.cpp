/*
 * GuiAction.cpp - action listener for flexible keybindings
 *
 * This file is part of LMMS - https://lmms.io
 *
 * Copyright (c) 2026 yohannd1 <mitonanan12@gmail.com>
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

std::map<QString, ActionData> ActionContainer::s_dataMap;

bool ActionContainer::tryRegister(QString name, ActionData data)
{
	// std::map::insert only inserts an element if it isn't already present
	const auto [_, success] = s_dataMap.insert({name, data});
	return success;
}

ActionData* ActionContainer::findData(const QString& name)
{
	auto it = s_dataMap.find(name);
	return (it == s_dataMap.end()) ? nullptr : &s_dataMap.at(name);
}

ActionData::ActionData(QString name, ActionTrigger::Any trigger)
	: m_name{name}
	, m_trigger{trigger}
{}

ActionData* ActionData::getOrCreate(QString name, ActionTrigger::Any trigger)
{
	ActionContainer::tryRegister(name, ActionData(name, trigger));
	return ActionContainer::findData(name);
}

ActionTrigger::Any ActionTrigger::pressed(uint32_t mods, uint32_t key)
{
	return ActionTrigger::KeyPressed { .mods = mods, .key = key };
}

ActionTrigger::Any ActionTrigger::held(uint32_t mods, uint32_t key)
{
	return ActionTrigger::KeyHeld { .mods = mods, .key = key };
}

GuiAction::GuiAction(QObject* parent, ActionData* data)
	: QObject(parent)
	, m_data{data}
	, m_active{false}
	, m_onActivateFunc{nullptr}
	, m_onDeactivateFunc{nullptr}
{
	if (parent != nullptr)
	{
		parent->installEventFilter(this);
		// TODO: how to detect when parent has changed?
	}

	// TODO: how to signal `GuiAction` when the ActionData has had its trigger changed? Does it even need that?
}

GuiAction::~GuiAction() {}

void GuiAction::setOnActivate(std::function<void (QObject*)> func)
{
	m_onActivateFunc = func;
	m_active = false; // TODO: confirm if this is alright
}

void GuiAction::setOnDeactivate(std::function<void (QObject*)> func)
{
	m_onDeactivateFunc = func;
	m_active = false; // TODO: confirm if this is alright
}

bool GuiAction::eventFilter(QObject* watched, QEvent* event)
{
	return false; // TODO
}

} // namespace lmms
