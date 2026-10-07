/*
 * LmmsErrors.h
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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

#ifndef LMMS_ERRORS_H
#define LMMS_ERRORS_H

#include <string_view>

namespace lmms {

enum class Error
{
    None,
    SampleNotFound,
    SampleDecodeFailure
};

constexpr auto errorMessage(Error error) -> std::string_view
{
    switch (error)
    {
	case Error::None:
        return "Success";
	case Error::SampleNotFound:
        return "Sample not found";
	case Error::SampleDecodeFailure:
        return "Failed to decode sample";
	}
}

} // namespace lmms

#endif // LMMS_ERRORS_H