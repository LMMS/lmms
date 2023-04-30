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

#include <Qt>

#include "lmms_export.h"

class QWheelEvent;


namespace lmms {


/*! \brief Mark event as ignored and return true if it matches the orientation
 *
 *  Convenience function that may be used for early return in wheelEvent.
 *  Events that doesn't match the orientation are marked accepted.
 */
bool LMMS_EXPORT ignoreScroll(const Qt::Orientation orientation, QWheelEvent* event);



/*! \brief Return number of scrolled steps.
 *
 *  By default it counts standard scroll wheel steps of 15°.
 *
 *  If you intend to round or divide WheelEvent::angleDelta() this function should ALWAYS be used to get proper
 *  support for smooth scrolling mice and trackpads.
 *
 *  Only call this function ONCE per event and orientation. Never call it if the event will be ignored.
 *
 *  \param event - QWheelEvent to get delta value from.
 *  \param orientation - Vertical or horizontal.
 *  \param factor - Scroll speed/precision. If factor=2 it returns 2 for a complete step and 1 for a halfstep.
 *  \param allowNatural - Whether macOS-style natural scroll should be allowed or inverted to regular scroll.
 */
int LMMS_EXPORT getScroll(const QWheelEvent* event, const Qt::Orientation orientation, const float factor,
	const bool allowNatural);


/*! \brief Overload of getScroll
 *
 * Returns a positive value if the top of the wheel is moved to the left
 */
int LMMS_EXPORT horizontalScroll(const QWheelEvent* event, const float factor = 1, const bool allowNatural = true);


/*! \brief Overload of getScroll
 *
 *  Returns a positive value if the top of the wheel is rotating away from the hand operating it
 */
int LMMS_EXPORT verticalScroll(const QWheelEvent* event, const float factor = 1, const bool allowNatural = true);


} // namespace lmms

#endif
