/*
 * common.h
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

#ifndef LMMS_CORE_COMMON_H
#define LMMS_CORE_COMMON_H

#include "lmms_export.h"
#include <iostream>

namespace lmms {

template <typename T>
void lmms_warning(T arg)
{
    std::cerr << arg << std::endl;
}

template <typename T, typename... Args>
void lmms_warning(T arg, Args... args)
{
    std::cerr << arg;
    // last recursive call should print with std::endl
    lmms_warning(args...);
}

} /* namespace lmms */

#define UNUSED_ARG(x) (void)x

#ifndef NDEBUG

/* copy how qt_assert_x is defined */
void assert_error(const char *where, const char *what, const char *file, int line) noexcept;

/* copy how QT_ASSERT_X works */
#define LMMS_ASSERT(cond, where, what) ((cond) ? static_cast<void>(0) : assert_error(where, what, __FILE__, __LINE__))

#else /* NDEBUG */

#define LMMS_ASSERT(cond, where, what)

#endif /* NDEBUG */

#endif /* CORE_COMMON_H */
