/*
 * ArrayVector.h
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

#ifndef LMMS_ARRAY_VECTOR_H
#define LMMS_ARRAY_VECTOR_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>
#include <type_traits>

namespace lmms {

namespace detail {

template<typename T, typename = void>
constexpr bool is_input_iterator_v = false;

template<typename T>
constexpr bool is_input_iterator_v<T, std::void_t<typename std::iterator_traits<T>::iterator_category>> =
	std::is_convertible_v<typename std::iterator_traits<T>::iterator_category, std::input_iterator_tag>;

} // namespace detail

/**
 * A container that stores up to a maximum of `N` elements of type `T` directly
 * within itself, rather than separately on the heap. Useful when a dynamically
 * resizeable container is needed for use in real-time code. Can be thought of
 * as a hybrid between `std::array` and `std::vector`. The interface follows
 * that of `std::vector` - see standard C++ documentation.
 */
template<typename T, std::size_t N>
class ArrayVector
{
public:
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using pointer = T*;
	using const_pointer = const T*;
	using iterator = pointer;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	ArrayVector() = default;

	ArrayVector(const ArrayVector& other) noexcept(std::is_nothrow_copy_constructible_v<T>) :
		m_size{other.m_size}
	{
		std::uninitialized_copy(other.begin(), other.end(), begin());
	}

	ArrayVector(ArrayVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) :
		m_size{other.m_size}
	{
		std::uninitialized_move(other.begin(), other.end(), begin());
		other.clear();
	}

	ArrayVector(size_type count, const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) :
		m_size{count}
	{
		assert(count <= N);
		std::uninitialized_fill_n(begin(), count, value);
	}

	explicit ArrayVector(size_type count) noexcept(std::is_nothrow_default_constructible_v<T>) :
		m_size{count}
	{
		assert(count <= N);
		std::uninitialized_value_construct_n(begin(), count);
	}

	template<typename It, std::enable_if_t<detail::is_input_iterator_v<It>, int> = 0>
	ArrayVector(It first, It last)
	{
		// Can't check the size first as the iterator may not be multipass
		const auto end = std::uninitialized_copy(first, last, begin());
		m_size = end - begin();
		assert(m_size <= N);
	}

	ArrayVector(std::initializer_list<T> il) noexcept(std::is_nothrow_copy_constructible_v<T>) :
		m_size{il.size()}
	{
		assert(il.size() <= N);
		std::uninitialized_copy(il.begin(), il.end(), begin());
	}

	~ArrayVector() { std::destroy(begin(), end()); }

	ArrayVector& operator=(const ArrayVector& other)
		noexcept(std::is_nothrow_copy_assignable_v<T> && std::is_nothrow_copy_constructible_v<T>)
	{
		if (this != &other) {
			const auto toAssign = std::min(other.size(), size());
			const auto assignedFromEnd = other.begin() + toAssign;
			const auto assignedToEnd = std::copy(other.begin(), other.begin() + toAssign, begin());
			std::destroy(assignedToEnd, end());
			std::uninitialized_copy(assignedFromEnd, other.end(), end());
			m_size = other.size();
		}
		return *this;
	}

	ArrayVector& operator=(ArrayVector&& other)
		noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>)
	{
		if (this != &other) {
			const auto toAssign = std::min(other.size(), size());
			const auto assignedFromEnd = other.begin() + toAssign;
			const auto assignedToEnd = std::move(other.begin(), other.begin() + toAssign, begin());
			std::destroy(assignedToEnd, end());
			std::uninitialized_move(assignedFromEnd, other.end(), end());
			m_size = other.size();
			other.clear();
		}
		return *this;
	}

	ArrayVector& operator=(std::initializer_list<T> il)
		noexcept(std::is_nothrow_copy_assignable_v<T> && std::is_nothrow_copy_constructible_v<T>)
	{
		assert(il.size() <= N);
		const auto toAssign = std::min(il.size(), size());
		const auto assignedFromEnd = il.begin() + toAssign;
		const auto assignedToEnd = std::copy(il.begin(), assignedFromEnd, begin());
		std::destroy(assignedToEnd, end());
		std::uninitialized_copy(assignedFromEnd, il.end(), end());
		m_size = il.size();
		return *this;
	}

	void assign(size_type count, const T& value)
		noexcept(std::is_nothrow_copy_assignable_v<T> && std::is_nothrow_copy_constructible_v<T>)
	{
		assert(count <= N);
		const auto temp = value;
		const auto toAssign = std::min(count, size());
		const auto toConstruct = count - toAssign;
		const auto assignedToEnd = std::fill_n(begin(), toAssign, temp);
		std::destroy(assignedToEnd, end());
		std::uninitialized_fill_n(assignedToEnd, toConstruct, temp);
		m_size = count;
	}

	template<typename It, std::enable_if_t<detail::is_input_iterator_v<It>, int> = 0>
	void assign(It first, It last)
	{
		// Can't check the size first as the iterator may not be multipass
		auto pos = begin();
		for (; first != last && pos != end(); ++pos, ++first) {
			*pos = *first;
		}
		std::destroy(pos, end());
		pos = std::uninitialized_copy(first, last, pos);
		m_size = pos - begin();
		assert(m_size <= N);
	}

	reference at(size_type index)
	{
		if (index >= m_size) { throw std::out_of_range{"index out of range"}; }
		return data()[index];
	}

	const_reference at(size_type index) const
	{
		if (index >= m_size) { throw std::out_of_range{"index out of range"}; }
		return data()[index];
	}

	reference operator[](size_type index) noexcept
	{
		assert(index < m_size);
		return data()[index];
	}

	const_reference operator[](size_type index) const noexcept
	{
		assert(index < m_size);
		return data()[index];
	}

	reference front() noexcept { return operator[](0); }
	const_reference front() const noexcept { return operator[](0); }

	reference back() noexcept { return operator[](m_size - 1); }
	const_reference back() const noexcept { return operator[](m_size - 1); }

	pointer data() noexcept { return *std::launder(reinterpret_cast<T(*)[N]>(m_data)); }
	const_pointer data() const noexcept { return *std::launder(reinterpret_cast<const T(*)[N]>(m_data)); }

	iterator begin() noexcept { return data(); }
	const_iterator begin() const noexcept { return data(); }
	const_iterator cbegin() const noexcept { return data(); }

	iterator end() noexcept { return data() + m_size; }
	const_iterator end() const noexcept { return data() + m_size; }
	const_iterator cend() const noexcept { return data() + m_size; }

	reverse_iterator rbegin() noexcept { return std::reverse_iterator{end()}; }
	const_reverse_iterator rbegin() const noexcept { return std::reverse_iterator{end()}; }
	const_reverse_iterator crbegin() const noexcept { return std::reverse_iterator{cend()}; }

	reverse_iterator rend() noexcept { return std::reverse_iterator{begin()}; }
	const_reverse_iterator rend() const noexcept { return std::reverse_iterator{begin()}; }
	const_reverse_iterator crend() const noexcept { return std::reverse_iterator{cbegin()}; }

	bool empty() const noexcept { return m_size == 0; }
	bool full() const noexcept { return m_size == N; }
	size_type size() const noexcept { return m_size; }
	size_type max_size() const noexcept { return N; }
	size_type capacity() const noexcept { return N; }

	void clear() noexcept
	{
		std::destroy(begin(), end());
		m_size = 0;
	}

	iterator insert(const_iterator pos, const T& value) { return emplace(pos, value); }
	iterator insert(const_iterator pos, T&& value) { return emplace(pos, std::move(value)); }

	iterator insert(const_iterator pos, size_type count, const T& value)
	{
		assert(m_size + count <= N);
		assert(cbegin() <= pos && pos <= cend());
		const auto mutPos = begin() + (pos - cbegin());
		const auto newEnd = std::uninitialized_fill_n(end(), count, value);
		std::rotate(mutPos, end(), newEnd);
		m_size += count;
		return mutPos;
	}

	template<typename It, std::enable_if_t<detail::is_input_iterator_v<It>, int> = 0>
	iterator insert(const_iterator pos, It first, It last)
	{
		// Can't check the size first as the iterator may not be multipass
		assert(cbegin() <= pos && pos <= cend());
		const auto mutPos = begin() + (pos - cbegin());
		const auto newEnd = std::uninitialized_copy(first, last, end());
		std::rotate(mutPos, end(), newEnd);
		m_size = newEnd - begin();
		assert(m_size <= N);
		return mutPos;
	}

	iterator insert(const_iterator pos, std::initializer_list<T> il) { return insert(pos, il.begin(), il.end()); }

	template<typename... Args>
	iterator emplace(const_iterator pos, Args&&... args)
	{
		assert(cbegin() <= pos && pos <= cend());
		const auto mutPos = begin() + (pos - cbegin());
		emplace_back(std::forward<Args>(args)...);
		std::rotate(mutPos, end() - 1, end());
		return mutPos;
	}

	iterator erase(const_iterator pos) { return erase(pos, pos + 1); }
	iterator erase(const_iterator first, const_iterator last)
	{
		assert(cbegin() <= first && first <= last && last <= cend());
		const auto mutFirst = begin() + (first - cbegin());
		const auto mutLast = begin() + (last - cbegin());
		const auto newEnd = std::move(mutLast, end(), mutFirst);
		std::destroy(newEnd, end());
		m_size = newEnd - begin();
		return mutFirst;
	}

	void push_back(const T& value) { emplace_back(value); }
	void push_back(T&& value) { emplace_back(std::move(value)); }

	template<typename... Args>
	reference emplace_back(Args&&... args)
	{
		assert(!full());
		// TODO C++20: Use std::construct_at
		const auto result = new(static_cast<void*>(end())) T(std::forward<Args>(args)...);
		++m_size;
		return *result;
	}

	void pop_back()
	{
		assert(!empty());
		--m_size;
		std::destroy_at(end());
	}

	void resize(size_type size)
	{
		if (size > N) { throw std::length_error{"size exceeds maximum size"}; }
		if (size < m_size) {
			std::destroy(begin() + size, end());
		} else {
			std::uninitialized_value_construct(end(), begin() + size);
		}
		m_size = size;
	}

	void resize(size_type size, const value_type& value)
	{
		if (size > N) { throw std::length_error{"size exceeds maximum size"}; }
		if (size < m_size) {
			std::destroy(begin() + size, end());
		} else {
			std::uninitialized_fill(end(), begin() + size, value);
		}
		m_size = size;
	}

	void swap(ArrayVector& other)
		noexcept(std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>)
	{
		using std::swap;
		swap(*this, other);
	}

	friend void swap(ArrayVector& a, ArrayVector& b)
		noexcept(std::is_nothrow_swappable_v<T> && std::is_nothrow_move_constructible_v<T>)
	{
		const auto toSwap = std::min(a.size(), b.size());
		const auto aSwapEnd = a.begin() + toSwap;
		const auto bSwapEnd = b.begin() + toSwap;
		std::swap_ranges(a.begin(), aSwapEnd, b.begin());
		std::uninitialized_move(aSwapEnd, a.end(), bSwapEnd);
		std::uninitialized_move(bSwapEnd, b.end(), aSwapEnd);
		std::destroy(aSwapEnd, a.end());
		std::destroy(bSwapEnd, b.end());
		std::swap(a.m_size, b.m_size);
	}

	// TODO C++20: Replace with operator<=>
	friend bool operator<(const ArrayVector& l, const ArrayVector& r)
	{
		return std::lexicographical_compare(l.begin(), l.end(), r.begin(), r.end());
	}
	friend bool operator<=(const ArrayVector& l, const ArrayVector& r) { return !(r < l); }
	friend bool operator>(const ArrayVector& l, const ArrayVector& r) { return r < l; }
	friend bool operator>=(const ArrayVector& l, const ArrayVector& r) { return !(l < r); }

	friend bool operator==(const ArrayVector& l, const ArrayVector& r)
	{
		return std::equal(l.begin(), l.end(), r.begin(), r.end());
	}
	// TODO C++20: Remove
	friend bool operator!=(const ArrayVector& l, const ArrayVector& r) { return !(l == r); }

private:
	alignas(T) std::byte m_data[std::max(N * sizeof(T), std::size_t{1})]; // Intentionally a raw array
	size_type m_size = 0;
};

} // namespace lmms

#endif // LMMS_ARRAY_VECTOR_H
