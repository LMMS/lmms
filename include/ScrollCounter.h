/*
 * ScrollCounter.h - helper to handle smooth scroll on widgets
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

#ifndef SCROLL_COUNTER_H
#define SCROLL_COUNTER_H

#include <QObject>
#include <QPoint>

#include "lmms_export.h"


class LMMS_EXPORT ScrollCounter: public QObject
{
public:
	static int getStepsX(float stepSize = 120);
	static int getStepsY(float stepSize = 120);
	static void registerWidget(QWidget* widget);

	bool eventFilter(QObject *watched, QEvent *event) override;

private:
	ScrollCounter();

	static ScrollCounter* s_instance;

	QPoint m_lastScroll;
	QPoint m_remainder;
};

#endif
