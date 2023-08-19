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


int getAngleDelta(ScrollFlags options, QWheelEvent* event)
{
	bool getX = options & HorizontalScroll;
#ifndef LMMS_BUILD_APPLE
	if (options & CustomAltModifierScroll && event->modifiers() & Qt::AltModifier)
	{
		// Qt inverts X and Y when holding Alt on Windows/Linux - here we invert it back
		getX = !getX;
	}
#endif
	return getX ? event->angleDelta().x() : event->angleDelta().y();
}




bool ignoreScroll(ScrollFlags options, QWheelEvent* event)
{
	bool hasOtherOrientation = getAngleDelta(options ^ HorizontalScroll, event) != 0;
	event->setAccepted(hasOtherOrientation);
	return !hasOtherOrientation;
}




bool hasScroll(ScrollFlags options, QWheelEvent* event)
{
	return getAngleDelta(options, event) != 0;
}





int getScroll(ScrollFlags options, QWheelEvent* event, const float factor)
{
	/* TODO: is there a good way to prevent calling this method multiple times with the same event and orientation?
	 *
	 * for (auto child: children)
	 * {
	 *     child->move(getScroll(ev));
	 * }
	 *
	 * Here the internal yRemainder will be increased by angleDelta().y() for every child until it reaches a full step,
	 * whereby getScroll() will return non-zero for that child only. For regular mice angleDelta() is always a full step
	 * so the bug will go unnoticed, but for many trackpads this won't work.
	 */
	static int xRemainder;
	static int yRemainder;

	int& remainder = options & HorizontalScroll ? xRemainder : yRemainder;
	int delta = getAngleDelta(options, event);

	if (event->inverted() && !(options & AllowNaturalScroll))
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




int getScroll(QWheelEvent* event, const float factor)
{
	return getScroll(VerticalScroll, event, factor);
}


} // namespace lmms
