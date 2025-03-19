/*
 * Cache.h
 *
 * Copyright (c) 2024 Sotonye Atemie <satemiej@gmail.com>
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

#ifndef LMMS_CACHE_H
#define LMMS_CACHE_H

#include <memory>

namespace lmms {
template <typename K, typename V> class Cache
{
public:
	//! Gets the cached resource `V` from `key`.
	//! On cache miss, a new resource should be created, added to the cache, and then returned.
	//! On cache hit, the cached resource can simply be returned.
	auto get(const K& key) -> std::shared_ptr<V> = 0;

	//! Adds a new resource with the specified `key` if it isn't already cached.
    //! Returns `true` if a resource was added to the cache and `false` otherwise.
	bool add(const K& key) = 0;

	//! Removes a resource with the specified `key` if it is already cached.
    //! Returns `true` if a resource was removed to the cache and `false` otherwise.
	bool remove(const K& key) = 0;
};
} // namespace lmms

#endif // LMMS_CACHE_H