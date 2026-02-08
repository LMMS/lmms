/*
 * TextFloat.h - class textFloat, a floating text-label
 *
 * Copyright (c) 2023 LMMS team
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


#ifndef LMMS_GUI_SIMPLE_TEXT_FLOAT_H
#define LMMS_GUI_SIMPLE_TEXT_FLOAT_H

#include <QTimer>
#include <QWidget>

#include "lmms_export.h"

class QLabel;

namespace lmms::gui
{

class LMMS_EXPORT SimpleTextFloat : public QWidget
{
	Q_OBJECT
public:
	SimpleTextFloat();
	~SimpleTextFloat() override = default;

	void setText(const QString & text);

	void showWithDelay(int msecBeforeDisplay, int msecDisplayTime);

	void showWithTimeout(int msec)
	{
		showWithDelay(0, msec);
	}

	void moveGlobal(QWidget * w, const QPoint & offset)
	{
		move(w->mapToGlobal(QPoint(0, 0)) + offset);
	}

	void show();
	void hide();

	void setRefreshRate(int timesPerSecond);

	template<class T>
	void setRefreshConnection(T* receiver, void(T::* slot)(), Qt::ConnectionType type = Qt::AutoConnection)
	{
		if (auto connection = m_refreshTimer->callOnTimeout(receiver, slot,
			static_cast<Qt::ConnectionType>(type | Qt::UniqueConnection)))
		{
			m_textUpdateConnection = connection;
		}
	}

private:
	QLabel * m_textLabel;
	QTimer * m_showTimer;
	QTimer * m_hideTimer;
	QTimer* m_refreshTimer;

	QMetaObject::Connection m_textUpdateConnection;
};

} // namespace lmms::gui

#endif // LMMS_GUI_SIMPLE_TEXT_FLOAT_H
