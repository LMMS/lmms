/*
 * TextFloat.h - class textFloat, a floating text-label
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_TEXT_FLOAT_H
#define LMMS_GUI_TEXT_FLOAT_H

#include <QWidget>

#include "lmms_export.h"

class QLabel;

namespace lmms::gui
{

class LMMS_EXPORT TextFloat : public QWidget
{
	Q_OBJECT
public:
	TextFloat();
	~TextFloat() override = default;

	void setTitle(const QString & title);
	void setText(const QString & text);
	void setPixmap(const QPixmap & pixmap);

	void setVisibilityTimeOut(int msecs);

	static TextFloat * displayMessage(const QString & title,
						const QString & msg,
						const QPixmap & pixmap = QPixmap(),
						int timeout = 2000,
						QWidget * parent = nullptr);

	void moveGlobal(QWidget * w, const QPoint & offset)
	{
		move(w->mapToGlobal(QPoint(0, 0)) + offset);
	}


protected:
	void mousePressEvent(QMouseEvent * me) override;


private:
	TextFloat(const QString & title, const QString & text, const QPixmap & pixmap);

	QLabel * m_pixmapLabel;
	QLabel * m_titleLabel;
	QLabel * m_textLabel;

};


} // namespace lmms::gui

#endif // LMMS_GUI_TEXT_FLOAT_H
