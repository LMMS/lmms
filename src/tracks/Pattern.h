/*
 * Pattern.h
 *
 * Copyright (c) [Year] [Author]
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
 */

#ifndef PATTERN_H
#define PATTERN_H

#include <cstdint>

namespace lmms {

class Pattern {
public:
    static constexpr std::size_t MIN_NOTE_LENGTH = 1;

    // ... existing code ...

    void setNoteLength(std::size_t length) {
        if (length == 0) {
            // Handle zero-length note length
            // For example, throw an exception or set a default value
            throw std::invalid_argument("Note length cannot be zero");
        }
        // ... existing code ...
    }

    // ... existing code ...

};

} // namespace lmms

#endif // PATTERN_H
