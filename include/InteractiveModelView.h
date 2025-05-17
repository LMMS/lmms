/*
 * InteractiveModelView.h - Implements command system for widgets
 *
 * Copyright (c) 2024 -2025 szeli1 <TODO/at/gmail/dot/com>
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

#ifndef LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
#define LMMS_GUI_INTERACTIVE_MODEL_VIEW_H

#include <list>
#include <memory>
#include <vector>

#include <QAction>
#include <QApplication>
#include <QWidget>
#include <QColor>

#include "Clipboard.h"
#include "GuiCommand.h"
#include "lmms_export.h"

class QColor;
class QMimeData;
class QPainter;
class QTimer;

namespace lmms::gui
{

class SimpleTextFloat;

class CommandData
{
public:
	/*
	* @param id: a unique identifier for the command, used to make handling commands easy
	* @param name: a name that describes what this command does
	* @param doFnIn: what function should run if this command is triggered
	* @param undoFnIn: what function should run if this command is undone
	* @param isTypeSpecific: if the command can not be performed with other kinds of widgets (example for false: delete command)
	* @param acceptedType: what kind of datatype does this command accept, use `Clipboard::DataType::Ignore` if datatype doesn't matter, use `Clipboard::DataType::Error` to later define the datatype
	*/
	CommandData(size_t id, const QString&& name, const CommandFnPtr& doFnIn, const CommandFnPtr& undoFnIn, bool isTypeSpecific);
	CommandData(size_t id, const QString&& name, const CommandFnPtr& doFnIn, bool isTypeSpecific);
	~CommandData(); // TODO
	//! sets the shortcut and isShortcut will be true
	void setShortcut(Qt::Key shortcutKey, Qt::KeyboardModifier shortcutModifier, bool isShortcutLoop);
	//! use this to add more datatypes to a command `Clipboard::DataType::Error` will not be added
	void addAcceptedDataType(Clipboard::DataType type);

	bool doesShortcutMatch(QKeyEvent* event) const;
	bool doesShortcutMatch(const CommandData& otherShortcut) const;
	void copyShortcut(const CommandData& otherShortcut);
	//! true if `type` is in `acceptedType`, isn't true if `acceptedType` has `Clipboard::DataType::Any`
	bool doesTypeMatch(Clipboard::DataType type) const;
	//! true if `type` is in `acceptedType`, true if `acceptedType` has `Clipboard::DataType::Any`
	bool isTypeAccepted(Clipboard::DataType type) const;
	const QString& getText() const;

	size_t getId() const;
	std::shared_ptr<CommandFnPtr> getDoFn() const;
	std::shared_ptr<CommandFnPtr> getUndoFn() const;
private:
	size_t m_commandId = 0;
	//! display name
	QString m_commandName = "";

	//! Function pointers needed to construct commands
	std::shared_ptr<CommandFnPtr> m_doFn;
	std::shared_ptr<CommandFnPtr> m_undoFn;

	//! if the command can not be performed with other kinds of widgets (example for false: delete command)
	bool m_isTypeSpecific = true;
	bool m_isShortcut = false;

	//! what kind of data does this command accept
	std::vector<Clipboard::DataType> m_acceptedType = {Clipboard::DataType::Any};

	// shortcut logic
	Qt::Key m_key = Qt::Key_F35;
	Qt::KeyboardModifier m_modifier = Qt::NoModifier;
};



class LMMS_EXPORT InteractiveModelView : public QWidget
{
Q_OBJECT
	Q_PROPERTY(QColor highlightColor READ getHighlightColor WRITE setHighlightColor)
public:
	//! typeId: get it by using `getTypeId()` with your inherited class
	InteractiveModelView(QWidget* widget, size_t typeId);
	~InteractiveModelView() override;

	//! highlight every InteractiveModelView that accepts dataType
	static void startHighlighting(Clipboard::DataType dataType);
	static void stopHighlighting();
	static void showMessage(QString& message);
	static void hideMessage();

	static QColor getHighlightColor();
	static void setHighlightColor(QColor& color);

	//! @param highlights this widgets specifically
	//! @param duration: highlight time in milliseconds
	//! @param shouldStopHighlightingOrhers: calls `stopHighlighting()`,
	//! should be false if multiple widgets are highlighted and `this` is not the first one
	void HighlightThisWidget(const QColor& color, size_t duration, bool shouldStopHighlightingOrhers = true);

	//! do a specified command on this widget
	//! @param functionPointer: what command to do from the `getCommands()` array
	//! @param shouldLinkBack: should the command before this be undone as well?
	//! NOTE: ONLY USE `doCommand()` IN QT FUNCTIONS OR COMMAND FUNCTIONS
	//! commands are meant to be triggered by users, triggering them from an internal function could lead to bad journalling
	void doCommand(size_t commandId, bool shouldLinkBack = false);
	void doCommandAt(size_t commandIndex, bool shouldLinkBack = false);

	template<typename DataType>
	void doCommand(size_t commandId, DataType doData, bool shouldLinkBack = false)
		{ doCommandAt<DataType>(getIndexFromId(commandId), doData, nullptr, shouldLinkBack); }
	template<typename DataType>
	void doCommand(size_t commandId, DataType doData, DataType undoData, bool shouldLinkBack = false)
		{ doCommandAt<DataType>(getIndexFromId(commandId), doData, &undoData, shouldLinkBack); }
	template<typename DataType>
	void doCommandAt(size_t commandIndex, DataType doData, DataType* undoData = nullptr, bool shouldLinkBack = false)
	{
		const std::vector<CommandData>& commands = getCommands();
		if (commandIndex > commands.size()) { return; }
		// if the command accepts the current clipboard data, `Clipboard::DataType::Any` will accept anything
		if (commands[commandIndex].isTypeAccepted(Clipboard::decodeKey(Clipboard::getMimeData())) == false) { return; }

		assert(commands[commandIndex].getDoFn().get() != nullptr);
		TypedCommand<DataType> command(commands[commandIndex].getText(), this, commands[commandIndex].getDoFn(), commands[commandIndex].getUndoFn(), doData, undoData, shouldLinkBack);
		command.redo();
	}

	//! should return a unique id for different widget classes
	template<typename BaseType>
	static constexpr size_t getTypeId() { return typeid(BaseType).hash_code(); };
	size_t getStoredTypeId();
protected:
	//! returns the avalible commands for the widget
	const std::vector<CommandData>& getCommands() { return m_commandArray; };
	//! override this if the widget requires custom updating code
	//! shouldOverrideUpdate: should `update()` widget if `isHighlighted` didn't changed but for example color changed
	virtual void overrideSetIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate);
	//! place here the `CommandData` code and call it in constructor
	virtual void addCommands(std::vector<CommandData>& targetList) = 0;

	//! draws the highlight automatically for the widget if highlighted
	void drawAutoHighlight(QPainter* painter);

	bool getIsHighlighted() const;
	//! shouldOverrideUpdate: should update if visible, ignore optimizations
	//! shouldOverrideUpdate could be needed if the color was changed
	void setIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate);

	//! @return: an index of a command from a given ID, reutrns getCommands().size() if not found
	size_t getIndexFromId(size_t id);

	//! construct in derived constructor
	std::vector<CommandData> m_commandArray;
private slots:
	inline static void timerStopHighlighting()
	{
		stopHighlighting();
	}
private:
	bool m_isHighlighted;
	
	//! stores the widget's type id that created this object
	const size_t m_interactiveModelViewTypeId;

	static QColor s_highlightColor;
	static QColor s_usedHighlightColor;
	static QTimer* s_highlightTimer;
	static SimpleTextFloat* s_simpleTextFloat;
	static std::list<InteractiveModelView*> s_interactiveWidgets;
};

} // namespace lmms::gui

#endif // LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
