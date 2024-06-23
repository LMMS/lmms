/*
 * ToolButton.h - declaration of class toolButton
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_TOOL_BUTTON_H
#define LMMS_GUI_TOOL_BUTTON_H

#include <QToolButton>

namespace lmms::gui
{

class ToolButton : public QToolButton
{
	Q_OBJECT
public:
	ToolButton(const QPixmap & _pixmap, const QString & _tooltip,
			QObject * _receiver=nullptr, const char * _slot=nullptr,
			QWidget * _parent=nullptr);

	inline ToolButton(QWidget * _parent) :
		QToolButton(_parent)
	{ }

	~ToolButton() override = default;

} ;

} // namespace lmms::gui

#endif // LMMS_GUI_TOOL_BUTTON_H
