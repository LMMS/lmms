/*
 * ScrollHelpers.cpp - helper functions for wheel events
 *
 * Copyright (c) 2023 Alex <allejok96/gmail>
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

#include "ScrollHelpers.h"

#include <QWheelEvent>


namespace lmms {


bool ignoreScroll(const Qt::Orientation orientation, QWheelEvent* event)
{
	bool hasOtherOrientation = orientation == Qt::Horizontal ? event->angleDelta().y() : event->angleDelta().x();
	event->setAccepted(hasOtherOrientation);
	return !hasOtherOrientation;
}




// TODO: is there a good way to prevent calling this method multiple times with the same event?
int getScroll(const QWheelEvent* event, const Qt::Orientation orientation, const float factor, const bool allowNatural)
{
	static int xRemainder;
	static int yRemainder;

	int& remainder = orientation == Qt::Horizontal ? xRemainder : yRemainder;

	int delta = orientation == Qt::Horizontal ? event->angleDelta().x() : event->angleDelta().y();
	if (event->inverted() && !allowNatural)
	{
		delta = -delta;
	}

	// If the wheel changed direction forget the accumulated value
	if (delta * remainder < 0) { remainder = 0; }

	// A normal scroll wheel click is 15° and Qt counts angle delta in 1/8 of a degree
	const float deltaPerWheelTick = 120;
	// Angle delta needed to scroll one step (never more than 15°)
	const float deltaPerStep = deltaPerWheelTick / std::max(1.0f, factor);

	// Summarize, return whole steps and keep what's left
	remainder += delta;
	int steps = remainder / deltaPerStep;
	remainder -= steps * deltaPerStep;

	return steps;
}




int horizontalScroll(const QWheelEvent* event, const float factor, const bool allowNatural)
{
	return getScroll(event, Qt::Horizontal, factor, allowNatural);
}




int verticalScroll(const QWheelEvent* event, const float factor, const bool allowNatural)
{
	return getScroll(event, Qt::Vertical, factor, allowNatural);
}


} // namespace lmms
