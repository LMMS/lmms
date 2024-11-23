/*
 * InteractiveModelView.h - Implements shortcut system, StringPair system and highlighting for widgets
 *
 * Copyright (c) 2024 szeli1 <TODO/at/gmail/dot/com>
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
#include "lmms_export.h"
#include "ModelView.h"

class QColor;
class QMimeData;
class QPainter;
class QTimer;

namespace lmms::gui
{

class SimpleTextFloat;

class LMMS_EXPORT InteractiveModelView : public QWidget
{
Q_OBJECT
	Q_PROPERTY(QColor highlightColor READ getHighlightColor WRITE setHighlightColor)
public:
	InteractiveModelView(QWidget* widget);
	~InteractiveModelView() override;

	//! highlight every InteractiveModelView that accepts dataType
	static void startHighlighting(Clipboard::StringPairDataType dataType);
	static void stopHighlighting();
	static void showMessage(QString& message);
	static void hideMessage();

	static QColor getHighlightColor();
	static void setHighlightColor(QColor& color);

	//! returns true if successful
	//! checks if QKeyEvent maches with a known shortcut and calls `processShortcutPressed()`
	bool HandleKeyPress(QKeyEvent* event);
protected:
	struct ModelShortcut
	{
		ModelShortcut() {}
		ModelShortcut(Qt::Key key, Qt::KeyboardModifier modifier, unsigned int times, QString description, bool shouldLoop) :
			key(key),
			modifier(modifier),
			times(times),
			shortcutDescription(description),
			shouldLoop(shouldLoop)
		{
		}

		bool operator==(ModelShortcut& rhs)
		{
			return key == rhs.key
				&& modifier == rhs.modifier
				&& times == rhs.times
				&& shouldLoop == rhs.shouldLoop;
		}

		void reset()
		{
			key = Qt::Key_F35;
			modifier = Qt::NoModifier;
			times = 0;
			shortcutDescription = "";
			shouldLoop = false;
		}

		Qt::Key key = Qt::Key_F35;
		Qt::KeyboardModifier modifier = Qt::NoModifier;
		//! how many times do the keys need to be pressed to activate this shortcut
		unsigned int times = 0;
		//! what the shortcut does
		QString shortcutDescription = "";
		//! should it loop back if m_times is reached
		bool shouldLoop = false;
	};

	void keyPressEvent(QKeyEvent* event) override;
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
	
	//! returns the avalible shortcuts for the widget
	virtual const std::vector<ModelShortcut>& getShortcuts() = 0;
	//! called when a shortcut from `getShortcuts()` is pressed
	virtual void processShortcutPressed(size_t shortcutLocation, QKeyEvent* event) = 0;
	//! called when a shortcut message needs to be displayed
	//! shortcut messages can be generated with `buildShortcutMessage()` (but it can be unoptimized to return `buildShortcutMessage()`)
	virtual QString getShortcutMessage() = 0;
	//! returns true if the widget supports pasting / dropping `dataType` (used for StringPairDrag and Copying)
	virtual bool canAcceptClipboardData(Clipboard::StringPairDataType dataType) = 0;
	//! should implement dragging and dropping widgets or pasting from clipboard
	//! should return if `QDropEvent` event can be accepted
	//! force implement this method
	virtual bool processPasteImplementation(Clipboard::StringPairDataType type, QString& value) = 0;
	//! calls `processPasteImplementation()` to process paste
	bool processPaste(const QMimeData* mimeData);
	//! override this if the widget requires custom updating code
	virtual void overrideSetIsHighlighted(bool isHighlighted);

	//! draws the highlight automatically for the widget if highlighted
	void drawAutoHighlight(QPainter* painter);
	//! builds a string from `getShortcuts()`
	QString buildShortcutMessage();

	bool getIsHighlighted() const;
	void setIsHighlighted(bool isHighlighted);
private slots:
	inline static void timerStopHighlighting()
	{
		stopHighlighting();
	}
private:
	bool doesShortcutMatch(const ModelShortcut* shortcut, QKeyEvent* event) const;
	bool doesShortcutMatch(const ModelShortcut* shortcutA, const ModelShortcut* shortcutB) const;
	

	bool m_isHighlighted;
	
	ModelShortcut m_lastShortcut;
	unsigned int m_lastShortcutCounter;

	static std::unique_ptr<QColor> s_highlightColor;
	static QTimer* s_highlightTimer;
	static SimpleTextFloat* s_simpleTextFloat;
	static std::list<InteractiveModelView*> s_interactiveWidgets;
};

} // namespace lmms::gui

#endif // LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
