/*
 * BarModelEditor.h - edit model values using a bar display
 *
 * Copyright (c) 2023-now Michael Gregorius
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

#pragma once

#ifndef LMMS_GUI_BAR_MODEL_EDITOR_H
#define LMMS_GUI_BAR_MODEL_EDITOR_H

#include "FloatModelEditorBase.h"


namespace lmms::gui
{

class LMMS_EXPORT BarModelEditor : public FloatModelEditorBase
{
	Q_OBJECT

public:
	Q_PROPERTY(QBrush backgroundBrush READ getBackgroundBrush WRITE setBackgroundBrush)
	Q_PROPERTY(QBrush barBrush READ getBarBrush WRITE setBarBrush)
	Q_PROPERTY(QColor textColor READ getTextColor WRITE setTextColor)

	BarModelEditor(QString text, FloatModel * floatModel, QWidget * parent = nullptr);

	// Define how the widget will behave in a layout
	QSizePolicy sizePolicy() const;

	QSize minimumSizeHint() const override;

	QSize sizeHint() const override;

	QBrush const & getBackgroundBrush() const;
	void setBackgroundBrush(QBrush const & backgroundBrush);

	QBrush const & getBarBrush() const;
	void setBarBrush(QBrush const & barBrush);

	QColor const & getTextColor() const;
	void setTextColor(QColor const & textColor);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	QString const m_text;

	QBrush m_backgroundBrush;
	QBrush m_barBrush;
	QColor m_textColor;
};

} // namespace lmms::gui

#endif // LMMS_GUI_BAR_MODEL_EDITOR_H
