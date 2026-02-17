/*
 * GuiAction.h - declaration of class GuiAction (and related ones)
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_ACTION_H
#define LMMS_GUI_ACTION_H

#include <variant>
#include <QString>
#include <QObject>

namespace lmms {

class ActionData;

class ActionContainer
{
public:
	//! Register a new action. Overrides action with the same name.
	static void getOrCreate(QString name, ActionTrigger::Top trigger);

	//! Find an action by its name. Returns null when it was not found.
	static ActionData* findData(const char* name);

private:
	ActionContainer() = delete;
	~ActionContainer() = delete;

	static std::map<QString, ActionData> s_dataMap;
};

class ActionTrigger
{
public:
	struct Never {}; //!< Can never be triggered
	struct KeyPressed { QKey key; };
	struct KeyHeld { QKey upKey, downKey; };

	//! Top type for all possible triggers
	typedef std::variant<Never, KeyPress, KeyHeld> Any;
};

class ActionData
{
public:
	//! For now, to avoid memory safety issues, actions are never removed.
	static ActionData* create(QString name, ActionTrigger::Top trigger = ActionTrigger::Never{});
	// FIXME: not sure if the lifetime of this returned pointer ^ is all that good either... maybe use a
	// std::shared_ptr? At the cost of fragmentation

	const QString& name();
	const ActionTrigger::Top& trigger();
	void setTrigger(ActionTrigger::Top newTrigger);

private:
	ActionData(QString name, ActionTrigger::Top trigger);

	QString m_name;
	ActionTrigger::Top m_trigger;
};

// TODO: think of a better name. `ActionListener` or `CommandListener` might be good?
class GuiAction : QObject
{
public:
	GuiAction(QObject* parent, ActionData* data);

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	ActionData* m_data;
	bool m_active;

	std::function<QObject*> m_onActivateFunc;
	std::function<QObject*> m_onDeactivateFunc;
};

} // namespace lmms

#endif // LMMS_GUI_ACTION_H
