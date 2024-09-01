/*
 * InteractiveModelView.h - TODO
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
#include <vector>

#include <QApplication>
#include <QWidget>

#include "Clipboard.h"
#include "lmms_export.h"
#include "ModelView.h"

class QMimeData;
class QPainter;

namespace lmms::gui
{

class SimpleTextFloat;

class LMMS_EXPORT InteractiveModelView : public QWidget
{
Q_OBJECT
public:
	InteractiveModelView(QWidget* widget);
	~InteractiveModelView() override;

	static void startHighlighting(Clipboard::StringPairDataType dataType);
	static void stopHighlighting();
	static void showMessage(QString& message);
	static void hideMessage();

protected:
	class ModelShortcut
	{
	public:
		inline ModelShortcut() {}
		inline ModelShortcut(Qt::Key key, Qt::KeyboardModifier modifier, unsigned int times, QString description, bool shouldLoop) :
			m_key(key),
			m_modifier(modifier),
			m_times(times),
			m_shortcutDescription(description),
			m_shouldLoop(shouldLoop)
		{
		}

		inline bool operator==(ModelShortcut& rhs)
		{
			return m_key == rhs.m_key
				&& m_modifier == rhs.m_modifier
				&& m_times == rhs.m_times
				&& m_shouldLoop == rhs.m_shouldLoop;
		}

		inline void reset()
		{
			m_key = Qt::Key_F35;
			m_modifier = Qt::NoModifier;
			m_times = 0;
			m_shortcutDescription = "";
			m_shouldLoop = false;
		}

		Qt::Key m_key = Qt::Key_F35;
		Qt::KeyboardModifier m_modifier = Qt::NoModifier;
		//! how many times do the keys need to be pressed to activate this shortcut
		unsigned int m_times = 0;
		//! what the shortcut does
		QString m_shortcutDescription = "";
		//! should it loop back if m_times is reached
		bool m_shouldLoop = false;
	};

	void keyPressEvent(QKeyEvent* event) override;
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
	
	//! return the avalible shortcuts for the widget
	virtual std::vector<ModelShortcut> getShortcuts() = 0;
	//! called when a `getShortcuts()` shortcut shortcut is pressed
	virtual void shortcutPressedEvent(size_t shortcutLocation, QKeyEvent* event) = 0;
	//! called when a shortcut message needs to be displayed
	//! should return the message built with `buildShortcutMessage()`
	virtual QString& getShortcutMessage() = 0;
	//! return true if the widget supports pasting / dropping `dataType` (used for StringPairDrag and Copying)
	virtual bool canAcceptClipBoardData(Clipboard::StringPairDataType dataType) = 0;
	//! should implement dragging and dropping widgets or pasting from clipboard
	//! should return if `QDropEvent` event can be accepted
	//! force implement this method
	virtual bool processPaste(const QMimeData* mimeData) = 0;

	void drawAutoHighlight(QPainter* painter);
	QString buildShortcutMessage();

	bool getIsHighlighted() const;
private:
	void setIsHighlighted(bool isHighlighted);
	//! draws
	bool doesShortcutMatch(const ModelShortcut* shortcut, QKeyEvent* event) const;
	bool doesShortcutMatch(const ModelShortcut* shortcutA, const ModelShortcut* shortcutB) const;

	bool m_isHighlighted;
	
	ModelShortcut m_lastShortcut;
	unsigned int m_lastShortcutCounter;

	QWidget* m_focusedBeforeWidget;

	static SimpleTextFloat* s_simpleTextFloat;
	static std::list<InteractiveModelView*> s_interactiveWidgets;
};

} // namespace lmms::gui

#endif // LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
