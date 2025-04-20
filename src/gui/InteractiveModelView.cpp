/*
 * InteractiveModelView.cpp - Implements shortcut and action system for widgets
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

#include <iostream> // DEBUG REMOVE TODO

namespace lmms::gui
{

std::unique_ptr<QColor> InteractiveModelView::s_highlightColor = std::make_unique<QColor>();
std::unique_ptr<QColor> InteractiveModelView::s_usedHighlightColor = std::make_unique<QColor>();
QTimer* InteractiveModelView::s_highlightTimer = nullptr;

SimpleTextFloat* InteractiveModelView::s_simpleTextFloat = nullptr;
std::list<InteractiveModelView*> InteractiveModelView::s_interactiveWidgets;

InteractiveModelView::InteractiveModelView(QWidget* widget, size_t typeId) :
	QWidget(widget),
	m_isHighlighted(false),
	m_lastShortcutCounter(0),
	m_interactiveModelViewTypeId(typeId)
{
	s_interactiveWidgets.push_back(this);

	m_lastShortcut.resetShortcut();
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
		const std::vector<ActionStruct>& actions = (*it)->getActions();
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

bool InteractiveModelView::HandleKeyPress(QKeyEvent* event)
{
	qDebug("HandleKeyPress 1");
	std::cout << "InteractiveModelView::HandleKeyPress this:" << this << "\n";
	const std::vector<ActionStruct>& actions = getActions();
	qDebug("HandleKeyPress 2");
	
	size_t foundIndex = 0;
	size_t minMaxTimes = 0;
	bool found = false;

	// if the last shortcut's keys match the current keys
	if (m_lastShortcut.doesShortcutMatch(event))
	{
		// find the highest `ActionStruct::times`
		// or the shortcut that's `ActionStruct::times` == m_lastShortcutCounter
		// by default `m_lastShortcutCounter == actions[i].times` shortcut will be the output, 
		// but if this doesn't exist, the shortcut with the highest `times` will be the output
		for (size_t i = 0; i < actions.size(); i++)
		{
			qDebug("HandleKeyPress 3");
			if (actions[i].doesShortcutMatch(event))
			{
				// finding the shortcut with the largest m_times
				if (found == false || minMaxTimes < actions[i].times)
				{
					foundIndex = i;
					minMaxTimes = actions[i].times;
				}
				found = true;
			
				// or finding the shortcut where m_lastShortcutCounter == actions[i].times 
				if (m_lastShortcutCounter == actions[i].times)
				{
					m_lastShortcutCounter = actions[i].isLoop ? 0 : m_lastShortcutCounter + 1;
					foundIndex = i;
					break;
				}
			}
		}
	}
	else
	{
		qDebug("HandleKeyPress 5");
		// when a new shortcut is pressed (not the last)
		// find it with the lowest `ActionStruct::times`
		for (size_t i = 0; i < actions.size(); i++)
		{
			qDebug("HandleKeyPress 6");
			if (actions[i].doesShortcutMatch(event))
			{
				qDebug("HandleKeyPress 7");
				// selecting the shortcut with the lowest `ActionStruct::times`
				if (found == false || minMaxTimes > actions[i].times)
				{
					foundIndex = i;
					minMaxTimes = actions[i].times;
					qDebug("HandleKeyPress 8");
				}
				m_lastShortcut = actions[i];
				m_lastShortcutCounter = 1;
				found = true;
			}
		}
	}
	if (found)
	{
		qDebug("HandleKeyPress 9");
		QString message = actions[foundIndex].actionName;
		qDebug("HandleKeyPress 10");
		showMessage(message);
		qDebug("HandleKeyPress 11");
		doAction(foundIndex);
		qDebug("HandleKeyPress 12");

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
	m_lastShortcut.resetShortcut();

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

size_t InteractiveModelView::getFromFn(void* functionPtr)
{
	const std::vector<ActionStruct>& actions = getActions();
	for (size_t i = 0; i < actions.size(); i++)
	{
		if (actions[i].doFn == functionPtr || actions[i].doTypedFn.isMatch(functionPtr))
		{
			return i;
		}
	}
	return actions.size();
}

void InteractiveModelView::doAction(size_t actionIndex, bool shouldLinkBack)
{
	const std::vector<ActionStruct>& actions = getActions();
	if (actionIndex > actions.size()) { return; }
	// if the action accepts the current clipboard data, `Clipboard::DataType::Any` will accept anything
	if (actions[actionIndex].isTypeAccepted(Clipboard::decodeKey(Clipboard::getMimeData())) == false) { return; }

	// if this assert fails, you will need to call the typed `doAction()`
	assert(actions[actionIndex].doFn != nullptr);

	qDebug("doAction typeless, %d", actionIndex);
	GuiAction action(actions[actionIndex].actionName, this, actions[actionIndex].doFn, actions[actionIndex].undoFn, 1, shouldLinkBack);
	action.redo();
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

QString InteractiveModelView::buildShortcutMessage(const std::vector<ActionStruct>& actions)
{
	QString message = "";
	for (size_t i = 0; i < actions.size(); i++)
	{
		if (actions[i].isShortcut == true)
		{
			message = message + QString("\"") + QKeySequence(actions[i].modifier).toString()
				+ QKeySequence(actions[i].key).toString();
			if (actions[i].times > 0)
			{
				message = message + QString(" (x") + QString::number(actions[i].times + 1) + QString(")");
			}
			message = message + QString("\": ")
				+ actions[i].actionName;
			if (i + 1 < actions.size())
			{
				message = message + QString(", ");
			}
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

ActionStruct::ActionStruct(const QString& actionName, const QString& actionHint, ActionTypelessFnPtr doFn, ActionTypelessFnPtr undoFn, bool isTypeSpecific, Clipboard::DataType acceptedType) :
	actionName(actionName),
	actionHint(actionHint),
	doFn(doFn),
	undoFn(undoFn),
	doTypedFn(),
	undoTypedFn(),
	isTypeSpecific(isTypeSpecific),
	isShortcut(false)
{
	addAcceptedDataType(acceptedType);
}

ActionStruct::ActionStruct(const QString& actionName, const QString& actionHint, ActionSafeFnPtr doFn, ActionSafeFnPtr undoFn, bool isTypeSpecific, Clipboard::DataType acceptedType) :
	actionName(actionName),
	actionHint(actionHint),
	doFn(nullptr),
	undoFn(nullptr),
	doTypedFn(doFn),
	undoTypedFn(undoFn),
	isTypeSpecific(isTypeSpecific),
	isShortcut(false)
{
	addAcceptedDataType(acceptedType);
}

void ActionStruct::setShortcut(Qt::Key shortcutKey, Qt::KeyboardModifier shortcutModifier, size_t shortcutTimes, bool isShortcutLoop)
{
	isShortcut = true;
	key = shortcutKey;
	modifier = shortcutModifier;
	times = shortcutTimes;
	isLoop = isShortcutLoop;
}

void ActionStruct::addAcceptedDataType(Clipboard::DataType type)
{
	if (type == Clipboard::DataType::Error) { return; }
	acceptedType.push_back(type);
}

void ActionStruct::resetShortcut()
{
	isShortcut = false;
	key = Qt::Key_F35;
	modifier = Qt::NoModifier;
	times = 0;
	isLoop = false;
}

bool ActionStruct::doesShortcutMatch(QKeyEvent* event) const
{
	// if shortcut key == event key and the shortcut modifier can be found inside event modifiers or there is no modifier
	return isShortcut && key == event->key() && ((event->modifiers() & modifier)
		|| (event->nativeModifiers() <= 0 && modifier == Qt::NoModifier));
}

bool ActionStruct::doesShortcutMatch(const ActionStruct& otherShortcut) const
{
	return isShortcut && key == otherShortcut.key && (modifier & otherShortcut.modifier) && times == otherShortcut.times;
}

bool ActionStruct::doesFullShortcutMatch(const ActionStruct& otherShortcut) const
{
	return isShortcut && key == otherShortcut.key && (modifier & otherShortcut.modifier) && times == otherShortcut.times && isLoop == otherShortcut.isLoop;
}

bool ActionStruct::doesTypeMatch(Clipboard::DataType type) const
{
	for (Clipboard::DataType curType : acceptedType)
	{
		if (curType == type) { return true; }
	}
	return false;
}

bool ActionStruct::isTypeAccepted(Clipboard::DataType type) const
{
	for (Clipboard::DataType curType : acceptedType)
	{
		if (curType == Clipboard::DataType::Any || curType == type) { return true; }
	}
	return false;
}

bool ActionStruct::doesActionMatch(const ActionStruct& otherAction) const
{
}

} // namespace lmms::gui
