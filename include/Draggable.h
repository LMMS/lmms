/*
 * Draggable.h
 *
 * Copyright (c) 2025 Lost Robot <r94231/at/gmail/dot/com>
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
 
#ifndef LMMS_GUI_DRAGGABLE_H
#define LMMS_GUI_DRAGGABLE_H

#include "FloatModelEditorBase.h"

namespace lmms::gui
{

/**
 * @brief A pixmap that can be dragged from one location to another to control a FloatModel
 */
class LMMS_EXPORT Draggable : public FloatModelEditorBase
{
	Q_OBJECT

public:
	using FloatModelEditorBase::DirectionOfManipulation;
	
	/**
	 * @param directionOfManipulation Direction the user can drag the control
	 * @param floatModel Pointer to the FloatModel this draggable modifies
	 * @param pixmap The pixmap shown for this handle
	 * @param pointA Starting pixel position (Y if vertical, X if horizontal)
	 * @param pointB Ending pixel position (Y if vertical, X if horizontal)
	 * @param parent Parent QWidget
	 */
	Draggable(DirectionOfManipulation directionOfManipulation,
		FloatModel* floatModel, const QPixmap& pixmap, int pointA, int pointB, QWidget* parent = nullptr);

	QSize sizeHint() const override;
	void setPixmap(const QPixmap& pixmap);
	void setDefaultValPixmap(const QPixmap& pixmap, float value = 0.f);

protected:
	void paintEvent(QPaintEvent* event) override;
	void mouseMoveEvent(QMouseEvent* me) override;

protected slots:
	void handleMovement();

private:
	QPixmap m_pixmap;
	QPixmap m_defaultValPixmap;
	float m_pointA;
	float m_pointB;
	float m_defaultValue;
	bool m_hasDefaultValPixmap;
};

} // namespace lmms::gui

#endif // LMMS_GUI_DRAGGABLE_H
