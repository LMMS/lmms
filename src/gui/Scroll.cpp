/*
 * Scroll.cpp - calculate scroll distance
 *
 * Copyright (c) 2025 Alex <allejok96/gmail>
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

#include "Scroll.h"

#include <QGraphicsSceneWheelEvent>
#include <QWheelEvent>


namespace lmms::gui {


float Scroll::s_partialStepX = 0;
float Scroll::s_partialStepY = 0;



Scroll::Scroll(QWheelEvent* event) :
	m_event(event),
	m_initialPartialStepX(s_partialStepX),
	m_initialPartialStepY(s_partialStepY)
{
}



int Scroll::getAngleDelta(const Flags flags)
{
	bool useHorizontal = flags.testFlag(Scroll::Flag::Horizontal);

#ifdef LMMS_BUILD_APPLE
	const auto swapModifiers = Qt::ShiftModifier;
#else
	const auto swapModifiers = Qt::ShiftModifier|Qt::AltModifier;
#endif

	// Swap x/y when shift (or alt) is pressed
	if (m_event->modifiers() & swapModifiers && flags & Scroll::Flag::SwapWithShiftOrAlt)
	{
		useHorizontal = !useHorizontal;
	}

	// Compensate for Qt swapping x/y when Alt is pressed on Windows and Linux
	if (m_event->modifiers() & Qt::AltModifier)
	{
#ifndef LMMS_BUILD_APPLE
		useHorizontal = !useHorizontal;
#endif
	}

	int delta = useHorizontal ? m_event->angleDelta().x() : m_event->angleDelta().y();

	// Compensate natural scrolling
	if (m_event->inverted() && flags & Scroll::Flag::DisableNaturalScrolling)
	{
		delta = -delta;
	}

	return delta;
}



int Scroll::getSteps(const float stepsPerWheelTick, const Flags flags)
{
	return calculateSteps(getAngleDelta(flags), stepsPerWheelTick, flags.testFlag(Flag::Horizontal));
}



bool Scroll::isVertical()
{
	return getAngleDelta() != 0;
}



bool Scroll::isHorizontal()
{
	return getAngleDelta(Flag::Horizontal) != 0;
}



int Scroll::calculateSteps(const int delta, const float stepsPerTick, const bool horizontal)
{
	// Partial step saved from another wheel event
	float& partialStep = horizontal ? s_partialStepX : s_partialStepY;

	// Prevent partial steps from building up if this is called multiple times for the same event
	partialStep = horizontal ? m_initialPartialStepX : m_initialPartialStepY;

	// If scroll changed direction, forget the partial step
	if (delta * partialStep < 0) { partialStep = 0; }

	const float steps = partialStep + (delta / ANGLE_DELTA_PER_TICK) * std::max(1.f, stepsPerTick);
	const int wholeSteps = static_cast<int>(steps);

	partialStep = (steps - wholeSteps);

	return wholeSteps;
}



} // namespace lmms::gui
