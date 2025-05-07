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
protected:

	//! returns true if the widget supports pasting / dropping `dataType` (used for StringPairDrag and Copying)
	virtual bool canAcceptClipboardData(Clipboard::DataType dataType) = 0;
	//! should implement dragging and dropping widgets or pasting from clipboard
	//! should return if `QDropEvent` event can be accepted
	//! force implement this method
	virtual bool processPasteImplementation(Clipboard::DataType type, QString& value) = 0;
	//! calls `processPasteImplementation()` to process paste
	bool processPaste(const QMimeData* mimeData);
	//! override this if the widget requires custom updating code
	virtual void overrideSetIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate);

	//! draws the highlight automatically for the widget if highlighted
	void drawAutoHighlight(QPainter* painter);

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

	static std::unique_ptr<QColor> s_highlightColor;
	static std::unique_ptr<QColor> s_usedHighlightColor;
	static QTimer* s_highlightTimer;
	static SimpleTextFloat* s_simpleTextFloat;
	static std::list<InteractiveModelView*> s_interactiveWidgets;
};

} // namespace lmms::gui

#endif // LMMS_GUI_INTERACTIVE_MODEL_VIEW_H
