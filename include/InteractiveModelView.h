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

#include <QApplication>
#include <QWidget>
#include <QColor>

#include "Clipboard.h"
#include "GuiAction.h"
#include "lmms_export.h"
#include "ModelView.h"

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
	* actionName: short string that will be displayed (in shortcuts and context menus)
	* actionHint: description of the action
	* doFn: what function should run if this action is triggered
	* undoFn: what function should run if this action is undone
	* isTypeSpecific: if the action can not be performed with other kinds of widgets (example for false: delete action)
	* acceptedType: what kind of datatype does this action accept, use `Clipboard::DataType::Ignore` if datatype doesn't matter, use `Clipboard::DataType::Error` to later define the datatype
	*/
	ActionStruct(const QString& actionName, const QString& actionHint, GuiAction::ActionFn doFn, GuiAction::ActionFn undoFn, bool isTypeSpecific, Clipboard::DataType acceptedType);
	ActionStruct(const QString& actionName, const QString& actionHint, GuiAction::FloatActionFn doFn, GuiAction::FloatActionFn undoFn, bool isTypeSpecific, Clipboard::DataType acceptedType);
	ActionStruct() { resetShortcut(); }
	//! sets the shortcut and isShortcut will be true
	void setShortcut(Qt::Key shortcutKey, Qt::KeyboardModifier shortcutModifier, size_t shortcutTimes, bool isShortcutLoop);
	//! use this to add more datatypes to an action `Clipboard::DataType::Error` will not be added
	void addAcceptedDataType(Clipboard::DataType type);
	//! display name
	QString actionName = "";
	//! hint or description
	QString actionHint = "";

	//! function pointers needed for `GuiAction`
	GuiAction::ActionFn doFn = nullptr;
	GuiAction::ActionFn undoFn = nullptr;
	GuiAction::FloatActionFn doFloatFn = nullptr;
	GuiAction::FloatActionFn undoFloatFn = nullptr;

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
	//! true if `type` is in `acceptedType`, isn't true if `acceptedType` has `Clipboard::DataType::Any`
	bool doesTypeMatch(Clipboard::DataType type) const;
	//! true if `type` is in `acceptedType`, true if `acceptedType` has `Clipboard::DataType::Any`
	bool isTypeAccepted(Clipboard::DataType type) const;
	bool doesActionMatch(const ActionStruct& otherAction) const;
};


class LMMS_EXPORT InteractiveModelView : public QWidget
{
Q_OBJECT
	Q_PROPERTY(QColor highlightColor READ getHighlightColor WRITE setHighlightColor)
public:
	InteractiveModelView(QWidget* widget);
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
protected:
	void keyPressEvent(QKeyEvent* event) override;
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
	
	//! returns the avalible shortcuts for the widget
	virtual const std::vector<ActionStruct>& getActions() = 0;
	//! TODO
	void doAction(size_t actionIndex);
	void doAction(size_t actionIndex, const std::vector<ActionStruct>& actions);
	static void doActions(size_t actionIndex, const std::vector<InteractiveModelView*> widgets, const std::vector<ActionStruct>& actions);
	//! called when a shortcut message needs to be displayed
	//! shortcut messages can be generated with `buildShortcutMessage()` (but it can be unoptimized to return `buildShortcutMessage()`)
	virtual QString getShortcutMessage() = 0;
	//! should implement dragging and dropping widgets or pasting from clipboard
	//! should return if `QDropEvent` event can be accepted
	//! force implement this method
	virtual bool processPasteImplementation(Clipboard::DataType type, QString& value) = 0;
	//! calls `processPasteImplementation()` to process paste
	bool processPaste(const QMimeData* mimeData);

	//! override this if the widget requires custom updating code
	//! shouldOverrideUpdate: should `update()` widget if `isHighlighted` didn't changed but for example color changed
	virtual void overrideSetIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate);
	virtual size_t getTypeId();
	
	
	//! called when a shortcut from `getActions()` is pressed
	//virtual void processShortcutPressed(size_t shortcutLocation, QKeyEvent* event) = 0;
	//! returns true if the widget supports pasting / dropping `dataType` (used for StringPairDrag and Copying)
	//virtual bool canAcceptClipboardData(Clipboard::DataType dataType) = 0;
	

	//! draws the highlight automatically for the widget if highlighted
	void drawAutoHighlight(QPainter* painter);
	//! builds a string from `getActions()`
	QString buildShortcutMessage();

	bool getIsHighlighted() const;
	//! shouldOverrideUpdate: should update if visible, ignore optimizations
	//! shouldOverrideUpdate could be needed if the color was changed
	void setIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate);
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

	static std::unique_ptr<QColor> s_highlightColor;
	static std::unique_ptr<QColor> s_usedHighlightColor;
	static QTimer* s_highlightTimer;
	static SimpleTextFloat* s_simpleTextFloat;
	static std::list<InteractiveModelView*> s_interactiveWidgets;
};

template<typename DataType>
class LMMS_EXPORT InteractiveModelViewTyped : public InteractiveModelView
{
	
}

} // namespace lmms::gui

#endif // LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
