/*
 * Span.h - A C++17 compatible implementation of C++20's std::span.
 *          Includes additions from C++23 and C++26, but does not
 *          support ranges.
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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

#ifndef LMMS_SPAN_H
#define LMMS_SPAN_H

#include <array>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>

// TODO C++20: Use std::span instead
#if __cplusplus >= 202002L && __has_include(<span>) && !defined(_MSC_VER)
#warning "Time to replace lmms::Span with std::span"
#endif

namespace lmms
{

inline constexpr auto dynamic_extent = static_cast<std::size_t>(-1);


//! C++20 std::type_identity implementation
template<class T>
struct type_identity { using type = T; };

template< class T >
using type_identity_t = typename type_identity<T>::type;


//! Span with static extent
template<class T, std::size_t Extent = dynamic_extent>
class Span
{
public:
	// constants and types
	using element_type           = T;
	using value_type             = std::remove_cv_t<T>;
	using size_type              = std::size_t;
	using difference_type        = std::ptrdiff_t;
	using pointer                = T*;
	using const_pointer          = const T*;
	using reference              = element_type&;
	using const_reference        = const element_type&;
	using iterator               = pointer;
	using const_iterator         = const_pointer;
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	static constexpr std::size_t extent = Extent;

	// constructors, copy, and assignment
	template<auto ext = Extent, std::enable_if_t<ext == 0, bool> = true>
	constexpr Span() noexcept {}

	constexpr explicit Span(T* data, std::size_t count)
		: m_data{data}
	{
		assert(count == Extent);
	}

	constexpr Span(type_identity_t<T> (&arr)[Extent]) noexcept : m_data{arr} {}

	template<class U, std::enable_if_t<std::is_convertible_v<U*, T*>, bool> = true>
	constexpr Span(std::array<U, Extent>& arr) noexcept
		: m_data{arr.data()}
	{
	}

	template<class U, std::enable_if_t<std::is_convertible_v<const U*, T*>, bool> = true>
	constexpr Span(const std::array<U, Extent>& arr) noexcept
		: m_data{arr.data()}
	{
	}

	// C++26 P2447R6
	template<class U = T, std::enable_if_t<std::is_const_v<U>, bool> = true>
	constexpr explicit Span(std::initializer_list<value_type> il)
		: m_data{il.begin()}
	{
		assert(il.size() == Extent);
	}

	template<class U, std::enable_if_t<std::is_convertible_v<U*, T*>, bool> = true>
	constexpr Span(const Span<U, Extent>& source) noexcept
		: m_data{source.data()}
	{
	}

	template<class U, std::enable_if_t<std::is_convertible_v<U*, T*>, bool> = true>
	constexpr explicit Span(const Span<U, dynamic_extent>& source) noexcept
		: m_data{source.data()}
	{
		assert(source.size() == Extent);
	}

	constexpr Span(const Span& other) noexcept = default;

	constexpr auto operator=(const Span& other) noexcept -> Span& = default;

	// subviews
	template<std::size_t Count>
	constexpr auto first() const -> Span<T, Count>
	{
		static_assert(Count <= Extent);
		return Span<T, Count>(m_data, Count);
	}

	template<std::size_t Count>
	constexpr auto last() const -> Span<T, Count>
	{
		static_assert(Count <= Extent);
		return Span<T, Count>(m_data + Extent - Count, Count);
	}

	template<std::size_t Offset, std::size_t Count = dynamic_extent>
	constexpr auto subspan() const -> Span<T, Count>
	{
		static_assert(Offset <= Extent);
		if constexpr (Count != dynamic_extent) {
			static_assert(Count <= Extent - Offset);
			return Span<T, Count>(m_data + Offset, Count);
		} else {
			return Span<T, Count>(m_data + Offset, Extent - Offset);
		}
	}

	constexpr auto first(std::size_t count) const -> Span<T, dynamic_extent>
	{
		assert(count <= Extent);
		return Span<T, dynamic_extent>(m_data, count);
	}

	constexpr auto last(std::size_t count) const -> Span<T, dynamic_extent>
	{
		assert(count <= Extent);
		return Span<T, dynamic_extent>(m_data + Extent - count, count);
	}

	constexpr auto subspan(std::size_t offset, std::size_t count = dynamic_extent) const
		-> Span<T, dynamic_extent>
	{
		assert(offset <= Extent);
		if (count != dynamic_extent) {
			assert(count <= Extent - offset);
			return Span<T, dynamic_extent>(m_data + offset, count);
		} else {
			return Span<T, dynamic_extent>(m_data + offset, Extent - offset);
		}
	}

	// observers
	constexpr auto size() const noexcept -> std::size_t { return Extent; }
	constexpr auto size_bytes() const noexcept -> std::size_t { return Extent * sizeof(T); }
	constexpr auto empty() const noexcept -> bool { return Extent == 0; }

	// element access
	constexpr auto operator[](std::size_t idx) const -> T& { return m_data[idx]; }

	constexpr auto at(std::size_t idx) const -> T&
	{
		if (idx >= Extent) { throw std::out_of_range{"Span::at()"}; }
		return m_data[idx];
	}

	constexpr auto front() const -> T& { return *m_data; }
	constexpr auto back() const -> T& { return *(m_data + Extent - 1); }
	constexpr auto data() const noexcept -> T* { return m_data; }

	// iterator support
	constexpr auto begin() const noexcept -> T* { return m_data; }
	constexpr auto end() const noexcept -> T* { return m_data + Extent; }
	constexpr auto cbegin() const noexcept -> const T* { return begin(); }
	constexpr auto cend() const noexcept -> const T* { return end(); }
	constexpr auto rbegin() const noexcept -> reverse_iterator { return reverse_iterator{end()}; }
	constexpr auto rend() const noexcept -> reverse_iterator { return reverse_iterator{begin()}; }
	constexpr auto crbegin() const noexcept -> const_reverse_iterator { return rbegin(); }
	constexpr auto crend() const noexcept -> const_reverse_iterator { return rend(); }

private:
	pointer m_data = nullptr;
};


//! Span with dynamic extent
template<class T>
class Span<T, dynamic_extent>
{
public:
	// constants and types
	using element_type           = T;
	using value_type             = std::remove_cv_t<T>;
	using size_type              = std::size_t;
	using difference_type        = std::ptrdiff_t;
	using pointer                = T*;
	using const_pointer          = const T*;
	using reference              = element_type&;
	using const_reference        = const element_type&;
	using iterator               = pointer;
	using const_iterator         = const_pointer;
	using reverse_iterator       = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	static constexpr std::size_t extent = dynamic_extent;

	// constructors, copy, and assignment
	constexpr Span() noexcept = default;

	constexpr Span(T* data, std::size_t count)
		: m_data{data}
		, m_size{count}
	{
	}

	template<std::size_t N>
	constexpr Span(type_identity_t<T> (&arr)[N]) noexcept
		: m_data{arr}
		, m_size{N}
	{
	}

	template<class U, std::size_t N, std::enable_if_t<std::is_convertible_v<U*, T*>, bool> = true>
	constexpr Span(std::array<U, N>& arr) noexcept
		: m_data{arr.data()}
		, m_size{N}
	{
	}

	template<class U, std::size_t N, std::enable_if_t<std::is_convertible_v<const U*, T*>, bool> = true>
	constexpr Span(const std::array<U, N>& arr) noexcept
		: m_data{arr.data()}
		, m_size{N}
	{
	}

	// C++26 P2447R6
	template<class U = T, std::enable_if_t<std::is_const_v<U>, bool> = true>
	constexpr Span(std::initializer_list<value_type> il) noexcept
		: m_data{il.begin()}
		, m_size{il.size()}
	{
	}

	template<class U, std::size_t N, std::enable_if_t<N != dynamic_extent, bool> = true>
	constexpr Span(const Span<U, N>& source) noexcept
		: m_data{source.data()}
		, m_size{N}
	{
	}

	constexpr Span(const Span& other) noexcept = default;

	constexpr auto operator=(const Span& other) noexcept -> Span& = default;

	// subviews
	template<std::size_t Count>
	constexpr auto first() const -> Span<T, Count>
	{
		assert(Count <= m_size);
		return Span<T, Count>(m_data, Count);
	}

	template<std::size_t Count>
	constexpr auto last() const -> Span<T, Count>
	{
		assert(Count <= m_size);
		return Span<T, Count>(m_data + m_size - Count, Count);
	}

	template<std::size_t Offset, std::size_t Count = dynamic_extent>
	constexpr auto subspan() const -> Span<T, Count>
	{
		assert(Offset <= m_size);
		if constexpr (Count != dynamic_extent) {
			assert(Count <= m_size - Offset);
			return Span<T, Count>(m_data + Offset, Count);
		} else {
			return Span<T, Count>(m_data + Offset, m_size - Offset);
		}
	}

	constexpr auto first(std::size_t count) const -> Span<T, dynamic_extent>
	{
		assert(count <= m_size);
		return Span<T, dynamic_extent>(m_data, count);
	}

	constexpr auto last(std::size_t count) const -> Span<T, dynamic_extent>
	{
		assert(count <= m_size);
		return Span<T, dynamic_extent>(m_data + m_size - count, count);
	}

	constexpr auto subspan(std::size_t offset, std::size_t count = dynamic_extent) const
		-> Span<T, dynamic_extent>
	{
		assert(offset <= m_size);
		if (count != dynamic_extent) {
			assert(count <= m_size - offset);
			return Span<T, dynamic_extent>(m_data + offset, count);
		} else {
			return Span<T, dynamic_extent>(m_data + offset, m_size - offset);
		}
	}

	// observers
	constexpr auto size() const noexcept -> std::size_t { return m_size; }
	constexpr auto size_bytes() const noexcept -> std::size_t { return m_size * sizeof(T); }
	constexpr auto empty() const noexcept -> bool { return m_size == 0; }

	// element access
	constexpr auto operator[](std::size_t idx) const -> T& { return m_data[idx]; }

	constexpr auto at(std::size_t idx) const -> T&
	{
		if (idx >= m_size) { throw std::out_of_range{"Span::at()"}; }
		return m_data[idx];
	}

	constexpr auto front() const -> T& { return *m_data; }
	constexpr auto back() const -> T& { return *(m_data + m_size - 1); }
	constexpr auto data() const noexcept -> T* { return m_data; }

	// iterator support
	constexpr auto begin() const noexcept -> T* { return m_data; }
	constexpr auto end() const noexcept -> T* { return m_data + m_size; }
	constexpr auto cbegin() const noexcept -> const T* { return begin(); }
	constexpr auto cend() const noexcept -> const T* { return end(); }
	constexpr auto rbegin() const noexcept -> reverse_iterator { return reverse_iterator{end()}; }
	constexpr auto rend() const noexcept -> reverse_iterator { return reverse_iterator{begin()}; }
	constexpr auto crbegin() const noexcept -> const_reverse_iterator { return rbegin(); }
	constexpr auto crend() const noexcept -> const_reverse_iterator { return rend(); }

private:
	pointer m_data = nullptr;
	size_type m_size = 0;
};


// Deduction guides

template<class T>
Span(T*, std::size_t) -> Span<T>;

template<class T, std::size_t N>
Span(T (&)[N]) -> Span<T, N>;

template<class T, std::size_t N>
Span(std::array<T, N>&) -> Span<T, N>;

template<class T, std::size_t N>
Span(const std::array<T, N>&) -> Span<const T, N>;


// Non-member functions

template<class T, std::size_t N>
inline auto as_bytes(Span<T, N> s) noexcept
	-> Span<const std::byte, (N == dynamic_extent ? dynamic_extent : N * sizeof(T))>
{
	using Type = Span<const std::byte, (N == dynamic_extent ? dynamic_extent : N * sizeof(T))>;
	return Type(reinterpret_cast<const std::byte*>(s.data()), s.size_bytes());
}

template<class T, std::size_t N, std::enable_if_t<!std::is_const_v<T>, bool> = true>
inline auto as_writable_bytes(Span<T, N> s) noexcept
	-> Span<std::byte, (N == dynamic_extent ? dynamic_extent : N * sizeof(T))>
{
	using Type = Span<std::byte, (N == dynamic_extent ? dynamic_extent : N * sizeof(T))>;
	return Type(reinterpret_cast<std::byte*>(s.data()), s.size_bytes());
}


} // namespace lmms

#endif // LMMS_SPAN_H
