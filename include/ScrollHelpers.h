/*
 * ScrollHelpers.h - helper functions for wheel events
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

#ifndef SCROLL_HELPERS_H
#define SCROLL_HELPERS_H

#include <QFlags>

#include "lmms_export.h"

class QWheelEvent;


namespace lmms {

enum ScrollFlag
{
	//! Default orientation - placeholder value.
	VerticalScroll = 0,

	//! Use horizontal orientation INSTEAD of vertical.
	//! Values will be positive if the finger is moving to the left.
	HorizontalScroll = 1 << 1,

	//! Pass-through natural (reversed) scroll on macOS.
	//! By default natural scroll will be inverted to normal scroll.
	AllowNaturalScroll = 1 << 2,

	//! Deactivate Qt's built-in Alt modifier behavior.
	//! By default Alt on Windows/Linux will swap scroll orientation.
	CustomAltModifierScroll = 1 << 3,
};

Q_DECLARE_FLAGS(ScrollFlags, ScrollFlag);
Q_DECLARE_OPERATORS_FOR_FLAGS(ScrollFlags);


/*! \brief If event matches orientation, ignore() it and return true.
 *
 *  Convenience function for early return in wheelEvent().
 */
bool LMMS_EXPORT ignoreScroll(ScrollFlags options, QWheelEvent* event);


/*! \brief Return true if event matches given orientation
 *
 *  Convenience function. Especially useful for CustomAltModifierScroll.
 */
bool LMMS_EXPORT hasScroll(ScrollFlags options, QWheelEvent* event);


/*! \brief Return number of scrolled steps.
 *
 *  This function should ALWAYS be used to get proper support for smooth scrolling mice and trackpads.
 *  Only call this function ONCE per event and orientation. Never call it on events that will be ignored.
 *
 *  For vertical orientation (default), the return value is positive if the finger moving forward.
 *
 *  If factor=1 it counts number of completed 15° scroll wheel steps. If factor=2 it counts halfsteps, and so on.
 *
 *  \param options - see ScrollFlag
 *  \param event - QWheelEvent
 *  \param factor - speed/precision
 */
int LMMS_EXPORT getScroll(ScrollFlags options, QWheelEvent* event, const float factor = 1);
int LMMS_EXPORT getScroll(QWheelEvent* event, const float factor = 1);


} // namespace lmms

#endif
