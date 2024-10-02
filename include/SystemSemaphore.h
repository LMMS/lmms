/*
 * SystemSemaphore.h
 *
 * Copyright (c) 2024 Dominic Clark
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

#ifndef LMMS_SYSTEM_SEMAPHORE_H
#define LMMS_SYSTEM_SEMAPHORE_H

#include <memory>
#include <string>

namespace lmms {

namespace detail {

class SystemSemaphoreImpl;

} // namespace detail

class SystemSemaphore
{
public:
	SystemSemaphore() noexcept;
	SystemSemaphore(std::string key, unsigned int value);
	explicit SystemSemaphore(std::string key);
	~SystemSemaphore();

	SystemSemaphore(SystemSemaphore&& other) noexcept;
	auto operator=(SystemSemaphore&& other) noexcept -> SystemSemaphore&;

	auto acquire() noexcept -> bool;
	auto release() noexcept -> bool;

	auto key() const noexcept -> const std::string& { return m_key; }

private:
	std::string m_key;
	std::unique_ptr<detail::SystemSemaphoreImpl> m_impl;
};

} // namespace lmms

#endif // LMMS_SYSTEM_SEMAPHORE_H
