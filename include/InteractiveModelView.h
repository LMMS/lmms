/*
 * InteractiveModelView.h - Implements action system for widgets
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

struct CommandData
{
	/*
	* id: should be unique, used to refer to this action
	* doFnIn: what function should run if this action is triggered
	* undoFnIn: what function should run if this action is undone
	* isTypeSpecific: if the action can not be performed with other kinds of widgets (example for false: delete action)
	* acceptedType: what kind of datatype does this action accept, use `Clipboard::DataType::Ignore` if datatype doesn't matter, use `Clipboard::DataType::Error` to later define the datatype
	*/
	CommandData(QString& name, CommandFnPtr& doFnIn, CommandFnPtr& undoFnIn, bool isTypeSpecific, Clipboard::DataType acceptedType);
	~CommandData();
	//! sets the shortcut and isShortcut will be true
	void setShortcut(Qt::Key shortcutKey, Qt::KeyboardModifier shortcutModifier, size_t shortcutTimes, bool isShortcutLoop);
	//! use this to add more datatypes to an action `Clipboard::DataType::Error` will not be added
	void addAcceptedDataType(Clipboard::DataType type);
	//! display name
	QString actionName = "";

	//! Function pointers needed to construct `GuiAction`
	CommandFnPtr doFn;
	CommandFnPtr undoFn;

	//! if the action can not be performed with other kinds of widgets (example for false: delete action)
	bool isTypeSpecific = true;
	bool isShortcut = false;

	//! what kind of data does this action accept
	std::vector<Clipboard::DataType> acceptedType = {Clipboard::DataType::Any};

	// shortcut logic
	Qt::Key key = Qt::Key_F35;
	Qt::KeyboardModifier modifier = Qt::NoModifier;
	//! should it loop back if m_times is reached
	bool isLoop = false;

	//! resets the shortcut and isShortcut will be false
	void resetShortcut();
	
	bool doesShortcutMatch(QKeyEvent* event) const;
	bool doesShortcutMatch(const CommandData& otherShortcut) const;
	bool doesFullShortcutMatch(const CommandData& otherShortcut) const;
	void copyShortcut(const CommandData& otherShortcut);
	//! true if `type` is in `acceptedType`, isn't true if `acceptedType` has `Clipboard::DataType::Any`
	bool doesTypeMatch(Clipboard::DataType type) const;
	//! true if `type` is in `acceptedType`, true if `acceptedType` has `Clipboard::DataType::Any`
	bool isTypeAccepted(Clipboard::DataType type) const;
	const QString& getText() const;
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
	void HighlightThisWidget(const QColor& color, size_t duration, bool shouldStopHighlightingOrhers = true);s

	//! do a specified action on this widget
	//! @param functionPointer: what action to do from the `getActions()` array
	//! @param shouldLinkBack: should the Action before this be undone as well?
	//! NOTE: ONLY USE `doAction()` IN QT FUNCTIONS OR ACTION FUNCTIONS
	//! actions are meant to be triggered by users, triggering them from an internal function could lead to bad journalling
	void doAction(void* functionPointer, bool shouldLinkBack = false);
	void doActionAt(size_t actionIndex, bool shouldLinkBack = false);

	template<typename DataType>
	void doAction(void* functionPointer, DataType doData, bool shouldLinkBack = false)
		{ doActionAt(getIndexFromFn(functionPointer), doData, nullptr, shouldLinkBack); }
	void doAction(void* functionPointer, DataType doData, DataType undoData, bool shouldLinkBack = false)
		{ doActionAt(getIndexFromFn(functionPointer), doData, &undoData, shouldLinkBack); }
	template<typename DataType>
	void doActionAt(size_t actionIndex, DataType doData, DataType* undoData = nullptr, bool shouldLinkBack = false)
	{
		const std::vector<CommandData>& actions = getActions();
		if (actionIndex > actions.size()) { return; }
		// if the action accepts the current clipboard data, `Clipboard::DataType::Any` will accept anything
		qDebug("doActionAt typed before type return");
		if (actions[actionIndex].isTypeAccepted(Clipboard::decodeKey(Clipboard::getMimeData())) == false) { return; }
		qDebug("doActionAt typed after type return");

		qDebug("doAction typed, %ld, %s", actionIndex, actions[actionIndex].getText().toStdString().c_str());
		GuiActionTyped<DataType> action(actions[actionIndex].getText(), this, actions[actionIndex].doFn, actions[actionIndex].undoFn, doData, undoData, shouldLinkBack);
		action.redo();
	}

	//! should return a unique id for different widget classes
	template<typename BaseType>
	static constexpr size_t getTypeId() { return typeid(BaseType).hash_code(); };
	size_t getStoredTypeId();
protected:
	//! returns the avalible shortcuts for the widget
	const std::vector<CommandData>& getActions() { return m_actionArray; };
	//! override this if the widget requires custom updating code
	//! shouldOverrideUpdate: should `update()` widget if `isHighlighted` didn't changed but for example color changed
	virtual void overrideSetIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate);
	//! place here the `QAction` and `CommandData` code and call it in constructor
	virtual void addActions(std::vector<CommandData>& targetList) = 0;

	//! draws the highlight automatically for the widget if highlighted
	void drawAutoHighlight(QPainter* painter);
	//! builds a string from a provided action array
	static QString buildShortcutMessage(const std::vector<CommandData>& actions);

	bool getIsHighlighted() const;
	//! shouldOverrideUpdate: should update if visible, ignore optimizations
	//! shouldOverrideUpdate could be needed if the color was changed
	void setIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate);

	//! returns an index of an action from a given id, assert fails if not found
	size_t getIndexFromFn(void* functionPointer);

	//! construct in derived constructor
	std::vector<CommandData> m_actionArray;
private slots:
	inline static void timerStopHighlighting()
	{
		stopHighlighting();
	}
private:
	bool m_isHighlighted;
	
	//! stores the widget's type id that created this object
	const size_t m_interactiveModelViewTypeId;

	static std::unique_ptr<QColor> s_highlightColor;
	static std::unique_ptr<QColor> s_usedHighlightColor;
	static QTimer* s_highlightTimer;
	static SimpleTextFloat* s_simpleTextFloat;
	static std::list<InteractiveModelView*> s_interactiveWidgets;
};

/*
template<typename BaseType>
class LMMS_EXPORT InteractiveModelViewTyped : public InteractiveModelView
{
public:
	InteractiveModelViewTyped(QWidget* widget) :
		InteractiveModelView(widget, getTypeId<BaseType>())
	{}
	InteractiveModelViewTyped(QWidget* widget, size_t typeId) :
		InteractiveModelView(widget, typeId)
	{}
protected:
	//! use Template Specialization to add customized actions
	static std::vector<CommandData> addActions() { return std::vector<CommandData>(); }
	virtual const std::vector<CommandData>& getActions() override { return s_actionArray; }
	static std::vector<CommandData> s_actionArray;
};

inline template<typename BaseType>
std::vector<CommandData> InteractiveModelViewTyped<BaseType>::s_actionArray(InteractiveModelViewTyped<BaseType>::addActions());
*/

} // namespace lmms::gui

#endif // LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
