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
#include <QStyleOption>
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
	layout->setMargin(3);
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

void SimpleTextFloat::hide()
{
	m_showTimer->stop();
	m_hideTimer->stop();
	QWidget::hide();
}

} // namespace lmms::gui
