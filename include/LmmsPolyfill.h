/*
 * LmmsPolyfill.h - Small helpers and stand-in for newer C++ features
 *
 * Copyright (c) 2025 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_POLYFILL_H
#define LMMS_POLYFILL_H

namespace lmms
{

/**
 * Stand-in for C++23's std::unreachable
 * Taken from https://en.cppreference.com/w/cpp/utility/unreachable
 *
 * TODO C++23: Use std::unreachable instead
 */
[[noreturn]] inline void unreachable()
{
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
	__assume(false);
#else // GCC, Clang
	__builtin_unreachable();
#endif
}


/**
 * Can be used with static_assert() in an uninstantiated template
 * as a workaround for static_assert(false)
 *
 * TODO C++23: No longer needed with resolution of CWG2518
 */
template<class... T>
inline constexpr bool always_false_v = false;

} // namespace lmms

#endif // LMMS_POLYFILL_H
