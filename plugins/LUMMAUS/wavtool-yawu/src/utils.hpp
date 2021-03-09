/*
    wavtool-yawu
    Copyright (C) 2015 StarBrilliant <m13253@hotmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3.0 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program.  If not,
    see <http://www.gnu.org/licenses/>.
*/

#ifndef YAWU_UTILS_HPP
#define YAWU_UTILS_HPP

#include <cstring>
#include <stdexcept>

namespace YAWU {

/**
 * Mark unused arguments to avoid compiler warnings
 *
 * Usage:
 *   int func(int a) {
 *       unused_arg(a);
 *       return 42;
 *   }
 */
template <typename T>
static inline void unused_arg(const T &arg) {
    static_cast<void>(arg);
}

/**
 * Clamp value in range [a, b]
 */
template <typename T>
static inline T clamp(T value, T a, T b) {
    return a < b ?
        value < a ? a : b < value ? b : value :
        value < b ? b : a < value ? a : value;
}

/**
 * The exception that strtonum throws to indicate an error
 */
class StrToNumError : public std::runtime_error {
public:
    StrToNumError() : std::runtime_error("Invalid number format") {}
};

/**
 * A wrapper to C-style strtol function family
 * Usage:
 *   long result1 = strtonum(std::strtol, "42", 10);
 *   double result2 = strtonum(std::strtod, "0.618");
 * Throws:
 *   StrToNumError, when input is not a valid number
 */
template <typename Fn, typename ...Args>
static inline auto strtonum(Fn fn, const char *str, Args &&...args) -> decltype(fn(str, nullptr, std::forward<Args>(args)...)) {
    if(str[0] != '\0') {
        char *endptr;
        auto result = fn(str, &endptr, std::forward<Args>(args)...);
        if(endptr == &str[std::strlen(str)]) {
            return result;
        } else {
            throw StrToNumError();
        }
    } else {
        throw StrToNumError();
    }
}

}

#endif
