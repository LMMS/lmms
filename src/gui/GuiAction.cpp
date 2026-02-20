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

#include <QDebug>
#include <QEvent>
#include <QKeyEvent>

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
{
}

ActionData* ActionData::getOrCreate(QString name, ActionTrigger::Any trigger)
{
	ActionContainer::tryRegister(name, ActionData(name, trigger));
	return ActionContainer::findData(name);
}

const ActionTrigger::Any& ActionData::trigger() const
{
	return m_trigger;
}

void ActionData::setTrigger(ActionTrigger::Any&& newTrigger)
{
	m_trigger = newTrigger;
}

ActionTrigger::Any ActionTrigger::pressed(uint32_t mods, uint32_t key, bool repeat)
{
	return ActionTrigger::KeyPressed{.mods = mods, .key = key, .repeat = repeat};
}

ActionTrigger::Any ActionTrigger::held(uint32_t mods, uint32_t key)
{
	return ActionTrigger::KeyHeld{.mods = mods, .key = key};
}

GuiAction::GuiAction(QObject* parent, ActionData* data)
	: QObject(parent)
	, m_data{data}
	, m_active{false}
	, m_onActivateFunc{[](QObject*){}}
	, m_onDeactivateFunc{[](QObject*){}}
{
	if (parent != nullptr)
	{
		parent->installEventFilter(this);
		// TODO: how to detect when parent has changed?
	}

	// TODO: how to signal `GuiAction` when the ActionData has had its trigger changed? Does it even need that?
}

GuiAction::~GuiAction()
{
}

void GuiAction::setOnActivateObj(std::function<void(QObject*)> func)
{
	m_onActivateFunc = func;
	m_active = false; // TODO: confirm if this is alright
}

void GuiAction::setOnDeactivateObj(std::function<void(QObject*)> func)
{
	m_onDeactivateFunc = func;
	m_active = false; // TODO: confirm if this is alright
}

bool GuiAction::eventFilter(QObject* watched, QEvent* event)
{
	const auto& trigger_g = m_data->trigger();
	if (std::holds_alternative<ActionTrigger::KeyPressed>(trigger_g))
	{
		const auto& trigger = std::get<ActionTrigger::KeyPressed>(trigger_g);
		if (!m_active && event->type() == QEvent::KeyPress)
		{
			auto* ke = dynamic_cast<QKeyEvent*>(event);
			assert(ke != nullptr);

			// FIXME: "This function cannot always be trusted. The user can confuse it by pressing both Shift keys
			// simultaneously and releasing one of them, for example." @ https://doc.qt.io/qt-6/qkeyevent.html#modifiers

			if ((uint32_t)ke->key() == trigger.key && ke->modifiers() == trigger.mods)
			{
				m_onActivateFunc(watched);
				m_active = false;
				return true;
			}
		}
		// else if (m_active && event->type() == QEvent::KeyRelease)
		// {
		// 	auto* ke = dynamic_cast<QKeyEvent*>(event);
		// 	assert(ke != nullptr);
		// 	return true;
		// }
	}

	return QObject::eventFilter(watched, event);
}

} // namespace lmms
