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
}

void SimpleTextFloat::setText(const QString & text)
{
	m_textLabel->setText(text);
}

void SimpleTextFloat::showWithDelay(int msecBeforeDisplay, int msecDisplayTime)
{
	auto _ = std::lock_guard{m_mutex};

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
	auto _ = std::lock_guard{m_mutex};

	m_hideTimer->start();

	if (!isVisible())
	{
		emit visibilityChanged(true);
	}

	QWidget::show();
}

void SimpleTextFloat::hide()
{
	auto _ = std::lock_guard{m_mutex};

	m_showTimer->stop();
	m_hideTimer->stop();

	if (isVisible())
	{
		emit visibilityChanged(false);
	}

	QWidget::hide();
}

void SimpleTextFloat::setSource(QObject* source)
{
	auto _ = std::lock_guard{m_mutex};

	disconnect(m_connection);
	m_source = source;
	m_connection = {};
}

void SimpleTextFloat::setSource(QObject* source, QMetaObject::Connection connection)
{
	auto _ = std::lock_guard{m_mutex};

	if (source != m_source || !source)
	{
		disconnect(m_connection);
		m_connection = {};
	}

	if (connection)
	{
		disconnect(m_connection);
		m_connection = connection;
	}

	m_source = source;
}

} // namespace lmms::gui
