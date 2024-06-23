/*
 * NoCopyNoMove.h - NoCopyNoMove class
 *
 * Copyright (c) 2023-2023 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_NOCOPYNOMOVE_H
#define LMMS_NOCOPYNOMOVE_H

namespace lmms
{

/**
 * Inherit this class to make your class non-copyable and non-movable
 */
class NoCopyNoMove
{
protected:
	NoCopyNoMove() = default;
	NoCopyNoMove(const NoCopyNoMove& other) = delete;
	NoCopyNoMove& operator=(const NoCopyNoMove& other) = delete;
	NoCopyNoMove(NoCopyNoMove&& other) = delete;
	NoCopyNoMove& operator=(NoCopyNoMove&& other) = delete;
};

} // namespace lmms

#endif // LMMS_NOCOPYNOMOVE_H

