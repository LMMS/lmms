/*
 * LmmsPolyfill.h
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

#ifndef LMMS_POLYFILL_H
#define LMMS_POLYFILL_H

#include <variant>

namespace lmms {

template <typename T, typename E>
class Expected
{
public:
	constexpr explicit Expected(T expected)
		: m_value{std::move(expected)}
	{
	}

	constexpr explicit Expected(E unexpected)
		: m_value{std::move(unexpected)}
	{
	}

	operator bool() const { return std::holds_alternative<T>(m_value); }
    auto value() const -> const T& { return std::get<T>(m_value); }
	auto error() const -> const E& { return std::get<E>(m_value); }

private:
	std::variant<T, E> m_value;
};
} // namespace lmms

#endif // LMMS_POLYFILL_H