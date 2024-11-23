/*
 * InteractiveModelView.cpp - Implements shortcut system, StringPair system and highlighting for widgets
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
	m_isHighlighted(false),
	m_lastShortcutCounter(0)
{
	s_interactiveWidgets.push_back(this);

	m_lastShortcut.reset();
}

InteractiveModelView::~InteractiveModelView()
{
	auto it = std::find(s_interactiveWidgets.begin(), s_interactiveWidgets.end(), this);
	if (it != s_interactiveWidgets.end())
	{
		s_interactiveWidgets.erase(it);
	}
}

void InteractiveModelView::startHighlighting(Clipboard::StringPairDataType dataType)
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

bool InteractiveModelView::HandleKeyPress(QKeyEvent* event)
{
	std::vector<ModelShortcut> shortcuts(getShortcuts());

	size_t foundIndex = 0;
	unsigned int minMaxTimes = 0;
	bool found = false;

	// if the last shortcut's keys mach the current keys
	if (doesShortcutMatch(&m_lastShortcut, event))
	{
		// find the highest `ModelShortcut::times` or
		// the shortcut that's `ModelShortcut::times` == m_lastShortcutCounter
		for (size_t i = 0; i < shortcuts.size(); i++)
		{
			if (doesShortcutMatch(&shortcuts[i], event))
			{
				// selecting the shortcut with the largest m_times
				if (found == false || minMaxTimes < shortcuts[i].times)
				{
					foundIndex = i;
					minMaxTimes = shortcuts[i].times;
				}
				found = true;
			
				if (m_lastShortcutCounter == shortcuts[i].times)
				{
					m_lastShortcutCounter = shortcuts[i].shouldLoop ? 0 : m_lastShortcutCounter + 1;
					foundIndex = i;
					break;
				}
			}
		}
	}
	else
	{
		// find the lowest `ModelShortcut::times`
		for (size_t i = 0; i < shortcuts.size(); i++)
		{
			if (doesShortcutMatch(&shortcuts[i], event))
			{
				// selecting the shortcut with the lowest `ModelShortcut::times`
				if (found == false || minMaxTimes > shortcuts[i].times)
				{
					foundIndex = i;
					minMaxTimes = shortcuts[i].times;
				}
				m_lastShortcut = shortcuts[i];
				m_lastShortcutCounter = 1;
				found = true;
			}
		}
	}
	if (found)
	{
		QString message = shortcuts[foundIndex].shortcutDescription;
		showMessage(message);
		processShortcutPressed(foundIndex, event);

		event->accept();
	}
	else
	{
		// reset focus
		if (event->key() != Qt::Key_Control
			&& event->key() != Qt::Key_Shift
			&& event->key() != Qt::Key_Alt
			&& event->key() != Qt::Key_AltGr)
		{
			getGUI()->mainWindow()->setFocusedInteractiveModel(nullptr);
		}
	}
	return found;
}

void InteractiveModelView::keyPressEvent(QKeyEvent* event)
{
	// this will run `HandleKeyPress()` for the widget that is focused inside MainWindow
	getGUI()->mainWindow()->focusedInteractiveModelHandleKeyPress(event);
}

void InteractiveModelView::enterEvent(QEvent* event)
{
	m_lastShortcutCounter = 0;
	m_lastShortcut.reset();

	QString message = getShortcutMessage();
	showMessage(message);

	if (isVisible())
	{
		// focus on this widget so keyPressEvent works
		getGUI()->mainWindow()->setFocusedInteractiveModel(this);
	}
}

void InteractiveModelView::leaveEvent(QEvent* event)
{
	hideMessage();
}

bool InteractiveModelView::processPaste(const QMimeData* mimeData)
{
	if (Clipboard::hasFormat(Clipboard::MimeType::StringPair) == false) { return false; }

	Clipboard::StringPairDataType type = Clipboard::decodeKey(mimeData);
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

QString InteractiveModelView::buildShortcutMessage()
{
	QString message = "";
	std::vector<ModelShortcut> shortcuts(getShortcuts());
	for (size_t i = 0; i < shortcuts.size(); i++)
	{
		message = message + QString("\"") + QKeySequence(shortcuts[i].modifier).toString()
			+ QKeySequence(shortcuts[i].key).toString();
		if (shortcuts[i].times > 0)
		{
			message = message + QString(" (x") + QString::number(shortcuts[i].times + 1) + QString(")");
		}
		message = message + QString("\": ")
			+ shortcuts[i].shortcutDescription;
		if (i + 1 < shortcuts.size())
		{
			message = message + QString(", ");
		}
	}
	return message;
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

bool InteractiveModelView::doesShortcutMatch(const ModelShortcut* shortcut, QKeyEvent* event) const
{
	// if shortcut key == event key and the shortcut modifier can be found inside event modifiers or there is no modifier
	return shortcut->key == event->key() && ((event->modifiers() & shortcut->modifier)
		|| (event->nativeModifiers() <= 0 && shortcut->modifier == Qt::NoModifier));
}

bool InteractiveModelView::doesShortcutMatch(const ModelShortcut* shortcutA, const ModelShortcut* shortcutB) const
{
	return shortcutA->key == shortcutB->key && (shortcutA->modifier & shortcutB->modifier);
}

} // namespace lmms::gui
