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

class BarModelEditor : public FloatModelEditorBase
{
public:
	BarModelEditor(QString text, FloatModel * floatModel, QWidget * parent = nullptr);

	// Define how the widget will behave in a layout
	QSizePolicy sizePolicy() const;

	virtual QSize minimumSizeHint() const override;

	virtual QSize sizeHint() const override;

protected:
	virtual void paintEvent(QPaintEvent *event) override;

private:
	QString const m_text;
};

} // namespace lmms::gui

#endif // LMMS_GUI_BAR_MODEL_EDITOR_H
