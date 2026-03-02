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

#include <QObject>
#include <QDebug> // TODO: remove
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>

#include "GuiAction.h"

namespace lmms {

std::map<QString, ActionData*> ActionContainer::s_dataMap;

bool ActionContainer::tryRegister(QString name, ActionTrigger::Any trigger)
{
	auto it = s_dataMap.find(name);
	if (it != s_dataMap.end()) { return false; }
	s_dataMap[name] = new ActionData(name, trigger);
	return true;
}

ActionData* ActionContainer::findData(const QString& name)
{
	auto it = s_dataMap.find(name);
	return (it == s_dataMap.end()) ? nullptr : s_dataMap.at(name);
}

ActionData::ActionData(QString name, ActionTrigger::Any trigger)
	: QObject(nullptr)
	, m_name{name}
	, m_trigger{trigger}
{
}

ActionData* ActionData::get(const QString& name, ActionTrigger::Any trigger)
{
	ActionContainer::tryRegister(name, trigger);
	return ActionContainer::findData(name);
}

const ActionTrigger::Any& ActionData::trigger() const
{
	return m_trigger;
}

void ActionData::setTrigger(ActionTrigger::Any&& newTrigger)
{
	m_trigger = newTrigger;
	emit modified();
}

ActionTrigger::Any ActionTrigger::pressed(Qt::KeyboardModifiers mods, Qt::Key key, bool repeat)
{
	return ActionTrigger::KeyPressed{.mods = mods, .key = key, .repeat = repeat};
}

ActionTrigger::Any ActionTrigger::held(Qt::KeyboardModifiers mods, Qt::Key key) {
	return ActionTrigger::KeyHeld{.mods = mods, .key = key};
}

GuiAction::GuiAction(QObject* parent, ActionData* data)
	: QObject(parent)
	, m_data{data}
	, m_active{false}
{
	if (parent != nullptr) { parent->installEventFilter(this); }
	connect(data, &ActionData::modified, this, [this] { m_active = false; });
}

GuiAction::~GuiAction()
{
}

ActionContainer::MappingIterator ActionContainer::mappingsBegin()
{
	return s_dataMap.begin();
}

ActionContainer::MappingIterator ActionContainer::mappingsEnd()
{
	return s_dataMap.end();
}

bool GuiAction::eventFilter(QObject* watched, QEvent* event)
{
	const auto& trigger_g = m_data->trigger();
	if (std::holds_alternative<ActionTrigger::KeyPressed>(trigger_g))
	{
		const auto& trigger = std::get<ActionTrigger::KeyPressed>(trigger_g);
		if (event->type() == QEvent::KeyPress)
		{
			auto* ke = dynamic_cast<QKeyEvent*>(event);
			assert(ke != nullptr);

			// FIXME: "This function cannot always be trusted. The user can
			// confuse it by pressing both Shift keys simultaneously and
			// releasing one of them, for example." @
			// https://doc.qt.io/qt-6/qkeyevent.html#modifiers

			if (ke->key() == trigger.key && ke->modifiers() == trigger.mods
				&& !(ke->isAutoRepeat() && !trigger.repeat))
			{
				m_active = false;
				emit activated();
				return true;
			}
		}
	}
	else if (std::holds_alternative<ActionTrigger::KeyHeld>(trigger_g))
	{
		const auto& trigger = std::get<ActionTrigger::KeyHeld>(trigger_g);
		if (!m_active && event->type() == QEvent::KeyPress)
		{
			auto* ke = dynamic_cast<QKeyEvent*>(event);
			assert(ke != nullptr);

			if (ke->key() == trigger.key && ke->modifiers() == trigger.mods)
			{
				m_active = true;
				emit activated();
				return true;
			}
		}
		else if (m_active && event->type() == QEvent::KeyRelease)
		{
			auto* ke = dynamic_cast<QKeyEvent*>(event);
			assert(ke != nullptr);

			// Ignore auto-repeat releases
			if (ke->key() == trigger.key && !ke->isAutoRepeat())
			{
				m_active = false;
				emit deactivated();
				return true;
			}
		}
	}

	return QObject::eventFilter(watched, event);
}

void syncActionDataToQAction(ActionData* data, QAction* action)
{
	auto updateAction = [action, data]
	{
		const auto& trigger_g = data->trigger();
		if (std::holds_alternative<ActionTrigger::KeyPressed>(trigger_g))
		{
			const auto& trigger = std::get<ActionTrigger::KeyPressed>(trigger_g);
			action->setShortcut(QKeySequence{static_cast<int>(trigger.mods + trigger.key)});
			action->setAutoRepeat(trigger.repeat);
		}
		else
		{
			qWarning() << "Expected KeyPressed trigger! QAction will have no trigger keybinding.";
			action->setShortcut(QKeySequence{});
		}
	};

	updateAction();
	QObject::connect(data, &ActionData::modified, action, updateAction);
}

} // namespace lmms
