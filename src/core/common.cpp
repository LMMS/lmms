/*
 * common.cpp
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

#include "core/common.h"

#ifndef NDEBUG

/* mimic the output of a raised QT_ASSERT_X */
void assert_error(const char *where, const char *what, const char *file, int line) noexcept
{
    std::cerr << "ASSERT failure in " << where << ": "
		<< '"' << what << "\", "
		<< "file " << file << ", "
		<< "line " << line << std::endl;
}

#endif /* NDEBUG */
