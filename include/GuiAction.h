/*
 * GuiAction.h - declaration of class GuiAction (and related ones)
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

#ifndef LMMS_GUI_ACTION_H
#define LMMS_GUI_ACTION_H

#include <QString>
#include <QObject>

#include <variant>
#include <functional>

namespace lmms {

class ActionData;

class ActionTrigger
{
public:
	// FIXME: For key events, the enums Qt::KeyboardModifier and Qt::Key are used. I can't find them so I'm using
	// uint32_t...

	struct Never {}; //!< Can never be triggered
	struct KeyPressed
	{
		uint32_t mods, key;
		bool repeat;
	};
	struct KeyHeld { uint32_t mods, key; };

	//! Top type for all possible triggers
	typedef std::variant<Never, KeyPressed, KeyHeld> Any;

	static Any pressed(uint32_t mods, uint32_t key, bool repeat = true);
	static Any held(uint32_t mods, uint32_t key);
};

class ActionContainer
{
public:
	//! Attempts to register a new action, but refuses if it is already registered. Returns whether the insertion
	//! happened.
	static bool tryRegister(QString name, ActionData data);

	//! Find an action by its name. Returns null when it was not found.
	static ActionData* findData(const QString& name);

private:
	ActionContainer() = delete;
	~ActionContainer() = delete;

	static std::map<QString, ActionData> s_dataMap;
};

class ActionData
{
public:
	//! For now, to avoid memory safety issues, actions are never removed.
	static ActionData* getOrCreate(QString name, ActionTrigger::Any trigger = ActionTrigger::Never{});

	// FIXME: not sure if the lifetime of this returned pointer ^ is all that good either... maybe use a
	// std::shared_ptr? At the cost of fragmentation

	const QString& name() const;
	const ActionTrigger::Any& trigger() const;
	void setTrigger(ActionTrigger::Any&& newTrigger);

private:
	ActionData(QString name, ActionTrigger::Any trigger);

	QString m_name;
	ActionTrigger::Any m_trigger;
};

// TODO: think of a better name. `ActionListener` or `CommandListener` might be good?
class GuiAction : QObject
{
public:
	GuiAction(QObject* parent, ActionData* data);
	~GuiAction();

	void setOnActivate(std::function<void (QObject*)> func);
	void setOnDeactivate(std::function<void (QObject*)> func);

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	ActionData* m_data;
	bool m_active;
	uint32_t m_mods;

	std::function<void (QObject*)> m_onActivateFunc;
	std::function<void (QObject*)> m_onDeactivateFunc;
};

} // namespace lmms

#endif // LMMS_GUI_ACTION_H
