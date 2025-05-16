/*
 * InteractiveModelView.cpp - Implements action system for widgets
 *
 * Copyright (c) 2024 - 2025 szeli1 <TODO/at/gmail/dot/com>
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
#include <cassert>

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

InteractiveModelView::InteractiveModelView(QWidget* widget, size_t typeId) :
	QWidget(widget),
	m_actionArray(),
	m_isHighlighted(false),
	m_interactiveModelViewTypeId(typeId)
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

	// highlighting the widgets that accept the data type
	for (auto it = s_interactiveWidgets.begin(); it != s_interactiveWidgets.end(); ++it)
	{
		bool found = false;
		const std::vector<CommandData>& actions = (*it)->getActions();
		// this could be optimized by logging `getStoredTypeId()`s and comparing it's Id to the already accepted Ids
		for (auto& curAction : actions)
		{
			if (curAction.doesTypeMatch(dataType)) { found = true; break; }
		}
		(*it)->overrideSetIsHighlighted(found, shouldOverrideUpdate);
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

void InteractiveModelView::doAction(void* functionPointer, bool shouldLinkBack)
{
	InteractiveModelView::doActionAt(getIndexFromFn(functionPointer), shouldLinkBack);
}
void InteractiveModelView::doActionAt(size_t actionIndex, bool shouldLinkBack)
{
	const std::vector<CommandData>& actions = getActions();
	if (actionIndex > actions.size()) { return; }
	// if the action accepts the current clipboard data, `Clipboard::DataType::Any` will accept anything
	qDebug("doActionAt before type return");
	if (actions[actionIndex].isTypeAccepted(Clipboard::decodeKey(Clipboard::getMimeData())) == false) { return; }
	qDebug("doActionAt after type return");

	qDebug("doAction typed, %ld, %s", actionIndex, actions[actionIndex].getText().toStdString().c_str());
	GuiCommand command(actions[actionIndex].getText(), this, *actions[actionIndex].doFn, *actions[actionIndex].undoFn, 1, shouldLinkBack);
	command.redo();
}

void InteractiveModelView::overrideSetIsHighlighted(bool isHighlighted, bool shouldOverrideUpdate)
{
	setIsHighlighted(isHighlighted, shouldOverrideUpdate);
}

size_t InteractiveModelView::getStoredTypeId()
{
	return m_interactiveModelViewTypeId;
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

size_t InteractiveModelView::getIndexFromFn(void* functionPointer)
{
	const std::vector<CommandData>& actions = getActions();
	for (size_t i = 0; i < actions.size(); i++)
	{
		if (actions[i].doFn->isMatch(functionPointer))
		{
			return i;
		}
	}
	assert(false);
	return actions.size();
}


CommandData::CommandData(QString& name, CommandFnPtr& doFnIn, CommandFnPtr& undoFnIn, bool isTypeSpecific, Clipboard::DataType acceptedType) :
	commandName(name),
	doFn(&doFnIn),
	undoFn(&undoFnIn),
	isTypeSpecific(isTypeSpecific),
	isShortcut(false)
{
	addAcceptedDataType(acceptedType);
}

CommandData::~CommandData()
{
	// TODO delete from history
}

void CommandData::setShortcut(Qt::Key shortcutKey, Qt::KeyboardModifier shortcutModifier, size_t shortcutTimes, bool isShortcutLoop)
{
	isShortcut = true;
	key = shortcutKey;
	modifier = shortcutModifier;
	isLoop = isShortcutLoop;
}

void CommandData::addAcceptedDataType(Clipboard::DataType type)
{
	if (type == Clipboard::DataType::Error) { return; }
	acceptedType.push_back(type);
}

void CommandData::resetShortcut()
{
	isShortcut = false;
	key = Qt::Key_F35;
	modifier = Qt::NoModifier;
	isLoop = false;
}

bool CommandData::doesShortcutMatch(QKeyEvent* event) const
{
	// if shortcut key == event key and the shortcut modifier can be found inside event modifiers or there is no modifier
	return isShortcut && key == event->key() && ((event->modifiers() & modifier)
		|| (event->nativeModifiers() <= 0 && modifier == Qt::NoModifier));
}

bool CommandData::doesShortcutMatch(const CommandData& otherShortcut) const
{
	return isShortcut && key == otherShortcut.key && (modifier & otherShortcut.modifier);
}

bool CommandData::doesFullShortcutMatch(const CommandData& otherShortcut) const
{
	return isShortcut && key == otherShortcut.key && (modifier & otherShortcut.modifier) && isLoop == otherShortcut.isLoop;
}

void CommandData::copyShortcut(const CommandData& otherShortcut)
{
	isShortcut = otherShortcut.isShortcut;
	key = otherShortcut.key;
	modifier = otherShortcut.modifier;
	isLoop = otherShortcut.isLoop;
}

bool CommandData::doesTypeMatch(Clipboard::DataType type) const
{
	for (Clipboard::DataType curType : acceptedType)
	{
		if (curType == type) { return true; }
	}
	return false;
}

bool CommandData::isTypeAccepted(Clipboard::DataType type) const
{
	for (Clipboard::DataType curType : acceptedType)
	{
		if (curType == Clipboard::DataType::Any || curType == type) { return true; }
	}
	return false;
}

const QString& CommandData::getText() const
{
	return commandName;
}

} // namespace lmms::gui
