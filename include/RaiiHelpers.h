/*
 * RaiiHelpers.h
 *
 * Copyright (c) 2022 Dominic Clark <mrdomclark/at/gmail.com>
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

#ifndef LMMS_RAII_HELPERS_H
#define LMMS_RAII_HELPERS_H

#include <cstddef>
#include <memory>


namespace lmms
{

template<typename T, T Null>
class NullableResource
{
public:
	NullableResource() = default;
	NullableResource(std::nullptr_t) noexcept { }
	NullableResource(T value) noexcept : m_value{value} { }
	operator T() const noexcept { return m_value; }
	explicit operator bool() const noexcept { return m_value != Null; }
	friend bool operator==(NullableResource a, NullableResource b) noexcept { return a.m_value == b.m_value; }
	friend bool operator==(NullableResource a, T b) noexcept { return a.m_value == b; }
	friend bool operator==(T a, NullableResource b) noexcept { return a == b.m_value; }
	friend bool operator!=(NullableResource a, NullableResource b) noexcept { return a.m_value != b.m_value; }
	friend bool operator!=(NullableResource a, T b) noexcept { return a.m_value != b; }
	friend bool operator!=(T a, NullableResource b) noexcept { return a != b.m_value; }

private:
	T m_value = Null;
};

template<typename T, T Null, auto Deleter>
struct NullableResourceDeleter
{
	using pointer = NullableResource<T, Null>;
	void operator()(T value) const noexcept { Deleter(value); }
};

template<typename T, T Null, auto Deleter>
using UniqueNullableResource = std::unique_ptr<T, NullableResourceDeleter<T, Null, Deleter>>;

} // namespace lmms

#endif // LMMS_RAII_HELPERS_H
