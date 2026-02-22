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

#include <QObject>
#include <QString>
#include <QDebug>

#include <functional>
#include <variant>

namespace lmms {

class ActionData;

class ActionTrigger
{
public:
	struct Never //!< Can never be triggered
	{
	};

	struct KeyPressed
	{
		Qt::KeyboardModifiers mods;
		Qt::Key key;
		bool repeat;
	};

	struct KeyHeld
	{
		Qt::KeyboardModifiers mods;
		Qt::Key key;
	};

	//! Top type for all possible triggers
	typedef std::variant<Never, KeyPressed, KeyHeld> Any;

	static Any pressed(Qt::KeyboardModifiers mods, Qt::Key key, bool repeat = true);
	static Any held(Qt::KeyboardModifiers mods, Qt::Key key);
};

class ActionContainer
{
public:
	//! Attempts to register a new action, but refuses if it is already registered. Returns whether the insertion
	//! happened.
	static bool tryRegister(QString name, ActionTrigger::Any trigger);

	//! Find an action by its name. Returns null when it was not found.
	static ActionData* findData(const QString& name);

	using MappingIterator = std::map<QString, ActionData*>::iterator;
	static MappingIterator mappingsBegin();
	static MappingIterator mappingsEnd();

private:
	ActionContainer() = delete;
	~ActionContainer() = delete;

	//! Map with all known actions (owned by this).
	static std::map<QString, ActionData*> s_dataMap;
};

class ActionData : public QObject
{
	Q_OBJECT

public:
	//! For now, to avoid memory safety issues, ActionData instances are never removed or freed.
	static ActionData* getOrCreate(QString name, ActionTrigger::Any trigger = ActionTrigger::Never{});

	const QString& name() const;
	const ActionTrigger::Any& trigger() const;
	void setTrigger(ActionTrigger::Any&& newTrigger);

	friend class ActionContainer;

signals:
	void modified();

private:
	ActionData(QString name, ActionTrigger::Any trigger);

	QString m_name;
	ActionTrigger::Any m_trigger;
};

/**
	* Do not change the parent of this object! (FIXME: implement this, perhaps)
	*
	* TODO: think of a better name. `ActionListener` or `CommandListener` might be good?
*/
class GuiAction : public QObject
{
	Q_OBJECT

public:
	GuiAction(QObject* parent, ActionData* data);
	~GuiAction();

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

signals:
	void activated();
	void deactivated();

private:
	ActionData* m_data;
	bool m_active;
	uint32_t m_mods;
};

} // namespace lmms

#endif // LMMS_GUI_ACTION_H
