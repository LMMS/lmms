/*
 * TextFloat.cpp - class textFloat, a floating text-label
 *
 * Copyright (c) LMMS team
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

#include "SimpleTextFloat.h"

#include <QTimer>
#include <QHBoxLayout>
#include <QLabel>

#include "GuiApplication.h"
#include "MainWindow.h"

namespace lmms::gui
{


SimpleTextFloat::SimpleTextFloat() :
	QWidget(getGUI()->mainWindow(), Qt::ToolTip)
{
	QHBoxLayout * layout = new QHBoxLayout(this);
	layout->setContentsMargins(3, 3, 3, 3);
	setLayout(layout);

	m_textLabel = new QLabel(this);
	layout->addWidget(m_textLabel);

	m_showTimer = new QTimer(this);
	m_showTimer->setSingleShot(true);
	QObject::connect(m_showTimer, &QTimer::timeout, this, &SimpleTextFloat::show);

	m_hideTimer = new QTimer(this);
	m_hideTimer->setSingleShot(true);
	QObject::connect(m_hideTimer, &QTimer::timeout, this, &SimpleTextFloat::hide);

	m_refreshTimer = new QTimer(this);
}

void SimpleTextFloat::setText(const QString & text)
{
	m_textLabel->setText(text);
}

void SimpleTextFloat::showWithDelay(int msecBeforeDisplay, int msecDisplayTime)
{
	if (msecBeforeDisplay > 0)
	{
		m_showTimer->start(msecBeforeDisplay);
	}
	else
	{
		show();
	}

	if (msecDisplayTime > 0)
	{
		m_hideTimer->start(msecBeforeDisplay + msecDisplayTime);
	}
}

void SimpleTextFloat::show()
{
	if (m_refreshTimer->interval() > 0 && !m_refreshTimer->isActive())
	{
		// Emit timeout signal once at very start
		const auto interval = m_refreshTimer->interval();
		m_refreshTimer->setSingleShot(true);
		m_refreshTimer->start(0);
		m_refreshTimer->setSingleShot(false);
		m_refreshTimer->setInterval(interval);

		// Now start timer normally
		m_refreshTimer->start();
	}

	QWidget::show();
}

void SimpleTextFloat::hide()
{
	disconnect(m_textUpdateConnection);
	m_showTimer->stop();
	m_hideTimer->stop();
	m_refreshTimer->stop();
	QWidget::hide();
}

void SimpleTextFloat::setRefreshRate(int timesPerSecond)
{
	if (timesPerSecond > 0)
	{
		m_refreshTimer->setInterval(1000 / timesPerSecond);
	}
	else
	{
		m_refreshTimer->stop();
		m_refreshTimer->setInterval(0);
	}
}

} // namespace lmms::gui
