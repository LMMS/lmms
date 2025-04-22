/*
 * InteractiveModelView.h - Implements shortcut and action system for widgets
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
#include "GuiAction.h"
#include "lmms_export.h"

class QColor;
class QMimeData;
class QPainter;
class QTimer;

namespace lmms::gui
{

class SimpleTextFloat;

struct ActionStruct
{
	/*
	* id: should be unique, used to refer to this action
	* doFnIn: what function should run if this action is triggered
	* undoFnIn: what function should run if this action is undone
	* isTypeSpecific: if the action can not be performed with other kinds of widgets (example for false: delete action)
	* acceptedType: what kind of datatype does this action accept, use `Clipboard::DataType::Ignore` if datatype doesn't matter, use `Clipboard::DataType::Error` to later define the datatype
	*/
	ActionStruct(size_t id, QAction& doFnIn, QAction* undoFnIn, bool isTypeSpecific, Clipboard::DataType acceptedType);
	ActionStruct() { resetShortcut(); }
	~ActionStruct();
	//! sets the shortcut and isShortcut will be true
	void setShortcut(Qt::Key shortcutKey, Qt::KeyboardModifier shortcutModifier, size_t shortcutTimes, bool isShortcutLoop);
	//! use this to add more datatypes to an action `Clipboard::DataType::Error` will not be added
	void addAcceptedDataType(Clipboard::DataType type);
	//! display name
	//QString actionName = "";
	size_t actionId = 0;

	//! `QActions` needed for `GuiAction`
	QAction* doFn = nullptr;
	QAction* undoFn = nullptr;
	GuiActionIO data;

	//! if the action can not be performed with other kinds of widgets (example for false: delete action)
	bool isTypeSpecific = true;
	bool isShortcut = false;

	//! what kind of data does this action accept
	std::vector<Clipboard::DataType> acceptedType = {Clipboard::DataType::Any};

	// shortcut logic
	Qt::Key key = Qt::Key_F35;
	Qt::KeyboardModifier modifier = Qt::NoModifier;
	//! how many times do the keys need to be pressed to activate this shortcut
	size_t times = 0;
	//! should it loop back if m_times is reached
	bool isLoop = false;

	//! resets the shortcut and isShortcut will be false
	void resetShortcut();
	
	bool doesShortcutMatch(QKeyEvent* event) const;
	bool doesShortcutMatch(const ActionStruct& otherShortcut) const;
	bool doesFullShortcutMatch(const ActionStruct& otherShortcut) const;
	void copyShortcut(const ActionStruct& otherShortcut);
	//! true if `type` is in `acceptedType`, isn't true if `acceptedType` has `Clipboard::DataType::Any`
	bool doesTypeMatch(Clipboard::DataType type) const;
	//! true if `type` is in `acceptedType`, true if `acceptedType` has `Clipboard::DataType::Any`
	bool isTypeAccepted(Clipboard::DataType type) const;
	const QString getText() const;
	GuiActionIO* getData();
	void setData(const GuiActionIO& newData);
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

	//! highlights this widgets specifically
	//! duration: highlight time in milliseconds
	//! shouldStopHighlightingOrhers: calls `stopHighlighting()`,
	//! should be false if multiple widgets are highlighted and `this` is not the first one
	void HighlightThisWidget(const QColor& color, size_t duration, bool shouldStopHighlightingOrhers = true);

	//! returns true if successful
	//! checks if QKeyEvent maches with a known shortcut and calls `processShortcutPressed()`
	bool HandleKeyPress(QKeyEvent* event);

	//! do a specified action on this widget
	//! actionId: what action to do from the `getActions()` array
	//! shouldLinkBack: should the Action before this be undone as well?
	//! NOTE: ONLY USE `doAction()` IN QT FUNCTIONS OR ACTION FUNCTIONS
	//! actions are meant to be triggered by users, triggering them from an internal function could lead to bad journalling
	void doAction(size_t actionId, bool shouldLinkBack = false);
	//static void doActions(size_t actionId, const std::vector<InteractiveModelView*> widgets, const std::vector<ActionStruct>& actions);
	void doAction(size_t actionId, GuiActionIO data, bool shouldLinkBack = false);
	void doActionAt(size_t actionIndex, bool shouldLinkBack = false);
	void doActionAt(size_t actionIndex, GuiActionIO data, bool shouldLinkBack = false);

	//! should return a unique id for different widget classes
	template<typename BaseType>
	static constexpr size_t getTypeId() { return typeid(BaseType).hash_code(); };
	size_t getStoredTypeId();
protected:
	void keyPressEvent(QKeyEvent* event) override;
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;

	//! returns the avalible shortcuts for the widget
	const std::vector<ActionStruct>& getActions() { return m_actionArray; };
	std::vector<ActionStruct>& getActionsNotConst() { return m_actionArray; };
	//! called when a shortcut message needs to be displayed
	//! shortcut messages can be generated with `buildShortcutMessage()` (but it can be unoptimized to return `buildShortcutMessage()`)
	virtual const QString& getShortcutMessage() = 0;
	//! override this if the widget requires custom updating code
	//! shouldOverrideUpdate: should `update()` widget if `isHighlighted` didn't changed but for example color changed
	virtual void overrideSetIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate);
	//! place here the `QAction` and `ActionStruct` code and call it in constructor
	virtual void addActions(std::vector<ActionStruct>& targetList) = 0;

	//! draws the highlight automatically for the widget if highlighted
	void drawAutoHighlight(QPainter* painter);
	//! builds a string from a provided action array
	static QString buildShortcutMessage(const std::vector<ActionStruct>& actions);

	bool getIsHighlighted() const;
	//! shouldOverrideUpdate: should update if visible, ignore optimizations
	//! shouldOverrideUpdate could be needed if the color was changed
	void setIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate);

	//! returns an index of an action from a given id, returns `getActions().size()` if not found
	size_t getIndexFromId(size_t id);

	//! construct in derived constructor
	std::vector<ActionStruct> m_actionArray;
private slots:
	inline static void timerStopHighlighting()
	{
		stopHighlighting();
	}
private:
	bool m_isHighlighted;
	
	ActionStruct m_lastShortcut;
	//! how many times was the last shortcut pressed
	size_t m_lastShortcutCounter;
	//! stores the widget's type id that created this object
	const size_t m_interactiveModelViewTypeId;

	static std::unique_ptr<QColor> s_highlightColor;
	static std::unique_ptr<QColor> s_usedHighlightColor;
	static QTimer* s_highlightTimer;
	static SimpleTextFloat* s_simpleTextFloat;
	static std::list<InteractiveModelView*> s_interactiveWidgets;
};

} // namespace lmms::gui

#endif // LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
