/*
 * Scroll.h - calculate scroll distance
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

#ifndef SCROLL_H
#define SCROLL_H

#include "Flags.h"
#include "lmms_export.h"


class QWheelEvent;


namespace lmms::gui {


class LMMS_EXPORT Scroll
{
public:
	//! QWheelEvent->angleDelta() that corresponds to a "wheel tick"
	static constexpr float ANGLE_DELTA_PER_TICK = 120;
	//! Default scroll speed for various editors
	static constexpr int PIXELS_PER_STEP = 36;

	enum class Flag {
		NoFlag = 0x0,
		Horizontal = 0x1,
		//! Change any natural (inverted) scroll to regular scroll.
		//! This is useful for widgets like faders, where you want up to be up.
		//! Some operating systems does not support this.
		DisableNaturalScrolling = 0x2,
		//! Swap x/y scroll orientation when pressing Shift or Alt.
		//! Most software uses Shift for this, but Qt uses Alt by default
		//! so it is included here to match QScrollBar and other widgets.
		SwapWithShiftOrAlt = 0x4,
	};

	using Flags = lmms::Flags<Flag>;


	/*! \brief Scroll delta
	 *
	 *  This class measures scroll delta in "wheel ticks",
	 *  unlike QWheelEvent which measures scroll delta in 1/8ths of a degree.
	 */
	Scroll(QWheelEvent* event);


	/*! \brief Return number of completely scrolled steps of some size
	 *
	 *  The return value is positive when the wheel is rotated away from the hand.
	 *
	 *  `stepsPerWheelTick` is the number of steps to count for every wheel tick.
	 *  If set to 5 it will count a step whenever the wheel has moved 1/5 of a tick.
	 *  It will always cound at least one step per wheel tick.
	 *
	 *  --------------------------------------------------------------------------
	 *
	 *  You should always use this function instead of the following:
	 * 	    int steps = wheelEvent->angleDelta().y() / some_value
	 *
	 * 	This is because some trackpads and mice report much smaller chunks of angleDelta
	 *  than the standard 120 (which is a "wheel tick"). In the worst case scenario
	 *  the result will always be rounded down to 0. This function solves it by accumulating
	 *  the angleDelta until a complete step is reached.
	 *
	 *  Note: don't call this function if you intend to ignore() the event, as it
	 *  may result in double-counting scroll delta.
	 */
	int getSteps(const float stepsPerWheelTick = 1.0, const Flags flags = Flag::NoFlag);

	inline int getSteps(const Flags flags)
	{
		return getSteps(1.0, flags);
	}


	/*! \brief Return number of scrolled "wheel ticks"
	 *
	 *  The value is positive when the wheel is rotated away from the hand.
	 *
	 *  If you intend to use this in a calculation where the end result is an integer,
	 *  you should probably use getSteps() instead to avoid rounding issues with
	 *  smooth scrolling trackpads and mice.
	 */
	inline float getStepsFloat(const Flags flags)
	{
		return getAngleDelta(flags) / ANGLE_DELTA_PER_TICK;
	}

	//! \brief True when scrolling vertically
	bool isVertical();

	//! \brief True when scrolling horizontally
	bool isHorizontal();

private:
	int getAngleDelta(const Flags flags = Flag::NoFlag);
	int calculateSteps(const int angleDelta, const float stepsPerTick, const bool horizontal);

	QWheelEvent* m_event;

	// These are used by calculateSteps() to accumulate partially scrolled steps
	// They are shared across all widgets but that doesn't notisable affect the user experience
	static float s_partialStepX;
	static float s_partialStepY;

	const float m_initialPartialStepX;
	const float m_initialPartialStepY;
};

LMMS_DECLARE_OPERATORS_FOR_FLAGS(Scroll::Flag)


} // namespace lmms::gui

#endif
