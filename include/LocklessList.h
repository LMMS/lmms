/*
 * LocklessList.h
 *
 * Copyright (c) 2018 Lukas W <lukaswhl/at/gmail.com>
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

#ifndef LOCKLESS_LIST_H
#define LOCKLESS_LIST_H

#include <boost/double_ended/devector.hpp>
#include <cds/container/fcdeque.h>
#include "libcds.h"

#include "Memory.h"

#include "LocklessList_fwd.h"

template<typename T, unsigned N, typename A>
using _devector = boost::double_ended::devector<T,
	boost::double_ended::small_buffer_size<N>,
	boost::double_ended::devector_growth_policy,
	A
>;

// Use erenon's devector as a good tradeoff between std::vector and std::deque.
// It offers reserve() functionality like std::vector, while still supporting
// push_back() and push_front() in amortized O(1).
template<typename T, unsigned N>
using _LoocklessDeque_Base = cds::container::FCDeque<T, _devector<T, N, MmAllocator<T>>>;

template<typename T, unsigned N>
class LocklessList : private _LoocklessDeque_Base<T, N>
{
	using Base = _LoocklessDeque_Base<T, N>;
public:
	using Container = typename Base::deque_type;
	using value_type = T;

	LocklessList(size_t size = 0) : Base()
	{
		reserve_back(size);
	}

	void reserve_back(size_t size)
	{
		Base::apply([size](Container& st) {
			st.reserve_back(size);
		});
	}
	void reserve_front(size_t size)
	{
		Base::apply([size](Container& st) {
			st.reserve_front(size);
		});
	}

	using Base::push_front;
	using Base::pop_front;
	using Base::push_back;
	using Base::pop_back;
	using Base::apply;
};

/// Convenience subclass providing push and pop operations (equivalent to
/// push_back and pop_back)
template<typename T, unsigned N>
class LocklessStack : public LocklessList<T, N>
{
	using Base = LocklessList<T, N>;
public:
	using LocklessList<T>::LocklessList;
	void push(const T& value) { Base::push_back(value); }
	void push(T&& value) { Base::push_back(value); }
	bool pop(T& value) { return Base::pop_back(value); }
};

#endif
