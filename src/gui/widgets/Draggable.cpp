/*
 * Draggable.cpp
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

#include "Draggable.h"
#include "SimpleTextFloat.h"
#include "embed.h"
#include "DpiContext.h"
#include "interpolation.h"

#include <QPixmap>
#include <QMouseEvent>
#include <QPainter>

namespace lmms::gui
{

Draggable::Draggable(DirectionOfManipulation directionOfManipulation, 
	FloatModel* floatModel, const QPixmap& pixmap, int pointA, int pointB, QWidget* parent)
	: FloatModelEditorBase(directionOfManipulation, parent),
	  m_pixmap(pixmap),
	  m_defaultValPixmap(),
	  m_pointA(pointA),
	  m_pointB(pointB),
	  m_defaultValue(0),
	  m_hasDefaultValPixmap(false)
{
	m_resource = embed::describePixmap(pixmap);
	Dpi::watch(this, [this] {
		const qreal dpr = Dpi::effectiveDevicePixelRatio(this);
		m_resource = m_resource.atDpr(dpr);
		m_defaultResource = m_defaultResource.atDpr(dpr);
		m_pixmap = m_resource.pixmap;
		m_defaultValPixmap = m_defaultResource.pixmap;
	});
	setModel(floatModel);
	connect(model(), &FloatModel::dataChanged, this, &Draggable::handleMovement);
	handleMovement();
}

QSize Draggable::sizeHint() const
{
	return m_resource.logicalSize;
}

void Draggable::setPixmap(const QPixmap& pixmap)
{
	m_resource = embed::describePixmap(pixmap);
	m_pixmap = pixmap;
	update();
}

void Draggable::setDefaultValPixmap(const QPixmap& pixmap, float value)
{
	m_defaultResource = embed::describePixmap(pixmap);
	m_defaultValPixmap = pixmap;
	m_defaultValue = value;
	m_hasDefaultValPixmap = true;
	update();
}

void Draggable::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	
	if (m_hasDefaultValPixmap && model()->value() == m_defaultValue)
	{
		painter.drawPixmap(QRect(QPoint(), m_defaultResource.logicalSize), m_defaultValPixmap, m_defaultValPixmap.rect());
	}
	else
	{
		painter.drawPixmap(QRect(QPoint(), m_resource.logicalSize), m_pixmap, m_pixmap.rect());
	}
}

void Draggable::mouseMoveEvent(QMouseEvent* me)
{
	updateInteractionState(me);

	QPoint pPos = mapToParent(me->pos());
	if (currentInteraction() == InteractionType::MouseDrag && pPos != m_lastMousePos)
	{
		float point = (directionOfManipulation() == DirectionOfManipulation::Vertical) ? pPos.y() : pPos.x();
		float progress = (point - m_pointA) / (m_pointB - m_pointA);

		if (progress >= 0 && progress <= 1)
		{
			float newVal = progress * (model()->maxValue() - model()->minValue()) + model()->minValue();
			model()->setValue(newVal);
		}
		else if (progress < 0)
		{
			model()->setValue(model()->minValue());
		}
		else
		{
			model()->setValue(model()->maxValue());
		}

		emit sliderMoved(model()->value());
		m_lastMousePos = pPos;

		showTextFloat();
	}
}

void Draggable::handleMovement()
{
	float newCoord = std::lerp(m_pointA, m_pointB, (model()->value() - model()->minValue()) / (model()->maxValue() - model()->minValue()));
	const QSize logicalSize = m_resource.logicalSize;
	if (directionOfManipulation() == DirectionOfManipulation::Vertical)
	{
		move(x(), newCoord - logicalSize.height() / 2.f);
	}
	else if (directionOfManipulation() == DirectionOfManipulation::Horizontal)
	{
		move(newCoord - logicalSize.width() / 2.f, y());
	}
}

} // namespace lmms::gui
