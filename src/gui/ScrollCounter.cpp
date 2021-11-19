/*
 * ScrollCounter.cpp - helper to handle smooth scroll on widgets
 *
 * Copyright (c) 2021 Alex <allejok96/gmail>
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

#include "ScrollCounter.h"

#include <QWheelEvent>
#include <QWidget>


ScrollCounter* ScrollCounter::s_instance = nullptr;


/*! \brief High-resolution scroll management
 *
 * First resister the widget you wish to manage with registerWidget().
 *
 * Use getStepsX or getStepsY from within QWidget::wheelEvent to pop a value from the scroll counter.
 * If the counter is not checked (e.g. the widget choose to ignore the event) it will not be increased.
 *
 * Since the counter relies on an event filter, you cannot call QWidget::wheelEvent directly,
 * as this would bypass the filter. Instead use QCoreApplication::sendEvent.
 */
ScrollCounter::ScrollCounter()
{
}




//! Monitor scroll events of given widget
void ScrollCounter::registerWidget(QWidget* widget)
{
	// Create global instance
	if (s_instance == nullptr) { s_instance = new ScrollCounter(); }

	widget->installEventFilter(s_instance);
}



//! Return number of horizontally scrolled steps of size stepSize (120 = 15°)
int ScrollCounter::getStepsX(float stepSize)
{
	/* Only return a value if the last wheel event had this direction. Why? Imagine this scenario:
	 *
	 *   if (controlPressed)
	 *		zoom(counter->getStepsY(120));
	 *   else
	 *		scrollWindow(counter->getStepsX(1), conter->getStepsY(1));
	 *
	 * In this example, the first statement has a higher stepSize (threshold if you will) than the second.
	 *
	 * If the user presses control and scrolls 100 units VERTICALLY, it will not be enough to hit the threshold,
	 * and the value will be stored for later. If the user then releases control and scrolls HORIZONTALLY, the code
	 * would return the previous 100 units due to the much lower threshold and thus ALSO scroll the window VERTICALLY.
	 */

	if (s_instance == nullptr || s_instance->m_lastScroll.x() == 0) { return 0; }

	int sum = s_instance->m_lastScroll.x() + s_instance->m_remainder.x();
	int quotient = sum / stepSize;
	s_instance->m_remainder.setX(sum - quotient * stepSize);
	return quotient;
}



//! Return number of vertically scrolled steps of size stepSize (120 = 15°)
int ScrollCounter::getStepsY(float stepSize)
{
	if (s_instance == nullptr || s_instance->m_lastScroll.y() == 0) { return 0; }

	int sum = s_instance->m_lastScroll.y() + s_instance->m_remainder.y();
	int quotient = sum / stepSize;
	s_instance->m_remainder.setY(sum - quotient * stepSize);
	return quotient;
}




bool ScrollCounter::eventFilter(QObject *watchedObject, QEvent *event)
{
	if (event->type() == QEvent::Leave)
	{
		// Reset counter when mouse leaves the widget
		m_remainder.setX(0);
		m_remainder.setY(0);
	}
	else if (event->type() == QEvent::Wheel)
	{
		// Remember the last seen wheelEvent
		// in case we get a call in a few secs
		m_lastScroll = static_cast<QWheelEvent*>(event)->angleDelta();
	}
	return QObject::eventFilter(watchedObject, event);
}
