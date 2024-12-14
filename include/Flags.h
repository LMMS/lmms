/*
 * Flags.h - class to make flags from enums
 *
 * Copyright (c) 2023 Dominic Clark
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

#ifndef LMMS_FLAGS_H
#define LMMS_FLAGS_H

#include <type_traits>

namespace lmms {

template<typename T>
class Flags
{
	static_assert(std::is_enum_v<T>, "lmms::Flags can only be used with enum types");

public:
	using EnumType = T;
	using UnderlyingType = std::underlying_type_t<T>;

	constexpr Flags() = default;

	constexpr Flags(T value) : // Intentionally not explicit
		m_value{static_cast<UnderlyingType>(value)}
	{}

	constexpr explicit Flags(UnderlyingType value) :
		m_value{value}
	{}

	constexpr auto testAll(Flags flags) const -> bool { return (*this & flags) == flags; }
	constexpr auto testAny(Flags flags) const -> bool { return (*this & flags) != Flags{}; }
	constexpr auto testFlag(EnumType flag) const -> bool { return static_cast<bool>(*this & flag); }

	constexpr void setFlag(EnumType flag, bool value = true)
	{
		m_value = value ? (m_value | flag) : (m_value & ~flag);
	}

	constexpr auto operator~() const -> Flags { return Flags{~m_value}; }
	friend constexpr auto operator&(Flags l, Flags r) -> Flags { return Flags{l.m_value & r.m_value}; }
	friend constexpr auto operator|(Flags l, Flags r) -> Flags { return Flags{l.m_value | r.m_value}; }
	friend constexpr auto operator^(Flags l, Flags r) -> Flags { return Flags{l.m_value ^ r.m_value}; }
	friend constexpr auto operator+(Flags l, Flags r) -> Flags { return Flags{l.m_value | r.m_value}; }
	friend constexpr auto operator-(Flags l, Flags r) -> Flags { return Flags{l.m_value & ~r.m_value}; }

	constexpr auto operator&=(Flags f) -> Flags& { m_value &= f.m_value; return *this; }
	constexpr auto operator|=(Flags f) -> Flags& { m_value |= f.m_value; return *this; }
	constexpr auto operator^=(Flags f) -> Flags& { m_value ^= f.m_value; return *this; }
	constexpr auto operator+=(Flags f) -> Flags& { m_value |= f.m_value; return *this; }
	constexpr auto operator-=(Flags f) -> Flags& { m_value &= ~f.m_value; return *this; }

	constexpr explicit operator UnderlyingType() const { return m_value; } // TODO C++23: explicit(std::is_scoped_enum<T>)
	constexpr explicit operator bool() const { return m_value != 0; }

	friend constexpr auto operator==(Flags l, Flags r) -> bool { return l.m_value == r.m_value; } // TODO C++20: = default
	friend constexpr auto operator!=(Flags l, Flags r) -> bool { return l.m_value != r.m_value; } // TODO C++20: Remove

private:
	UnderlyingType m_value = 0;
};

#define LMMS_DECLARE_OPERATORS_FOR_FLAGS(type) \
constexpr inline auto operator|(type l, type r) -> ::lmms::Flags<type> { return ::lmms::Flags{l} | ::lmms::Flags{r}; }

} // namespace lmms

#endif // LMMS_FLAGS_H
