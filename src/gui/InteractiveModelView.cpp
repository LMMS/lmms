/*
 * InteractiveModelView.cpp - Implements StringPair system and highlighting for widgets
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

#include "InteractiveModelView.h"

#include <algorithm>

#include <QKeyEvent>
#include <QKeySequence> // displaying qt key names
#include <QMimeData> // processPaste()
#include <QPainter> // drawAutoHighlight()
#include <QPainterPath> // drawAutoHighlight()

#include "GuiApplication.h"
#include "MainWindow.h"
#include "SimpleTextFloat.h"

namespace lmms::gui
{

std::unique_ptr<QColor> InteractiveModelView::s_highlightColor = std::make_unique<QColor>();
std::unique_ptr<QColor> InteractiveModelView::s_usedHighlightColor = std::make_unique<QColor>();
QTimer* InteractiveModelView::s_highlightTimer = nullptr;

SimpleTextFloat* InteractiveModelView::s_simpleTextFloat = nullptr;
std::list<InteractiveModelView*> InteractiveModelView::s_interactiveWidgets;

InteractiveModelView::InteractiveModelView(QWidget* widget) :
	QWidget(widget),
	m_isHighlighted(false)
{
	s_interactiveWidgets.push_back(this);
}

InteractiveModelView::~InteractiveModelView()
{
	auto it = std::find(s_interactiveWidgets.begin(), s_interactiveWidgets.end(), this);
	if (it != s_interactiveWidgets.end())
	{
		s_interactiveWidgets.erase(it);
	}
}

void InteractiveModelView::startHighlighting(Clipboard::DataType dataType)
{
	if (s_highlightTimer == nullptr)
	{
		s_highlightTimer = new QTimer(getGUI()->mainWindow());
		s_highlightTimer->setSingleShot(true);
		QObject::connect(s_highlightTimer, &QTimer::timeout, timerStopHighlighting);
	}

	bool shouldOverrideUpdate = *s_usedHighlightColor != *s_highlightColor;
	if (shouldOverrideUpdate) { s_usedHighlightColor = std::make_unique<QColor>(*s_highlightColor); }
	for (auto it = s_interactiveWidgets.begin(); it != s_interactiveWidgets.end(); ++it)
	{
		(*it)->overrideSetIsHighlighted((*it)->canAcceptClipboardData(dataType), shouldOverrideUpdate);
	}
	s_highlightTimer->start(10000);
}

void InteractiveModelView::stopHighlighting()
{
	for (auto it = s_interactiveWidgets.begin(); it != s_interactiveWidgets.end(); ++it)
	{
		(*it)->overrideSetIsHighlighted(false, false);
	}
}

void InteractiveModelView::showMessage(QString& message)
{
	if (s_simpleTextFloat == nullptr)
	{
		// we don't own this object, so we do not need to delete it
		s_simpleTextFloat = new SimpleTextFloat();
	}
	s_simpleTextFloat->setText(message);
	s_simpleTextFloat->moveToGlobal(QPoint(getGUI()->mainWindow()->pos().x() + 2,
		getGUI()->mainWindow()->pos().y() + getGUI()->mainWindow()->height()));
	s_simpleTextFloat->showWithDelay(0, 60000);
}

void InteractiveModelView::hideMessage()
{
	if (s_simpleTextFloat == nullptr)
	{
		s_simpleTextFloat = new SimpleTextFloat();
	}
	s_simpleTextFloat->hide();
}

QColor InteractiveModelView::getHighlightColor()
{
	return *s_highlightColor;
}

void InteractiveModelView::setHighlightColor(QColor& color)
{
	s_highlightColor = std::make_unique<QColor>(color);
}

void InteractiveModelView::HighlightThisWidget(const QColor& color, size_t duration, bool shouldStopHighlightingOrhers)
{
	if (s_highlightTimer == nullptr)
	{
		s_highlightTimer = new QTimer(getGUI()->mainWindow());
		s_highlightTimer->setSingleShot(true);
		QObject::connect(s_highlightTimer, &QTimer::timeout, timerStopHighlighting);
	}

	if (shouldStopHighlightingOrhers)
	{
		// since only 1 `s_highlightTimer` exists, every other widget needs to stop using it
		stopHighlighting();
	}
	
	bool shouldOverrideUpdate = *s_usedHighlightColor != color;
	if (shouldOverrideUpdate) { s_usedHighlightColor = std::make_unique<QColor>(color); }
	overrideSetIsHighlighted(true, shouldOverrideUpdate);
	s_highlightTimer->start(duration);
}

bool InteractiveModelView::processPaste(const QMimeData* mimeData)
{
	if (mimeData->hasFormat(Clipboard::mimeType(Clipboard::MimeType::StringPair)) == false) { return false; }

	Clipboard::DataType type = Clipboard::decodeKey(mimeData);
	QString value = Clipboard::decodeValue(mimeData);
	bool shouldAccept = processPasteImplementation(type, value);
	if (shouldAccept)
	{
		InteractiveModelView::stopHighlighting();
	}
	return shouldAccept;
}

void InteractiveModelView::overrideSetIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate)
{
	setIsHighlighted(isHighlighted, shouldOverrideUpdate);
}

void InteractiveModelView::drawAutoHighlight(QPainter* painter)
{
	if (getIsHighlighted())
	{
		QColor fillColor = *s_usedHighlightColor;
		fillColor.setAlpha(70);
		painter->fillRect(QRect(1, 1, width() - 2, height() - 2), fillColor);

		painter->setPen(QPen(*s_usedHighlightColor, 2));
		painter->drawLine(1, 1, 5, 1);
		painter->drawLine(1, 1, 1, 5);
		painter->drawLine(width() - 2, height() - 2, width() - 6, height() - 2);
		painter->drawLine(width() - 2, height() - 2, width() - 2, height() - 6);
	}
}

bool InteractiveModelView::getIsHighlighted() const
{
	return m_isHighlighted;
}

void InteractiveModelView::setIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate)
{
	if (shouldOverrideUpdate || m_isHighlighted != isHighlighted)
	{
		m_isHighlighted = isHighlighted;
		if (isVisible())
		{
			update();
		}
	}
}

} // namespace lmms::gui
